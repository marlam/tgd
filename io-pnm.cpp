/*
 * Copyright (C) 2019 Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstdio>
#include <cstring>
#include <limits>

#include "io-pnm.hpp"

extern "C"
{
/* This header must come last because it contains so much junk that
 * it messes up other headers. */
#include <pam.h>
#undef max
}

namespace TAD {

FormatImportExportPNM::FormatImportExportPNM() :
    _f(nullptr),
    _arrayCount(-2)
{
    pm_init("libtadio-netpbm", 0);
}

FormatImportExportPNM::~FormatImportExportPNM()
{
    close();
}

Error FormatImportExportPNM::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        _f = stdin;
    else
        _f = fopen(fileName.c_str(), "rb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportPNM::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (fileName == "-")
        _f = stdout;
    else
        _f = fopen(fileName.c_str(), append ? "ab" : "wb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportPNM::close()
{
    if (_f) {
        if (_f != stdin && _f != stdout) {
            if (fclose(_f) != 0) {
                _f = nullptr;
                return ErrorSysErrno;
            }
        }
        _f = nullptr;
    }
    return ErrorNone;
}

int FormatImportExportPNM::arrayCount()
{
    if (_arrayCount >= -1)
        return _arrayCount;

    if (!_f)
        return -1;

    // find offsets of all PNMs in the file
    off_t curPos = ftello(_f);
    if (curPos < 0) {
        _arrayCount = -1;
        return _arrayCount;
    }
    rewind(_f);

    struct pam inpam;
    tuple *tuplerow;

    while (hasMore()) {
        off_t arrayPos = ftello(_f);
        if (arrayPos < 0) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
#ifdef PAM_STRUCT_SIZE
        pnm_readpaminit(_f, &inpam, PAM_STRUCT_SIZE(tuple_type));
#else
        pnm_readpaminit(_f, &inpam, sizeof(struct pam));
#endif
        tuplerow = pnm_allocpamrow(&inpam);
        for (int y = 0; y < inpam.height; y++)
            pnm_readpamrow(&inpam, tuplerow);
        pnm_freepamrow(tuplerow);

        _arrayOffsets.push_back(arrayPos);
        if (_arrayOffsets.size() == size_t(std::numeric_limits<int>::max()) && hasMore()) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
    }
    if (fseeko(_f, curPos, SEEK_SET) < 0) {
        _arrayOffsets.clear();
        _arrayCount = -1;
        return -1;
    }
    _arrayCount = _arrayOffsets.size();
    return _arrayCount;
}

ArrayContainer FormatImportExportPNM::readArray(Error* error, int arrayIndex)
{
    // Seek if necessary
    if (arrayIndex >= 0) {
        if (arrayCount() < 0) {
            *error = ErrorSeekingNotSupported;
            return ArrayContainer();
        }
        if (arrayIndex >= arrayCount()) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        if (fseeko(_f, _arrayOffsets[arrayIndex], SEEK_SET) < 0) {
            *error = ErrorSysErrno;
            return ArrayContainer();
        }
    }

    // Read the PNM
    struct pam inpam;
    tuple *tuplerow;
#ifdef PAM_STRUCT_SIZE
    pnm_readpaminit(_f, &inpam, PAM_STRUCT_SIZE(tuple_type));
#else
    pnm_readpaminit(_f, &inpam, sizeof(struct pam));
#endif
    if (inpam.width < 1 || inpam.height < 1 || inpam.depth < 1
            || (inpam.bytes_per_sample != 1 && inpam.bytes_per_sample != 2
                && inpam.bytes_per_sample != 4)) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    Type type = (inpam.bytes_per_sample == 1 ? uint8
            : inpam.bytes_per_sample == 2 ? uint16
            : uint32);
    ArrayContainer r({ static_cast<size_t>(inpam.width), static_cast<size_t>(inpam.height) },
            static_cast<size_t>(inpam.depth), type);
    switch (inpam.depth) {
    case 1:
    case 2:
        if (type == uint8) {
            r.componentTagList(0).set("INTERPRETATION", "SRGB/LUM");
        } else {
            r.componentTagList(0).set("INTERPRETATION", "GRAY");
        }
        if (inpam.depth == 2) {
            r.componentTagList(1).set("INTERPRETATION", "ALPHA");
        }
        break;
    case 3:
    case 4:
        if (type == uint8) {
            r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
        } else {
            r.componentTagList(0).set("INTERPRETATION", "RED");
            r.componentTagList(1).set("INTERPRETATION", "GREEN");
            r.componentTagList(2).set("INTERPRETATION", "BLUE");
        }
        if (inpam.depth == 4) {
            r.componentTagList(3).set("INTERPRETATION", "ALPHA");
        }
        break;
    }

    tuplerow = pnm_allocpamrow(&inpam);
    for (int pamrow = 0; pamrow < inpam.height; pamrow++) {
        pnm_readpamrow(&inpam, tuplerow);
        size_t arrayrow = inpam.height - 1 - pamrow;
        for (size_t x = 0; x < r.dimension(0); x++) {
            for (size_t c = 0; c < r.componentCount(); c++) {
                if (r.componentType() == uint8) {
                    r.set<uint8_t>({ x, arrayrow }, c, tuplerow[x][c]);
                } else if (r.componentType() == uint16) {
                    r.set<uint16_t>({ x, arrayrow }, c, tuplerow[x][c]);
                } else {
                    r.set<uint32_t>({ x, arrayrow }, c, tuplerow[x][c]);
                }
            }
        }
    }
    pnm_freepamrow(tuplerow);

    // Return the array
    return r;
}

bool FormatImportExportPNM::hasMore()
{
    int c = fgetc(_f);
    if (c == EOF) {
        return false;
    } else {
        ungetc(c, _f);
        return true;
    }
}

Error FormatImportExportPNM::writeArray(const ArrayContainer& array)
{
    const size_t intmax = std::numeric_limits<int>::max();
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > intmax || array.dimension(1) > intmax
            || array.componentCount() <= 0 || array.componentCount() > 4
            || (array.componentType() != uint8 && array.componentType() != uint16
                && array.componentType() != uint32)) {
        return ErrorFeaturesUnsupported;
    }

    struct pam outpam;
    tuple *tuplerow;
    std::memset(&outpam, 0, sizeof(outpam));
    outpam.size = sizeof(struct pam);
#ifdef PAM_STRUCT_SIZE
    outpam.len = PAM_STRUCT_SIZE(tuple_type);
#else
    outpam.len = outpam.size;
#endif
    outpam.file = _f;
    outpam.width = array.dimension(0);
    outpam.height = array.dimension(1);
    outpam.depth = array.componentCount();
    outpam.maxval = (array.componentType() == uint8 ? std::numeric_limits<uint8_t>::max()
            : array.componentType() == uint16 ? std::numeric_limits<uint16_t>::max()
            : std::numeric_limits<uint32_t>::max());
    outpam.bytes_per_sample = array.componentSize();
    outpam.plainformat = 0;
    if (array.componentCount() == 1) {
        outpam.format = RPGM_FORMAT;
        std::strcpy(outpam.tuple_type, PAM_PGM_TUPLETYPE);
    } else if (array.componentCount() == 2) {
        outpam.format = PAM_FORMAT;
        std::strcpy(outpam.tuple_type, "GRAYSCALE_ALPHA");
    } else if (array.componentCount() == 3) {
        outpam.format = RPPM_FORMAT;
        std::strcpy(outpam.tuple_type, PAM_PPM_TUPLETYPE);
    } else {
        outpam.format = PAM_FORMAT;
        std::strcpy(outpam.tuple_type, "RGB_ALPHA");
    }
    pnm_writepaminit(&outpam);

    tuplerow = pnm_allocpamrow(&outpam);
    for (int pamrow = 0; pamrow < outpam.height; pamrow++) {
        size_t arrayrow = outpam.height - 1 - pamrow;
        for (size_t x = 0; x < array.dimension(0); x++) {
            for (size_t c = 0; c < array.componentCount(); c++) {
                if (array.componentType() == uint8) {
                    tuplerow[x][c] = array.get<uint8_t>({ x, arrayrow }, c);
                } else if (array.componentType() == uint16) {
                    tuplerow[x][c] = array.get<uint16_t>({ x, arrayrow }, c);
                } else {
                    tuplerow[x][c] = array.get<uint32_t>({ x, arrayrow }, c);
                }
            }
        }
        pnm_writepamrow(&outpam, tuplerow);
    }
    pnm_freepamrow(tuplerow);
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_pnm()
{
    return new FormatImportExportPNM();
}

}
