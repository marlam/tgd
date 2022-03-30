/*
 * Copyright (C) 2022 Computer Graphics Group, University of Siegen
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

#include "io-stb.hpp"
#include "io-utils.hpp"

#include <vector>
#include <limits>

#define STBI_NO_PNM
#define STBI_NO_PIC
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace TAD {

FormatImportExportSTB::FormatImportExportSTB() : _f(nullptr), _hasMore(true)
{
    stbi_set_flip_vertically_on_load(1);
}

FormatImportExportSTB::~FormatImportExportSTB()
{
}

Error FormatImportExportSTB::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        _f = stdin;
    else
        _f = fopen(fileName.c_str(), "rb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportSTB::openForWriting(const std::string&, bool, const TagList&)
{
    return ErrorFeaturesUnsupported;
}

void FormatImportExportSTB::close()
{
    if (_f) {
        if (_f != stdin) {
            fclose(_f);
        }
        _f = nullptr;
        _hasMore = true;
    }
}

int FormatImportExportSTB::arrayCount()
{
    return 1;
}

ArrayContainer FormatImportExportSTB::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    int width, height, channels;
    TAD::ArrayContainer r;
    if (stbi_is_16_bit_from_file(_f)) {
        stbi_us* data = stbi_load_from_file_16(_f, &width, &height, &channels, 0);
        if (!data) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        r = TAD::ArrayContainer( { size_t(width), size_t(height) }, channels, uint16);
        std::memcpy(r.data(), data, r.dataSize());
        stbi_image_free(data);
    } else {
        stbi_uc* data = stbi_load_from_file(_f, &width, &height, &channels, 0);
        if (!data) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        r = TAD::ArrayContainer( { size_t(width), size_t(height) }, channels, uint8);
        std::memcpy(r.data(), data, r.dataSize());
        stbi_image_free(data);
    }

    if (r.componentCount() == 1) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
    } else if (r.componentCount() == 2) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
        r.componentTagList(1).set("INTERPRETATION", "ALPHA");
    } else if (r.componentCount() == 3) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
        r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
        r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
    } else {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
        r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
        r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
        r.componentTagList(3).set("INTERPRETATION", "ALPHA");
    }

    _hasMore = false;
    return r;
}

bool FormatImportExportSTB::hasMore()
{
    return _hasMore;
}

Error FormatImportExportSTB::writeArray(const ArrayContainer&)
{
    return ErrorFeaturesUnsupported;
}

extern "C" FormatImportExport* FormatImportExportFactory_stb()
{
    return new FormatImportExportSTB();
}

}
