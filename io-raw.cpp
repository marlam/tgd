/*
 * Copyright (C) 2018, 2019 Computer Graphics Group, University of Siegen
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "io-raw.hpp"


namespace TAD {

FormatImportExportRAW::FormatImportExportRAW() :
    _template(),
    _f(nullptr),
    _arrayCount(-1)
{
}

FormatImportExportRAW::~FormatImportExportRAW()
{
    close();
}

Error FormatImportExportRAW::openForReading(const std::string& fileName, const TagList& hints)
{
    // The following attributes define an array. Since they are not stored in raw binary files,
    // we need to get them from the hints.
    std::vector<size_t> dimensions;
    size_t components;
    Type type;

    // Dimensions:
    size_t dims;
    size_t dim0, dim1, dim2;
    if (hints.value("DIMENSIONS", &dims)) {
        for (size_t i = 0; i < dims; i++) {
            size_t d;
            if (!hints.value(std::string("DIMENSION") + std::to_string(i), &d))
                return ErrorMissingHints;
            dimensions.push_back(d);
        }
    } else if (hints.value("WIDTH", &dim0) && hints.value("HEIGHT", &dim1)) {
        dimensions.push_back(dim0);
        dimensions.push_back(dim1);
        if (hints.value("DEPTH", &dim2))
            dimensions.push_back(dim2);
    } else if (hints.value("SIZE", &dim0)) {
        dimensions.push_back(dim0);
    } else {
        return ErrorMissingHints;
    }
    for (size_t i = 0; i < dimensions.size(); i++) {
        if (dimensions[i] == 0)
            return ErrorInvalidData;
    }
    // Components:
    components = hints.value("COMPONENTS", 1);
    // Type:
    if (!hints.contains("TYPE"))
        return ErrorMissingHints;
    else if (!typeFromString(hints.value("TYPE"), &type))
        return ErrorInvalidData;

    _template = ArrayDescription(dimensions, components, type);

    // We have the metadata, now try and open the file
    if (fileName == "-")
        _f = stdin;
    else
        _f = fopen(fileName.c_str(), "rb");
    if (_f) {
        struct stat statbuf;
        if (fstat(fileno(_f), &statbuf) != 0) {
            _arrayCount = -1;
        } else {
            _arrayCount = statbuf.st_size / _template.dataSize();
        }
    }
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportRAW::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (fileName == "-")
        _f = stdout;
    else
        _f = fopen(fileName.c_str(), append ? "ab" : "wb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportRAW::close()
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

int FormatImportExportRAW::arrayCount()
{
    return _arrayCount;
}

ArrayContainer FormatImportExportRAW::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex >= 0) {
        if (fseeko(_f, arrayIndex * _template.dataSize(), SEEK_SET) != 0) {
            *error = ErrorSysErrno;
            return ArrayContainer();
        }
    }
    ArrayContainer r(_template);
    if (fread(r.data(), r.dataSize(), 1, _f) != 1) {
        *error = ErrorSysErrno;
        return ArrayContainer();
    }
    return r;
}

bool FormatImportExportRAW::hasMore()
{
    int c = fgetc(_f);
    if (c == EOF) {
        return false;
    } else {
        ungetc(c, _f);
        return true;
    }
}

Error FormatImportExportRAW::writeArray(const ArrayContainer& array)
{
    if (fwrite(array.data(), array.dataSize(), 1, _f) != 1)
        return ErrorSysErrno;
    return ErrorNone;
}

}
