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

#include <setjmp.h>
#include <jpeglib.h>

#include "io-jpeg.hpp"
#include "io-utils.hpp"
#include "io-exiv2.hpp"


namespace TGD {

struct my_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

static void my_error_exit(j_common_ptr cinfo)
{
    struct my_error_mgr* my_err = reinterpret_cast<struct my_error_mgr*>(cinfo->err);
    longjmp(my_err->setjmp_buffer, 1);
}

FormatImportExportJPEG::FormatImportExportJPEG() :
    _f(nullptr), _arrayWasReadOrWritten(false)
{
}

FormatImportExportJPEG::~FormatImportExportJPEG()
{
    close();
}

Error FormatImportExportJPEG::openForReading(const std::string& fileName, const TagList&)
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

Error FormatImportExportJPEG::openForWriting(const std::string& fileName, bool append, const TagList&)
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

void FormatImportExportJPEG::close()
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

int FormatImportExportJPEG::arrayCount()
{
    return (_f ? 1 : -1);
}

ArrayContainer FormatImportExportJPEG::readArray(Error* error, int arrayIndex, const Allocator& alloc)
{
    if (arrayIndex > 0) {
        *error = ErrorSeekingNotSupported;
        return ArrayContainer();
    }
    std::rewind(_f);

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, _f);
    jpeg_read_header(&cinfo, TRUE);

    ArrayContainer r({cinfo.image_width, cinfo.image_height}, cinfo.num_components, uint8, alloc);
    if (cinfo.num_components == 1) {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
    } else {
        r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
        r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
        r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
    }
    ImageOriginLocation originLocation = getImageOriginLocation(_fileName);
#if 0
    // These flags improve performance, but at unclear costs in quality.
    cinfo.do_fancy_upsampling = TRUE;
    cinfo.dct_method = JDCT_FASTEST;
#endif
    jpeg_start_decompress(&cinfo);
    std::vector<JSAMPROW> jrows(cinfo.output_height);
    for (unsigned int i = 0; i < cinfo.output_height; i++) {
        if (originLocation == OriginTopLeft)
            jrows[i] = static_cast<unsigned char*>(r.get((cinfo.output_height - 1 - i) * cinfo.image_width));
        else
            jrows[i] = static_cast<unsigned char*>(r.get(i * cinfo.image_width));
    }
    while (cinfo.output_scanline < cinfo.image_height) {
        jpeg_read_scanlines(&cinfo, &jrows[cinfo.output_scanline],
                cinfo.output_height - cinfo.output_scanline);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    if (originLocation != OriginTopLeft)
        fixImageOrientation(r, originLocation);

    _arrayWasReadOrWritten = true;

    return r;
}

bool FormatImportExportJPEG::hasMore()
{
    return !_arrayWasReadOrWritten;
}

Error FormatImportExportJPEG::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > 0x7fffffffL || array.dimension(1) > 0x7fffffffL
            || array.componentType() != uint8
            || (array.componentCount() != 1 && array.componentCount() != 3)
            || _arrayWasReadOrWritten) {
        return ErrorFeaturesUnsupported;
    }

    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        return ErrorInvalidData;
    }
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, _f);
    cinfo.image_width = array.dimension(0);
    cinfo.image_height = array.dimension(1);
    cinfo.input_components = array.componentCount();
    cinfo.in_color_space = (array.componentCount() == 1 ? JCS_GRAYSCALE : JCS_RGB);
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE);

    jpeg_start_compress(&cinfo, TRUE);
    std::vector<JSAMPROW> jrows(cinfo.image_height);
    for (unsigned int i = 0; i < cinfo.image_height; i++)
        jrows[i] = static_cast<unsigned char*>(const_cast<void*>(array.get((cinfo.image_height - 1 - i) * cinfo.image_width)));
    jpeg_write_scanlines(&cinfo, jrows.data(), cinfo.image_height);
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    if (fflush(_f) != 0) {
        return ErrorSysErrno;
    }
    _arrayWasReadOrWritten = true;
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_jpeg()
{
    return new FormatImportExportJPEG();
}

}
