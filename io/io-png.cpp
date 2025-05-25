/*
 * Copyright (C) 2018, 2019, 2020, 2021, 2022
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

#include <cstdio>

#include <png.h>

#include "io-png.hpp"
#include "io-utils.hpp"
#include "io-exiv2.hpp"


namespace TGD {

static void my_png_error(png_structp png_ptr, png_const_charp /* error_msg */)
{
    longjmp(png_jmpbuf(png_ptr), 1);
}

static void my_png_warning(png_structp /* png_ptr */, png_const_charp /* warning_msg */)
{
    // ignore warnings
}

FormatImportExportPNG::FormatImportExportPNG() :
    _f(nullptr), _arrayWasReadOrWritten(false)
{
}

FormatImportExportPNG::~FormatImportExportPNG()
{
    close();
}

Error FormatImportExportPNG::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-") {
        _f = stdin;
    } else {
        _f = fopen(fileName.c_str(), "rb");
        if (_f)
            _fileName = fileName;
    }
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportPNG::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (append)
        return ErrorFeaturesUnsupported;
    if (fileName == "-") {
        _f = stdout;
    } else {
        _f = fopen(fileName.c_str(), "wb");
        if (_f)
            _fileName = fileName;
    }
    return _f ? ErrorNone : ErrorSysErrno;
}

void FormatImportExportPNG::close()
{
    if (_f) {
        if (_f != stdin && _f != stdout) {
            fclose(_f);
        }
        _f = nullptr;
    }
    _fileName = std::string();
    _arrayWasReadOrWritten = false;
}

int FormatImportExportPNG::arrayCount()
{
    return (_f ? 1 : -1);
}

ArrayContainer FormatImportExportPNG::readArray(Error* error, int arrayIndex, const Allocator& alloc)
{
    if (arrayIndex > 0) {
        *error = ErrorSeekingNotSupported;
        return ArrayContainer();
    }
    std::rewind(_f);

    png_byte header[8];
    if (std::fread(header, 8, 1, _f) != 1) {
        *error = std::ferror(_f) ? ErrorSysErrno : ErrorInvalidData;
        return ArrayContainer();
    }
    if (png_sig_cmp(header, 0, 8)) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    png_set_user_limits(png_ptr, 0x7fffffffL, 0x7fffffffL);
    png_set_palette_to_rgb(png_ptr);
    png_set_expand_gray_1_2_4_to_8(png_ptr);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    png_set_error_fn(png_ptr, NULL, my_png_error, my_png_warning);
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    png_init_io(png_ptr, _f);
    png_set_sig_bytes(png_ptr, 8);
    // TODO: ??? png_set_gamma(png_ptr, 2.2, 0.45455);
    png_read_png(png_ptr, info_ptr,
            PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_SWAP_ENDIAN,
            NULL);
    unsigned int width = png_get_image_width(png_ptr, info_ptr);
    unsigned int height = png_get_image_height(png_ptr, info_ptr);
    unsigned int channels = png_get_channels(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);
    png_textp text_ptr;
    png_uint_32 num_text = png_get_text(png_ptr, info_ptr, &text_ptr, NULL);

    ArrayContainer r({ width, height }, channels, bit_depth <= 8 ? uint8 : uint16, alloc);
    for (unsigned int i = 0; i < num_text; i++) {
        if (std::strncmp(text_ptr[i].text, "\nexif\n", 6) == 0) {
            // This is EXIF data encoded in a string with control characters.
            // Not useful as a global tag. We could someday instead extract
            // individual EXIF data fields using e.g. libexiv2, but for now
            // that does not seem useful.
            continue;
        }
        r.globalTagList().set(text_ptr[i].key, text_ptr[i].text);
    }
    if (channels == 1) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
    } else if (channels == 2) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
        r.componentTagList(1).set("INTERPRETATION", "ALPHA");
    } else if (channels == 3) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
        r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
        r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
    } else if (channels == 4) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
        r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
        r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
        r.componentTagList(3).set("INTERPRETATION", "ALPHA");
    }
    ImageOriginLocation originLocation = getImageOriginLocation(_fileName);
    for (size_t i = 0; i < height; i++) {
        if (originLocation == OriginTopLeft)
            std::memcpy(r.get(i * width), row_pointers[height - 1 - i], r.elementSize() * width);
        else
            std::memcpy(r.get(i * width), row_pointers[i], r.elementSize() * width);
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    if (originLocation != OriginTopLeft)
        fixImageOrientation(r, originLocation);

    _arrayWasReadOrWritten = true;

    return r;
}

bool FormatImportExportPNG::hasMore()
{
    return !_arrayWasReadOrWritten;
}

Error FormatImportExportPNG::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > 0x7fffffffL || array.dimension(1) > 0x7fffffffL
            || (array.componentType() != uint8 && array.componentType() != uint16)
            || array.componentCount() < 1 || array.componentCount() > 4
            || _arrayWasReadOrWritten) {
        return ErrorFeaturesUnsupported;
    }

    std::vector<png_bytep> row_pointers(array.dimension(1));
    for (size_t i = 0; i < array.dimension(1); i++)
        row_pointers[i] = static_cast<unsigned char*>(const_cast<void*>(array.get((array.dimension(1) - 1 - i) * array.dimension(0))));
    std::vector<struct png_text_struct> text;
    for (auto it = array.globalTagList().cbegin(); it != array.globalTagList().cend(); it++) {
        struct png_text_struct t;
        std::memset(&t, 0, sizeof(t));
        t.compression = -1;
        t.key = const_cast<char*>(it->first.c_str());
        t.text = const_cast<char*>(it->second.c_str());
        t.text_length = it->second.length();
        text.push_back(t);
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        return ErrorLibrary;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        return ErrorLibrary;
    }
    png_set_error_fn(png_ptr, NULL, my_png_error, my_png_warning);
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return ErrorLibrary;
    }
    png_set_user_limits(png_ptr, 0x7fffffffL, 0x7fffffffL);
    png_init_io(png_ptr, _f);
    png_set_IHDR(png_ptr, info_ptr,
            array.dimension(0), array.dimension(1),
            array.componentType() == uint8 ? 8 : 16,
            array.componentCount() == 1 ? PNG_COLOR_TYPE_GRAY
            : array.componentCount() == 2 ? PNG_COLOR_TYPE_GRAY_ALPHA
            : array.componentCount() == 3 ? PNG_COLOR_TYPE_RGB
            : PNG_COLOR_TYPE_RGB_ALPHA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png_ptr, 6);
    png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_ABSOLUTE);
    if (text.size() > 0)
        png_set_text(png_ptr, info_ptr, &(text[0]), text.size());
    png_set_rows(png_ptr, info_ptr, &(row_pointers[0]));
    png_write_png(png_ptr, info_ptr,
            PNG_TRANSFORM_SWAP_ENDIAN,
            NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    if (fflush(_f) != 0) {
        return ErrorSysErrno;
    }
    _arrayWasReadOrWritten = true;
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_png()
{
    return new FormatImportExportPNG();
}

}
