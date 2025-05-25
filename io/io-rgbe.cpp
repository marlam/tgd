/*
 * Copyright (C) 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 * Copyright (C) 2023, 2024, 2025
 * Martin Lambers <marlam@marlam.de>
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

#include <algorithm>

#include <cstdio>
#include <cmath>
#include <cstring>

#include "io-rgbe.hpp"


namespace TGD {

FormatImportExportRGBE::FormatImportExportRGBE() :
    _f(nullptr),
    _arrayCount(-2)
{
}

FormatImportExportRGBE::~FormatImportExportRGBE()
{
    close();
}

Error FormatImportExportRGBE::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        _f = stdin;
    else
        _f = fopen(fileName.c_str(), "rb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportRGBE::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (fileName == "-")
        _f = stdout;
    else
        _f = fopen(fileName.c_str(), append ? "ab" : "wb");
    return _f ? ErrorNone : ErrorSysErrno;
}

void FormatImportExportRGBE::close()
{
    if (_f) {
        if (_f != stdin && _f != stdout) {
            fclose(_f);
        }
        _f = nullptr;
    }
}

Error FormatImportExportRGBE::readRGBEHeader(int& width, int& height, float& exposure)
{
    char buf[256];

    bool haveFormat = false;
    int w = -1;
    int h = -1;
    float e = 1.0f;

    while (std::fgets(buf, sizeof(buf), _f)) {
        if (!haveFormat) {
            if (std::strcmp(buf, "#?RADIANCE\n") != 0 && std::strcmp(buf, "#?RGBE\n") != 0) {
                return ErrorInvalidData;
            }
            haveFormat = true;
            continue;
        }
        if (buf[0] == '#') // comment
            continue;
        if (buf[0] == '\n') // end of header
            break;
        if (std::strcmp(buf, "FORMAT=32-bit_rle_rgbe\n") == 0)
            continue;
        // TODO: support 32-bit_rle_xyze?
        if (std::sscanf(buf, "EXPOSURE=%f", &e) == 1)
            continue;
        // There are a few more possible header lines; ignore them
    }
    if (!std::fgets(buf, sizeof(buf), _f)
            || std::sscanf(buf, "-Y %d +X %d", &h, &w) != 2
            || w < 1 || h < 1) {
        return ErrorInvalidData;
    }

    width = w;
    height = h;
    exposure = e;
    return ErrorNone;
}

static void rgbeToRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char e, float exposure, float* RGB)
{
    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;
    if (e != 0) {
        float v = std::ldexp(1.0f, int(e) - (128 + 8)) / exposure;
        R = (r + 0.5f) * v;
        G = (g + 0.5f) * v;
        B = (b + 0.5f) * v;
    }
    RGB[0] = R;
    RGB[1] = G;
    RGB[2] = B;
}

Error FormatImportExportRGBE::readRGBEData(Array<float>& a, float exposure)
{
    std::vector<unsigned char> line(a.dimension(0) * 4);

    for (size_t y = 0; y < a.dimension(1); y++) {
        size_t aY = a.dimension(1) - 1 - y;
        if (std::fread(line.data(), 4, 1, _f) != 1) {
            return std::ferror(_f) ? ErrorSysErrno : ErrorInvalidData;
        }
        if (line[0] != 2 || line[1] != 2 || (line[2] << 8) + line[3] != int(a.dimension(0))) {
            // plain format, not RLE: read rest of line
            if (std::fread(line.data() + 4, 4, a.dimension(0) - 1, _f) != a.dimension(0) - 1) {
                return std::ferror(_f) ? ErrorSysErrno : ErrorInvalidData;
            }
            for (size_t x = 0; x < a.dimension(0); x++) {
                rgbeToRGB(line[4 * x + 0],
                          line[4 * x + 1],
                          line[4 * x + 2],
                          line[4 * x + 3],
                          exposure, a[{x, aY}]);
            }
        } else {
            // RLE
            for (int i = 0; i < 4; i++) { // components are separated
                unsigned char* data = line.data() + i * a.dimension(0);
                size_t pos = 0;
                while (pos < a.dimension(0)) {
                    unsigned char p[2];
                    if (std::fread(p, sizeof(p), 1, _f) != 1) {
                        return std::ferror(_f) ? ErrorSysErrno : ErrorInvalidData;
                    }
                    if (p[0] > 128) {
                        size_t rleLen = p[0] - 128;
                        if (pos + rleLen > a.dimension(0)) {
                            return ErrorInvalidData;
                        }
                        for (size_t j = 0; j < rleLen; j++)
                            data[pos++] = p[1];
                    } else {
                        data[pos++] = p[1];
                        if (p[0] > 1) {
                            size_t plainLen = p[0] - 1;
                            if (pos + plainLen > a.dimension(0)) {
                                return ErrorInvalidData;
                            }
                            if (std::fread(data + pos, 1, plainLen, _f) != plainLen) {
                                return std::ferror(_f) ? ErrorSysErrno : ErrorInvalidData;
                            }
                            pos += plainLen;
                        }
                    }
                }
            }
            for (size_t x = 0; x < a.dimension(0); x++) {
                rgbeToRGB(line[0 * a.dimension(0) + x],
                          line[1 * a.dimension(0) + x],
                          line[2 * a.dimension(0) + x],
                          line[3 * a.dimension(0) + x],
                          exposure, a[{x, aY}]);
            }
        }
    }
    return ErrorNone;
}

ArrayContainer FormatImportExportRGBE::readRGBE(Error& e, const Allocator& alloc)
{
    int width, height;
    float exposure;

    e = readRGBEHeader(width, height, exposure);
    if (e != ErrorNone)
        return ArrayContainer();
    Array<float> a({ size_t(width), size_t(height) }, 3, alloc);
    a.componentTagList(0).set("INTERPRETATION", "RED");
    a.componentTagList(1).set("INTERPRETATION", "GREEN");
    a.componentTagList(2).set("INTERPRETATION", "BLUE");
    e = readRGBEData(a, exposure);
    if (e != ErrorNone)
        return ArrayContainer();
    return a;
}

int FormatImportExportRGBE::arrayCount()
{
    if (_arrayCount >= -1)
        return _arrayCount;

    if (!_f)
        return -1;

    // find offsets of all RGBEs in the file
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
        Error e;
        ArrayContainer a = readRGBE(e, Allocator()); // TODO XXX: Allow custom allocator here. How?
        if (e != ErrorNone) {
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

ArrayContainer FormatImportExportRGBE::readArray(Error* error, int arrayIndex, const Allocator& alloc)
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

    // Read the RGBE
    return readRGBE(*error, alloc);
}

bool FormatImportExportRGBE::hasMore()
{
    int c = fgetc(_f);
    if (c == EOF) {
        return false;
    } else {
        ungetc(c, _f);
        return true;
    }
}

static void rgbToRGBE(const float* RGB, unsigned char* rgbe)
{
    float v = std::max(RGB[0], std::max(RGB[1], RGB[2]));
    if (v <= 1e-32f) {
        rgbe[0] = 0;
        rgbe[1] = 0;
        rgbe[2] = 0;
        rgbe[3] = 0;
    } else {
        int e;
        std::frexp(v, &e);
        v = std::ldexp(1.0f, -e + 8);
        rgbe[0] = v * RGB[0];
        rgbe[1] = v * RGB[1];
        rgbe[2] = v * RGB[2];
        rgbe[3] = e + 128;
    }
}

Error FormatImportExportRGBE::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > 65535 || array.dimension(1) > 65535
            || array.componentCount() != 3
            || array.componentType() != float32) {
        return ErrorFeaturesUnsupported;
    }

    // Header
    std::fprintf(_f, "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y %d +X %d\n",
            int(array.dimension(1)), int(array.dimension(0)));
    // Data
    std::vector<unsigned char> line(array.dimension(0) * 4);
    for (size_t y = 0; y < array.dimension(1); y++) {
        const float* rgbLine = array.get<float>({ 0, array.dimension(1) - 1 - y });
        for (size_t x = 0; x < array.dimension(0); x++) {
            rgbToRGBE(rgbLine + 3 * x, line.data() + 4 * x);
        }
        std::fwrite(line.data(), line.size(), 1, _f);
    }
    if (std::ferror(_f) || std::fflush(_f) != 0) {
        return ErrorSysErrno;
    }
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_rgbe()
{
    return new FormatImportExportRGBE();
}

}
