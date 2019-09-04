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
#include "io-utils.hpp"


namespace TAD {

FormatImportExportPNM::FormatImportExportPNM() :
    _f(nullptr),
    _arrayCount(-2)
{
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

typedef struct
{
    Error error;
    int width;
    int height;
    int depth;
    int maxval; // negative means floating point
    bool plain;
    float factor;
    bool needsEndianFix;
} PNMInfo;

static bool readPnmWhitespaceAndComments(FILE *f)
{
    int c;
    bool inComment = false;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) {
            return !ferror(f);
        } else {
            if (inComment && c == '\n') {
                inComment = false;
            } else {
                if (c == '#')
                    inComment = true;
                else if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n'))
                    return !(ungetc(c, f) == EOF);
            }
        }
    }
}

static bool readPnmWidthHeight(FILE* f, PNMInfo* info)
{
    return (readPnmWhitespaceAndComments(f)
            && fscanf(f, "%d", &info->width) == 1 && info->width > 0
            && readPnmWhitespaceAndComments(f)
            && fscanf(f, "%d", &info->height) == 1 && info->height > 0);
}

static bool readPnmMaxval(FILE* f, PNMInfo* info)
{
    bool ret = (readPnmWhitespaceAndComments(f)
            && fscanf(f, "%d", &info->maxval) == 1
            && info->maxval >= 1 && info->maxval <= 65535);
    info->needsEndianFix = (info->maxval > 255);
    return ret;
}

static bool readPfmFactor(FILE* f, PNMInfo* info)
{
    bool ret = (readPnmWhitespaceAndComments(f)
            && fscanf(f, "%f", &info->factor) == 1);
    info->needsEndianFix = (info->factor > 0.0f);
    if (info->factor < 0.0f)
        info->factor = -info->factor;
    return ret;
}

static bool readWhitespaceUntilNewline(FILE* f)
{
    for (;;) {
        int c = fgetc(f);
        if (c == '\n')
            return true;
        else if (c == ' ' || c == '\t' || c == '\r')
            continue;
        else
            return false;
    }
}

static bool readAnythingUntilNewline(FILE* f)
{
    for (;;) {
        int c = fgetc(f);
        if (c == '\n')
            return true;
        else if (c == EOF)
            return false;
        else
            continue;
    }
}

static bool readNonNewlineWhitespace(FILE* f)
{
    for (;;) {
        int c = fgetc(f);
        if (c == ' ' || c == '\t' || c == '\r') {
            continue;
        } else if (c == EOF) {
            return false;
        } else {
            return (ungetc(c, f) != EOF);
        }
    }
}

static bool readWhitespace(FILE* f)
{
    for (;;) {
        int c = fgetc(f);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        } else if (c == EOF) {
            return false;
        } else {
            return (ungetc(c, f) != EOF);
        }
    }
}

static bool readPamHeader(FILE* f, PNMInfo* info)
{
    for (;;) {
        if (!readNonNewlineWhitespace(f))
            return false;
        int c = fgetc(f);
        if (c == '\n') {
            // empty line
        } else if (c == '#') {
            if (!readAnythingUntilNewline(f))
                return false;
        } else if (c == 'W') {
            if (fscanf(f, "IDTH %d", &info->width) != 1 || !readWhitespaceUntilNewline(f))
                return false;
        } else if (c == 'H') {
            if (fscanf(f, "EIGHT %d", &info->height) != 1 || !readWhitespaceUntilNewline(f))
                return false;
        } else if (c == 'D') {
            if (fscanf(f, "EPTH %d", &info->depth) != 1 || !readWhitespaceUntilNewline(f))
                return false;
        } else if (c == 'M') {
            if (fscanf(f, "AXVAL %d", &info->maxval) != 1 || !readWhitespaceUntilNewline(f))
                return false;
        } else if (c == 'T') {
            if (!readAnythingUntilNewline(f))
                return false;
        } else if (c == 'E') {
            char ndhdr[5];
            if (fread(ndhdr, 5, 1, f) != 1
                    || strncmp(ndhdr, "NDHDR", 5) != 0
                    || !readWhitespaceUntilNewline(f))
                return false;
            break;
        }
    }
    if (info->width < 1 || info->height < 1
            || info->depth < 1 || info->depth > 4
            || info->maxval < 1 || info->maxval > 65535) {
        return false;
    }
    info->needsEndianFix = (info->maxval > 255);
    return true;
}

static PNMInfo readPnmHeader(FILE* f)
{
    PNMInfo info;
    info.error = ErrorNone;
    info.width = -1;
    info.height = -1;
    info.depth = -1;
    info.maxval = 0;
    info.plain = false;
    info.factor = 1.0f;
    info.needsEndianFix = false;

    int c = fgetc(f);
    if (c == EOF) {
        info.error = ErrorSysErrno;
    } else if (c != 'P') {
        info.error = ErrorInvalidData;
    } else {
        c = fgetc(f);
        if (c == EOF) {
            info.error = ErrorSysErrno;
        } else if (c == '2') {
            info.depth = 1;
            info.plain = true;
            if (!readPnmWidthHeight(f, &info) || !readPnmMaxval(f, &info) || !readWhitespaceUntilNewline(f))
                info.error = ErrorInvalidData;
        } else if (c == '3') {
            info.depth = 3;
            info.plain = true;
            if (!readPnmWidthHeight(f, &info) || !readPnmMaxval(f, &info) || !readWhitespaceUntilNewline(f))
                info.error = ErrorInvalidData;
        } else if (c == '5') {
            info.depth = 1;
            if (!readPnmWidthHeight(f, &info) || !readPnmMaxval(f, &info) || !readWhitespaceUntilNewline(f))
                info.error = ErrorInvalidData;
        } else if (c == '6') {
            info.depth = 3;
            if (!readPnmWidthHeight(f, &info) || !readPnmMaxval(f, &info) || !readWhitespaceUntilNewline(f))
                info.error = ErrorInvalidData;
        } else if (c == 'f') {
            info.depth = 1;
            info.maxval = -1;
            if (!readPnmWidthHeight(f, &info) || !readPfmFactor(f, &info) || !readWhitespaceUntilNewline(f))
                info.error = ErrorInvalidData;
        } else if (c == 'F') {
            info.depth = 3;
            info.maxval = -1;
            if (!readPnmWidthHeight(f, &info) || !readPfmFactor(f, &info) || !readWhitespaceUntilNewline(f))
                info.error = ErrorInvalidData;
        } else if (c == '7') {
            if (!readPamHeader(f, &info))
                info.error = ErrorInvalidData;
        } else {
            info.error = ErrorInvalidData;
        }
    }
    return info;
}

bool readPnmData(FILE* f, const PNMInfo& info, ArrayContainer& array)
{
    if (info.plain) {
        for (size_t e = 0; e < array.elementCount(); e++) {
            for (size_t c = 0; c < array.componentCount(); c++) {
                int val;
                if (!readWhitespace(f)
                        || fscanf(f, "%d", &val) != 1
                        || val < 0 || val > 65535
                        || (array.componentType() == uint8 && val > 255)) {
                    return false;
                }
                if (array.componentType() == uint8) {
                    array.set<uint8_t>(e, c, uint8_t(val));
                } else {
                    array.set<uint16_t>(e, c, uint16_t(val));
                }
            }
        }
        readWhitespace(f); // ignore EOF
        return true;
    } else {
        return (fread(array.data(), array.dataSize(), 1, f) == 1);
    }
}

bool skipPnmData(FILE* f, const PNMInfo& info)
{
    if (info.plain) {
        size_t valueCount = info.width * info.height * info.depth;
        for (size_t i = 0; i < valueCount; i++) {
            int val;
            if (!readWhitespace(f)
                    || fscanf(f, "%d", &val) != 1
                    || val < 0 || val > 65535
                    || (info.maxval <= 255 && val > 255)) {
                return false;
            }
        }
        readWhitespace(f); // ignore EOF
    } else {
        off_t bytes =
              off_t(info.width)
            * off_t(info.height)
            * off_t(info.depth)
            * off_t(info.maxval < 0 ? 4 : info.maxval > 255 ? 2 : 1);
        if (fseeko(f, bytes, SEEK_CUR) < 0) {
            return false;
        }
    }
    return true;
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

    while (hasMore()) {
        off_t arrayPos = ftello(_f);
        if (arrayPos < 0) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
        PNMInfo pnminfo = readPnmHeader(_f);
        if (pnminfo.error != ErrorNone) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
        if (!skipPnmData(_f, pnminfo)) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
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
    PNMInfo pnminfo = readPnmHeader(_f);
    if (pnminfo.error != ErrorNone) {
        *error = pnminfo.error;
        return ArrayContainer();
    }
    Type type = (pnminfo.maxval < 0 ? float32
            : pnminfo.maxval <= 255 ? uint8
            : uint16);
    ArrayContainer r({ size_t(pnminfo.width), size_t(pnminfo.height) },
            size_t(pnminfo.depth), type);
    if (pnminfo.depth <= 2) {
        if (pnminfo.maxval < 0)
            r.componentTagList(0).set("INTERPRETATION", "GRAY");
        else
            r.componentTagList(0).set("INTERPRETATION", "SRGB/LUM");
        if (pnminfo.depth == 2) {
            r.componentTagList(1).set("INTERPRETATION", "ALPHA");
        }
    } else {
        if (pnminfo.maxval < 0) {
            r.componentTagList(0).set("INTERPRETATION", "RED");
            r.componentTagList(1).set("INTERPRETATION", "GREEN");
            r.componentTagList(2).set("INTERPRETATION", "BLUE");
        } else {
            r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
        }
        if (pnminfo.depth == 4) {
            r.componentTagList(3).set("INTERPRETATION", "ALPHA");
        }
    }
    if (!readPnmData(_f, pnminfo, r)) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    if (pnminfo.needsEndianFix) {
        swapEndianness(r);
    }
    if (type == float32) {
        for (size_t e = 0; e < r.elementCount(); e++)
            for (size_t c = 0; c < r.componentCount(); c++)
                r.set<float>(e, c, r.get<float>(e, c) * pnminfo.factor);
    } else {
        reverseY(r);
    }
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
                && !(array.componentType() == float32
                    && (array.componentCount() == 1 || array.componentCount() == 3)))) {
        return ErrorFeaturesUnsupported;
    }

    int width = array.dimension(0);
    int height = array.dimension(1);
    int depth = array.componentCount();
    int maxval =
        (  array.componentType() == uint8 ? std::numeric_limits<uint8_t>::max()
         : array.componentType() == uint16 ? std::numeric_limits<uint16_t>::max()
         : -1);
    std::string header;
    if (array.componentSize() <= 2 && array.componentCount() == 1) {
        header = std::string("P5\n")
            + std::to_string(width) + ' ' + std::to_string(height) + '\n'
            + std::to_string(maxval) + '\n';
    } else if (array.componentSize() <= 2 && array.componentCount() == 3) {
        header = std::string("P6\n")
            + std::to_string(width) + ' ' + std::to_string(height) + '\n'
            + std::to_string(maxval) + '\n';
    } else if (array.componentSize() <= 2) {
        const char* tupltypes[4] = { "GRAYSCALE", "GRAYSCALE_ALPHA", "RGB", "RGB_ALPHA" };
        header = std::string("P7\n")
            + "WIDTH "    + std::to_string(width)  + '\n'
            + "HEIGHT "   + std::to_string(height) + '\n'
            + "DEPTH "    + std::to_string(depth)  + '\n'
            + "MAXVAL "   + std::to_string(maxval) + '\n'
            + "TUPLTYPE " + tupltypes[depth]       + '\n'
            + "ENDHDR\n";
    } else {
        header = std::string("P") + (depth == 1 ? 'f' : 'F') + '\n'
            + std::to_string(width) + ' ' + std::to_string(height) + '\n'
            + "-1.0\n";
    }
    ArrayContainer data;
    if (array.componentType() == float32) {
        data = array;
    } else {
        data = array.deepCopy();
        reverseY(data);
        if (array.componentType() == uint16)
            swapEndianness(data);
    }
    if (fputs(header.c_str(), _f) == EOF || fwrite(data.data(), data.dataSize(), 1, _f) != 1) {
        return ErrorSysErrno;
    }
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_pnm()
{
    return new FormatImportExportPNM();
}

}
