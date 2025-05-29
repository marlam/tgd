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

#ifndef TGD_IO_TGD_HPP
#define TGD_IO_TGD_HPP

/* 
 * TGD file format specification
 *
 * All multi-byte values are stored little-endian.
 * Integers are two's complement.
 * Strings are zero-terminated and UTF-8 encoded.
 *
 * Tag list:
 * - 1 uint64: length of the following data in bytes (N)
 * - string pairs (key/value) until the N bytes are consumed
 *
 * TGD file header:
 * - 3 bytes: 'T', 'G', 'D' (84, 71, 68)
 * - 1 byte: format version, must be 0
 * - 1 byte: component type:
 *   `int8` = 0, `uint8` = 1, `int16` = 2, `uint16` = 3, `int32` = 4, `uint32` =
 *   5, `int64` = 6, `uint64` = 7, `float32` = 8, `float64` = 9
 * - 1 uint64: number of components (C)
 * - 1 uint64: number of dimensions (D)
 * - D uint64: size in each dimension
 * - 1 global tag list
 * - C component tag lists
 * - D dimension tag lists
 *
 * TGD file data:
 * If the global tag list contains the tag "DATAFILE", then the data is assumed to
 * be stored in a file with the file name specified by the value of this tag.
 * Otherwise, the data follows directly after the header.
 * In both cases, the data is packed (no fill bytes).
 */

#include <cstdio>

#include "io.hpp"

namespace TGD {

class FormatImportExportTGD : public FormatImportExport {
private:
    std::string _directory;
    FILE* _f;
    int _arrayCount;
    std::vector<off_t> _arrayOffsets;

public:
    FormatImportExportTGD();
    ~FormatImportExportTGD();

    virtual Error openForReading(const std::string& fileName, const TagList& hints) override;
    virtual Error openForWriting(const std::string& fileName, bool append, const TagList& hints) override;
    virtual void close() override;

    // for reading:
    virtual int arrayCount() override;
    virtual ArrayContainer readArray(Error* error, int arrayIndex, const Allocator& alloc) override;
    virtual bool hasMore() override;

    // for writing / appending:
    virtual Error writeArray(const ArrayContainer& array) override;
};

}
#endif
