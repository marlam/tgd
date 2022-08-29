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

#ifndef TGD_IO_UTILS_HPP
#define TGD_IO_UTILS_HPP

#include <cstdint>

#include "array.hpp"

namespace TGD {

inline std::string getExtension(const std::string& fileName)
{
    std::string extension;
    size_t lastDot = fileName.find_last_of('.');
    if (lastDot != std::string::npos) {
        extension = fileName.substr(lastDot + 1);
        for (size_t i = 0; i < extension.size(); i++)
            extension[i] = std::tolower(extension[i]);
    }
    return extension;
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

inline ArrayContainer transpose(const ArrayContainer& a)
{
    std::vector<size_t> vi = a.dimensions();
    for (size_t i = 0; i < vi.size() / 2; i++) {
        size_t tmp = vi[i];
        vi[i] = vi[vi.size() - 1 - i];
        vi[vi.size() - 1 - i] = tmp;
    }
    ArrayContainer r(vi, a.componentCount(), a.componentType());
    std::vector<size_t> original(a.dimensionCount());
    for (size_t i = 0; i < a.elementCount(); i++) {
        r.toVectorIndex(i, vi.data());
        for (size_t i = 0; i < vi.size() / 2; i++) {
            size_t tmp = vi[i];
            vi[i] = vi[vi.size() - 1 - i];
            vi[vi.size() - 1 - i] = tmp;
        }
        std::memcpy(r.get(i), a.get(vi), a.elementSize());
    }
    return r;
}

inline ArrayContainer reorderMatlabInputData(const std::vector<size_t>& dims, Type t, const void *data)
{
    ArrayContainer r;
    if (dims.size() > 2 && dims[dims.size() - 1] <= 4) {
        // heuristic: the first dim is probably a component count
        std::vector<size_t> dataIndex(dims.size());
        ArrayDescription dataArray(dims, 1, t);
        std::vector<size_t> rIndex(dims.size() - 1);
        for (size_t i = 0; i < rIndex.size(); i++)
            rIndex[i] = dims[dims.size() - 2 - i];
        r = ArrayContainer(rIndex, dims[dims.size() - 1], t);
        for (size_t i = 0; i < r.elementCount(); i++) {
            r.toVectorIndex(i, rIndex.data());
            for (size_t d = 0; d < rIndex.size(); d++)
                dataIndex[d] = rIndex[rIndex.size() - 1 - d];
            if (r.dimensionCount() == 2) // flip images in y
                dataIndex[0] = r.dimension(1) - 1 - dataIndex[0];
            for (size_t c = 0; c < r.componentCount(); c++) {
                dataIndex[2] = c;
                std::memcpy(static_cast<unsigned char*>(r.data()) + r.componentOffset(i, c),
                        static_cast<const unsigned char*>(data) + dataArray.elementOffset(dataIndex),
                        r.componentSize());
            }
        }
    } else {
        r = ArrayContainer(dims, 1, t);
        std::memcpy(r.data(), data, r.dataSize());
        r = transpose(r);
    }
    return r;
}

inline ArrayContainer reorderMatlabOutputData(const ArrayContainer& array)
{
    std::vector<size_t> dataDims(array.dimensionCount() + 1);
    for (size_t i = 0; i < array.dimensionCount(); i++)
        dataDims[i] = array.dimension(array.dimensionCount() - 1 - i);
    dataDims[array.dimensionCount()] = array.componentCount();
    ArrayContainer dataArray(dataDims, 1, array.componentType());
    std::vector<size_t> dataIndex(dataArray.dimensionCount());
    std::vector<size_t> arrayIndex(array.dimensionCount());
    for (size_t i = 0; i < dataArray.elementCount(); i++) {
        dataArray.toVectorIndex(i, dataIndex.data());
        for (size_t d = 0; d < array.dimensionCount(); d++)
            arrayIndex[array.dimensionCount() - 1 - d] = dataIndex[d];
        if (array.dimensionCount() == 2) // flip images in y
            arrayIndex[1] = array.dimension(1) - 1 - arrayIndex[1];
        size_t arrayComponentIndex = dataIndex[dataArray.dimensionCount() - 1];
        std::memcpy(dataArray.get(i),
                static_cast<const unsigned char*>(array.data())
                + array.componentOffset(arrayIndex, arrayComponentIndex),
                array.componentSize());
    }
    return dataArray;
}

typedef enum
{
    // the values correspond to the EXIF specification
    OriginTopLeft = 1,
    OriginTopRight = 2,
    OriginBottomRight = 3,
    OriginBottomLeft = 4,
    OriginLeftTop = 5,
    OriginRightTop = 6,
    OriginRightBottom = 7,
    OriginLeftBottom = 8
} ImageOriginLocation;

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

inline void reverseX(size_t width, size_t height, size_t line_size, size_t elem_size, unsigned char* data)
{
    std::vector<unsigned char> tmp_elem(elem_size);
    for (size_t y = 0; y < height; y++) {
        unsigned char* rowData = data + y * line_size;
        for (size_t x = 0; x < width / 2; x++) {
            std::memcpy(tmp_elem.data(), rowData + x * elem_size, elem_size);
            std::memcpy(rowData + x * elem_size, rowData + (width - 1 - x) * elem_size, elem_size);
            std::memcpy(rowData + (width - 1 - x) * elem_size, tmp_elem.data(), elem_size);
        }
    }
}

inline void reverseX(ArrayContainer& array)
{
    reverseX(array.dimension(0), array.dimension(1),
            array.dimension(0) * array.elementSize(),
            array.elementSize(),
            static_cast<unsigned char*>(array.data()));
}

inline ArrayContainer createTransposedContainer(const ArrayContainer& array)
{
    assert(array.dimensionCount() == 2);
    ArrayContainer r({ array.dimension(1), array.dimension(0) }, array.componentCount(), array.componentType());
    r.globalTagList() = array.globalTagList();
    r.dimensionTagList(0) = array.dimensionTagList(1);
    r.dimensionTagList(1) = array.dimensionTagList(0);
    for (size_t i = 0; i < r.componentCount(); i++)
        r.componentTagList(i) = array.componentTagList(i);
    return r;
}

inline void fixImageOrientation(ArrayContainer& array, ImageOriginLocation originLocation)
{
    assert(array.dimensionCount() == 2);
    switch (originLocation) {
    case OriginTopLeft:
        reverseY(array);
        break;
    case OriginTopRight:
        reverseY(array);
        reverseX(array);
        break;
    case OriginBottomRight:
        reverseX(array);
        break;
    case OriginBottomLeft:
        break;
    case OriginLeftTop:
        {
            ArrayContainer r = createTransposedContainer(array);
            for (size_t y = 0; y < r.dimension(1); y++) {
                for (size_t x = 0; x < r.dimension(0); x++) {
                    std::memcpy(r.get({ x, y }), array.get({ array.dimension(0) - 1 - y, x }), r.elementSize());
                }
            }
            array = r;
        }
        break;
    case OriginRightTop:
        {
            ArrayContainer r = createTransposedContainer(array);
            for (size_t y = 0; y < r.dimension(1); y++) {
                for (size_t x = 0; x < r.dimension(0); x++) {
                    std::memcpy(r.get({ x, y }), array.get({ array.dimension(0) - 1 - y, array.dimension(1) - 1 - x }), r.elementSize());
                }
            }
            array = r;
        }
        break;
    case OriginRightBottom:
        {
            ArrayContainer r = createTransposedContainer(array);
            for (size_t y = 0; y < r.dimension(1); y++) {
                for (size_t x = 0; x < r.dimension(0); x++) {
                    std::memcpy(r.get({ x, y }), array.get({ y, array.dimension(1) - 1 - x }), r.elementSize());
                }
            }
            array = r;
        }
        break;
    case OriginLeftBottom:
        {
            ArrayContainer r = createTransposedContainer(array);
            for (size_t y = 0; y < r.dimension(1); y++) {
                for (size_t x = 0; x < r.dimension(0); x++) {
                    std::memcpy(r.get({ x, y }), array.get({ y, x }), r.elementSize());
                }
            }
            array = r;
        }
        break;
    }
}

}

#endif
