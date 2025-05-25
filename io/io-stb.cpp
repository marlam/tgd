/*
 * Copyright (C) 2022 Computer Graphics Group, University of Siegen
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

#include "io-stb.hpp"
#include "io-utils.hpp"

#include <vector>
#include <limits>

#define STBI_NO_PNM
#define STBI_NO_PIC
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb_image.h"

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../ext/stb_image_write.h"


namespace TGD {

FormatImportExportSTB::FormatImportExportSTB() : _f(nullptr), _hasMore(true)
{
    stbi_set_flip_vertically_on_load(1);
    stbi_flip_vertically_on_write(1);
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

Error FormatImportExportSTB::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (append)
        return ErrorFeaturesUnsupported;
    if (fileName == "-") {
        _f = stdout;
        _extension = "png";
    } else {
        _f = fopen(fileName.c_str(), "wb");
        _extension = getExtension(fileName);
        if (_extension == "jpeg")
            _extension = "jpg";
    }
    return _f ? ErrorNone : ErrorSysErrno;
}

void FormatImportExportSTB::close()
{
    if (_f) {
        if (_f != stdin && _f != stdout) {
            fclose(_f);
        }
        _f = nullptr;
        _hasMore = true;
        _extension.clear();
    }
}

int FormatImportExportSTB::arrayCount()
{
    return 1;
}

ArrayContainer FormatImportExportSTB::readArray(Error* error, int arrayIndex, const Allocator& alloc)
{
    if (arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    int width, height, channels;
    TGD::ArrayContainer r;
    if (stbi_is_16_bit_from_file(_f)) {
        stbi_us* data = stbi_load_from_file_16(_f, &width, &height, &channels, 0);
        if (!data) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        r = TGD::ArrayContainer( { size_t(width), size_t(height) }, channels, uint16, alloc);
        std::memcpy(r.data(), data, r.dataSize());
        stbi_image_free(data);
    } else {
        stbi_uc* data = stbi_load_from_file(_f, &width, &height, &channels, 0);
        if (!data) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        r = TGD::ArrayContainer( { size_t(width), size_t(height) }, channels, uint8, alloc);
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

static void writeFunc(void* context, void* data, int size)
{
    FILE* f = static_cast<FILE*>(context);
    fwrite(data, size, 1, f);
}

Error FormatImportExportSTB::writeArray(const ArrayContainer& array)
{
    if (array.componentCount() < 1 || array.componentCount() > 4
            || array.dimensionCount() != 2
            || array.dimension(0) < 1 || array.dimension(0) > 65535
            || array.dimension(1) < 1 || array.dimension(1) > 65535
            || array.componentType() != uint8) {
        return ErrorFeaturesUnsupported;
    }

    int err = 0;
    if (_extension == "png") {
        err = stbi_write_png_to_func(writeFunc, _f, array.dimension(0), array.dimension(1), array.componentCount(), array.data(), 0);
    } else if (_extension == "bmp") {
        err = stbi_write_bmp_to_func(writeFunc, _f, array.dimension(0), array.dimension(1), array.componentCount(), array.data());
    } else if (_extension == "tga") {
        err = stbi_write_tga_to_func(writeFunc, _f, array.dimension(0), array.dimension(1), array.componentCount(), array.data());
    } else if (_extension == "jpg") {
        err = stbi_write_jpg_to_func(writeFunc, _f, array.dimension(0), array.dimension(1), array.componentCount(), array.data(), 85);
    } else {
        return ErrorFeaturesUnsupported;
    }
    if (err == 0) {
        return ErrorLibrary;
    }

    if (ferror(_f) || fflush(_f) != 0) {
        return ErrorSysErrno;
    }
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_stb()
{
    return new FormatImportExportSTB();
}

}
