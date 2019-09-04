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

#ifndef TAD_IO_UTILS_HPP
#define TAD_IO_UTILS_HPP

#include <cstdint>

namespace TAD {

inline void reverseY(size_t height, size_t line_size, unsigned char* data)
{
    std::vector<unsigned char> tmp_line(line_size);
    for (size_t y = 0; y < height / 2; y++) {
        size_t ty = height - 1 - y;
        std::memcpy(&(tmp_line[0]), &(data[ty * line_size]), line_size);
        std::memcpy(&(data[ty * line_size]), &(data[y * line_size]), line_size);
        std::memcpy(&(data[y * line_size]), &(tmp_line[0]), line_size);
    }
}

inline void reverseY(ArrayContainer& array)
{
    reverseY(array.dimension(1),
            array.dimension(0) * array.elementSize(),
            static_cast<unsigned char*>(array.data()));
}

inline void swapEndianness(ArrayContainer& array)
{
    size_t n = array.elementCount() * array.componentCount();
    if (array.componentSize() == 8) {
        uint64_t* data = static_cast<uint64_t*>(array.data());
        for (size_t i = 0; i < n; i++) {
            uint64_t x = data[i];
            x =   ((x                               ) << UINT64_C(56))
                | ((x & UINT64_C(0x000000000000ff00)) << UINT64_C(40))
                | ((x & UINT64_C(0x0000000000ff0000)) << UINT64_C(24))
                | ((x & UINT64_C(0x00000000ff000000)) << UINT64_C(8))
                | ((x & UINT64_C(0x000000ff00000000)) >> UINT64_C(8))
                | ((x & UINT64_C(0x0000ff0000000000)) >> UINT64_C(24))
                | ((x & UINT64_C(0x00ff000000000000)) >> UINT64_C(40))
                | ((x                               ) >> UINT64_C(56));
            data[i] = x;
        }
    } else if (array.componentSize() == 4) {
        uint32_t* data = static_cast<uint32_t*>(array.data());
        for (size_t i = 0; i < n; i++) {
            uint32_t x = data[i];
            x =   ((x                       ) << UINT32_C(24))
                | ((x & UINT32_C(0x0000ff00)) << UINT32_C(8))
                | ((x & UINT32_C(0x00ff0000)) >> UINT32_C(8))
                | ((x                       ) >> UINT32_C(24));
            data[i] = x;
        }
    } else if (array.componentSize() == 2) {
        uint16_t* data = static_cast<uint16_t*>(array.data());
        for (size_t i = 0; i < n; i++) {
            uint16_t x = data[i];
            x = (x << UINT16_C(8)) | (x >> UINT16_C(8));
            data[i] = x;
        }
    }
}

}

#endif
