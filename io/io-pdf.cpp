/*
 * Copyright (C) 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
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
#include <unistd.h>

#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-document.h>

#include "io-pdf.hpp"


namespace TGD {

static void popplerDebugOutput(const std::string&, void*)
{
    // just shut up
}

FormatImportExportPDF::FormatImportExportPDF() :
    _dpi(0.0f), _renderer(nullptr), _doc(nullptr), _lastReadPage(-1),
    _outFile(nullptr), _outArrayCount(0), _outLengthWithoutFooter(0)
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

static std::string toString(time_t time, bool forHumans)
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
        if (forHumans) {
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", tmPtr);
        } else { // for pdf
            strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", tmPtr);
        }
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
    _creationDate = toString(_doc->get_creation_date_t(), true);
    _modificationDate = toString(_doc->get_modification_date_t(), true);

    _renderer = new poppler::page_renderer();
    _renderer->set_image_format(poppler::image::format_rgb24);
    _renderer->set_render_hints(
              poppler::page_renderer::antialiasing
            | poppler::page_renderer::text_antialiasing
            | poppler::page_renderer::text_hinting);

    return ErrorNone;
}

Error FormatImportExportPDF::openForWriting(const std::string& fileName, bool append, const TagList& hints)
{
    if (append)
        return ErrorFeaturesUnsupported;
    _outFile = fopen(fileName.c_str(), "w+b");
    if (!_outFile)
        return ErrorSysErrno;

    _dpi = hints.value("DPI", 300.0f);
    _outCreationDate = toString(time(nullptr), false);
    return ErrorNone;
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
    if (_outFile) {
        fclose(_outFile);
        _outFile = nullptr;
    }
    _outCreationDate = std::string();
    _outArrayCount = 0;
    _outLengthWithoutFooter = 0;
    _arrayDataObjStart.clear();
    _arrayPageObjStart.clear();
    _arrayStuff0ObjStart.clear();
    _arrayStuff1ObjStart.clear();
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

Error FormatImportExportPDF::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) < 1 || array.dimension(0) > 65535
            || array.dimension(1) < 1 || array.dimension(1) > 65535
            || (array.componentType() != TGD::uint8 && array.componentType() != TGD::uint16)
            || (array.componentCount() != 1 && array.componentCount() != 3))
        return ErrorFeaturesUnsupported;

    // three objects describe the PDF:
    long long int metaObjIndex = 1;
    long long int catalogObjIndex = metaObjIndex + 1;
    long long int pagesObjIndex = catalogObjIndex + 1;
    // four objects per array:
    long long int perArrayObjIndexStart = pagesObjIndex + 1;
    long long int currentArrayDataObjIndex = perArrayObjIndexStart + 4 * _outArrayCount + 0;
    long long int currentArrayPageObjIndex = perArrayObjIndexStart + 4 * _outArrayCount + 1;
    long long int currentArrayStuff0ObjIndex = perArrayObjIndexStart + 4 * _outArrayCount + 2;
    long long int currentArrayStuff1ObjIndex = perArrayObjIndexStart + 4 * _outArrayCount + 3;

    // write header if necessary
    if (_outLengthWithoutFooter == 0) {
        fputs("%PDF-1.4\n", _outFile);
        _outLengthWithoutFooter = 9;
    }

    // remove any previous footer
    if (ftruncate(fileno(_outFile), _outLengthWithoutFooter) != 0)
        return ErrorSysErrno;
    if (fseeko(_outFile, 0, SEEK_END) != 0)
        return ErrorSysErrno;

    // write the four objects for this array
    int w = array.dimension(0);
    int h = array.dimension(1);
    _arrayDataObjStart.push_back(ftello(_outFile));
    fprintf(_outFile,
            "%lld 0 obj\n"
            "<< /Type /XObject /BitsPerComponent %d /ColorSpace [ /Device%s ] /Subtype /Image /Height %d /Width %d /Length %lld >>stream\n",
            currentArrayDataObjIndex,
            int(typeSize(array.componentType()) * 8),
            array.componentCount() == 1 ? "Gray" : "RGB",
            h, w, static_cast<long long int>(array.dataSize()));
    size_t lineSize = array.elementSize() * w;
    if (array.componentType() == TGD::uint8) {
        for (int i = 0; i < h; i++) {
            fwrite(reinterpret_cast<const unsigned char*>(array.data()) + (h - 1 - i) * lineSize, lineSize, 1, _outFile);
        }
    } else {
        std::vector<unsigned char> lineBuf(lineSize);
        for (int i = 0; i < h; i++) {
            memcpy(lineBuf.data(), reinterpret_cast<const unsigned char*>(array.data()) + (h - 1 - i) * lineSize, lineSize);
            // swap endianness
            {
                uint16_t* data = reinterpret_cast<uint16_t*>(lineBuf.data());
                for (size_t i = 0; i < lineSize / 2; i++) {
                    uint16_t x = data[i];
                    x = (x << UINT16_C(8)) | (x >> UINT16_C(8));
                    data[i] = x;
                }
            }
            fwrite(lineBuf.data(), lineSize, 1, _outFile);
        }
    }
    fputs("\nendstream\n"
            "endobj\n", _outFile);
    _arrayPageObjStart.push_back(ftello(_outFile));
    float mediaBoxWidth = 72.0f * w / _dpi;
    float mediaBoxHeight = 72.0f * h / _dpi;
    fprintf(_outFile,
            "%lld 0 obj\n"
            "<< /Type /Page /MediaBox [ 0 0 %.5f %.5f ] /Contents %lld 0 R /Resources << /XObject << /strip0 %lld 0 R >> >> /Parent %lld 0 R >>\n"
            "endobj\n",
            currentArrayPageObjIndex,
            mediaBoxWidth, mediaBoxHeight,
            currentArrayStuff0ObjIndex,
            currentArrayDataObjIndex,
            pagesObjIndex);
    _arrayStuff0ObjStart.push_back(ftello(_outFile));
    fprintf(_outFile,
            "%lld 0 obj\n"
            "<< /Length %lld 0 R >>stream\n",
            currentArrayStuff0ObjIndex, currentArrayStuff1ObjIndex);
    int stuff0Len = fprintf(_outFile, "q %.5f 0 0 %.5f 0 0 cm /strip0 Do Q\n", mediaBoxWidth, mediaBoxHeight);
    fputs("endstream\n"
            "endobj\n", _outFile);
    _arrayStuff1ObjStart.push_back(ftello(_outFile));
    fprintf(_outFile,
            "%lld 0 obj\n"
            "%d\n"
            "endobj\n",
            currentArrayStuff1ObjIndex, stuff0Len);
    _outLengthWithoutFooter = ftello(_outFile);
    _outArrayCount++;

    // write a new footer
    std::string outModDate = toString(time(nullptr), false);
    long long int metaObjStart = _outLengthWithoutFooter;
    fprintf(_outFile,
            "%lld 0 obj\n"
            "<< /CreationDate (D:%s) /ModDate (D:%s) /Producer (tgd - https://marlam.de/tgd)>>\n"
            "endobj\n",
            metaObjIndex, _outCreationDate.c_str(), outModDate.c_str());
    long long int catalogObjStart = ftello(_outFile);
    fprintf(_outFile,
            "%lld 0 obj\n"
            "<< /Type /Catalog /Pages %lld 0 R >>\n"
            "endobj\n",
            catalogObjIndex, pagesObjIndex);
    long long int pagesObjStart = ftello(_outFile);
    fprintf(_outFile,
            "%lld 0 obj\n"
            "<< /Type /Pages /Count %lld\n"
            "   /Kids [",
            pagesObjIndex, _outArrayCount);
    for (long long int i = 0; i < _outArrayCount; i++) {
        fprintf(_outFile, " %lld 0 R", perArrayObjIndexStart + 4 * i + 1);
    }
    fputs(" ]\n"
            ">>\n"
            "endobj\n", _outFile);
    long long int objectCount = currentArrayDataObjIndex + 4;
    long long int xrefStart = ftello(_outFile);
    fprintf(_outFile,
            "xref\n"
            "0 %lld\n"
            "0000000000 65535 f\n"
            "%010lld 00000 n\n"
            "%010lld 00000 n\n"
            "%010lld 00000 n\n",
            objectCount, metaObjStart, catalogObjStart, pagesObjStart);
    for (long long int i = 0; i < _outArrayCount; i++) {
        fprintf(_outFile,
                "%010lld 00000 n\n"
                "%010lld 00000 n\n"
                "%010lld 00000 n\n"
                "%010lld 00000 n\n",
                _arrayDataObjStart[i],
                _arrayPageObjStart[i],
                _arrayStuff0ObjStart[i],
                _arrayStuff1ObjStart[i]);
    }
    fprintf(_outFile,
            "trailer\n"
            "<< /Size %lld /Root %lld 0 R /Info %lld 0 R >>\n"
            "startxref\n"
            "%lld\n"
            "%%EOF\n",
            objectCount, catalogObjIndex, metaObjIndex, xrefStart);

    // error handling
    if (fflush(_outFile) != 0) {
        _outArrayCount--;
        _arrayDataObjStart.pop_back();
        _arrayPageObjStart.pop_back();
        _arrayStuff0ObjStart.pop_back();
        _arrayStuff1ObjStart.pop_back();
        return ErrorSysErrno;
    }

    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_pdf()
{
    return new FormatImportExportPDF();
}

}
