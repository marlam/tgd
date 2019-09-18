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

#ifndef TAD_IO_TIFF_HPP
#define TAD_IO_TIFF_HPP

#include <tiffio.h>

#include "io.hpp"

namespace TAD {

class FormatImportExportTIFF : public FormatImportExport {
private:
    TIFF* _tiff;
    int _dirCount;
    int _readCount;

public:
    FormatImportExportTIFF();
    ~FormatImportExportTIFF();

    virtual Error openForReading(const std::string& fileName, const TagList& hints) override;
    virtual Error openForWriting(const std::string& fileName, bool append, const TagList& hints) override;
    virtual void close() override;

    // for reading:
    virtual int arrayCount() override;
    virtual ArrayContainer readArray(Error* error, int arrayIndex = -1 /* -1 means next */) override;
    virtual bool hasMore() override;

    // for writing / appending:
    virtual Error writeArray(const ArrayContainer& array) override;
};

extern "C" FormatImportExport* FormatImportExportFactory_tiff();

}
#endif
