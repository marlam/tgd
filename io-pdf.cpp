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
#include <ctime>

#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-document.h>

#include "io-pdf.hpp"


namespace TAD {

static void popplerDebugOutput(const std::string&, void*)
{
    // just shut up
}

FormatImportExportPDF::FormatImportExportPDF() :
    _dpi(0.0f), _renderer(nullptr), _doc(nullptr), _lastReadPage(-1)
{
    poppler::set_debug_error_function(popplerDebugOutput, nullptr);
}

FormatImportExportPDF::~FormatImportExportPDF()
{
    close();
}

static std::string toString(const poppler::ustring& ustr)
{
    std::vector<char> charArray = ustr.to_utf8();
    return std::string(charArray.data(), charArray.size());
}

static std::string toString(unsigned int time)
{
    if (time == 0) {
        return std::string();
    } else {
        char buf[64];
        time_t t = time;
#if defined(_WIN32) || defined(_WIN64)
        struct tm *tmPtr = gmtime(&t);
#else
        struct tm tmBuf;
        struct tm *tmPtr = &tmBuf;
        gmtime_r(&t, tmPtr);
#endif
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", tmPtr);
        return buf;
    }
}

Error FormatImportExportPDF::openForReading(const std::string& fileName, const TagList& hints)
{
    if (fileName == "-")
        return ErrorInvalidData;
    FILE* f = fopen(fileName.c_str(), "rb");
    if (!f)
        return ErrorSysErrno;
    fclose(f);
    _doc = poppler::document::load_from_file(fileName);
    if (!_doc)
        return ErrorInvalidData;

    _dpi = hints.value("DPI", 300.0f);

    _author = toString(_doc->get_author());
    _creator = toString(_doc->get_creator());
    _producer = toString(_doc->get_producer());
    _subject = toString(_doc->get_subject());
    _title = toString(_doc->get_title());
    _creationDate = toString(_doc->get_creation_date());
    _modificationDate = toString(_doc->get_modification_date());

    _renderer = new poppler::page_renderer();
    _renderer->set_image_format(poppler::image::format_rgb24);
    _renderer->set_render_hints(
              poppler::page_renderer::antialiasing
            | poppler::page_renderer::text_antialiasing
            | poppler::page_renderer::text_hinting);

    return ErrorNone;
}

Error FormatImportExportPDF::openForWriting(const std::string&, bool, const TagList&)
{
    return ErrorFeaturesUnsupported;
}

void FormatImportExportPDF::close()
{
    delete _renderer;
    _renderer = nullptr;
    delete _doc;
    _doc = nullptr;
    _author = std::string();
    _creator = std::string();
    _producer = std::string();
    _subject = std::string();
    _title = std::string();
    _creationDate = std::string();
    _modificationDate = std::string();
    _lastReadPage = -1;
}

int FormatImportExportPDF::arrayCount()
{
    return _doc->pages();
}

ArrayContainer FormatImportExportPDF::readArray(Error* error, int arrayIndex)
{
    int pageIndex = 0;
    if (arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    } else if (arrayIndex < 0) {
        pageIndex = _lastReadPage + 1;
    } else {
        pageIndex = arrayIndex;
    }

    poppler::page* page = _doc->create_page(pageIndex);
    poppler::page::orientation_enum orientation = page->orientation();
    poppler::rotation_enum rotation = poppler::rotate_0;
    switch (orientation) {
    case poppler::page::landscape:
        rotation = poppler::rotate_270;
        break;
    case poppler::page::portrait:
        rotation = poppler::rotate_0;
        break;
    case poppler::page::seascape:
        rotation = poppler::rotate_90;
        break;
    case poppler::page::upside_down:
        rotation = poppler::rotate_180;
        break;
    }
    poppler::image img = _renderer->render_page(page, _dpi, _dpi, -1, -1, -1, -1, rotation);
    if (img.width() < 1 || img.height() < 1) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    Array<uint8_t> r({ size_t(img.width()), size_t(img.height()) }, 3);
    if (_author.length() > 0)
        r.globalTagList().set("AUTHOR", _author);
    if (_creator.length() > 0)
        r.globalTagList().set("CREATOR", _creator);
    if (_producer.length() > 0)
        r.globalTagList().set("PRODUCER", _producer);
    if (_subject.length() > 0)
        r.globalTagList().set("SUBJECT", _subject);
    if (_title.length() > 0)
        r.globalTagList().set("TITLE", _title);
    if (_creationDate.length() > 0)
        r.globalTagList().set("CREATION_DATE", _creationDate);
    if (_modificationDate.length() > 0)
        r.globalTagList().set("MODIFICATION_DATE", _modificationDate);
    r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
    r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
    r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
    for (size_t y = 0; y < r.dimension(1); y++) {
        const char* line = img.const_data() + (r.dimension(1) - 1 - y) * img.bytes_per_row();
        std::memcpy(r.get<uint8_t>({ 0, y }), line, r.dimension(0) * r.elementSize());
    }

    _lastReadPage = pageIndex;
    return r;
}

bool FormatImportExportPDF::hasMore()
{
    return _lastReadPage < arrayCount() - 1;
}

Error FormatImportExportPDF::writeArray(const ArrayContainer&)
{
        return ErrorFeaturesUnsupported;
}

extern "C" FormatImportExport* FormatImportExportFactory_pdf()
{
    return new FormatImportExportPDF();
}

}
