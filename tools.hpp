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

#ifndef TAD_TOOLS_HPP
#define TAD_TOOLS_HPP

/**
 * \file tools.hpp
 * \brief Array manipulation functions.
 */

#include "array.hpp"

namespace TAD {

/*! \brief Fill \a array \a element (zero by default), starting at \a index (zero by default) and filling a region with the
 * given \a size (to the array extent by default). */
template<typename T>
void fill(Array<T>& array, const std::vector<T>& element = {}, const std::vector<size_t>& index = {}, const std::vector<size_t>& size = {})
{
    assert(element.size() == 0 || element.size() == array.componentCount());
    assert(index.size() == 0 || index.size() == array.dimensionCount());
    assert(size.size() == 0 || size.size() == array.dimensionCount());
    for (size_t d = 0; d < array.dimensionCount(); d++) {
        if (size.size() > 0 && size[d] == 0)
            return;
        if (index.size() > d && index[d] >= array.dimension(d))
            return;
    }
    if (index.size() == 0 && size.size() == 0) {
        // optimize common special case
        if (element.size() == 0) {
            std::memset(array.data(), 0, array.dataSize());
        } else {
            for (size_t e = 0; e < array.elementCount(); e++)
                std::memcpy(array.get(e), element.data(), array.elementSize());
        }
    } else {
        size_t i[array.dimensionCount()];
        for (size_t d = 0; d < array.dimensionCount(); d++)
            i[d] = (index.size() == 0 ? 0 : index[d]);
        for (;;) {
            T* data = array[std::vector<size_t>(i, i + array.dimensionCount())];
            size_t elementsToSet = std::min((size.size() == 0 ? std::numeric_limits<size_t>::max() : size[0]), array.dimension(0) - (index.size() == 0 ? 0 : index[0]));
            if (element.size() == 0) {
                std::memset(data, 0, elementsToSet * array.elementSize());
            } else {
                for (size_t e = 0; e < elementsToSet; e++)
                    std::memcpy(data + e * array.elementSize(), element.data(), array.elementSize());
            }
            size_t dimToInc = 1;
            bool inced = false;
            for (; dimToInc < array.dimensionCount(); ) {
                if (i[dimToInc] + 1 < (index.size() == 0 ? 0 : index[dimToInc]) + (size.size() == 0 ? std::numeric_limits<size_t>::max() : size[dimToInc]) && i[dimToInc] + 1 < array.dimension(dimToInc)) {
                    i[dimToInc]++;
                    for (size_t d = 0 ; d < dimToInc; d++)
                        i[d] = (index.size() == 0 ? 0 : index[d]);
                    inced = true;
                    break;
                }
            }
            if (!inced)
                break;
        }
    }
}

/*! \brief Get a region of \a array, starting at \a index (zero by default), with a given \a size (to the array extent by default).
 * If index and size result in data elements that are not actually included in the original array, these will be filled
 * with \a element (zero by default). */
template<typename T>
Array<T> getSubArray(const Array<T>& array, const std::vector<size_t>& index = {}, const std::vector<size_t>& size = {}, const std::vector<T>& element = {})
{
    assert(index.size() == 0 || index.size() == array.dimensionCount());
    assert(size.size() == 0 || size.size() == array.dimensionCount());
    assert(element.size() == 0 || element.size() == array.componentCount());

    // Determine default size if required
    size_t defaultSize[array.dimensionCount()];
    if (size.size() == 0) {
        for (size_t d = 0; d < array.dimensionCount(); d++) {
            if (index.size() != 0 && index[d] >= array.dimension(d))
                defaultSize[d] = 0;
            else
                defaultSize[d] = array.dimension(d) - (index.size() == 0 ? 0 : index[d]);
        }
    }
    const std::vector<size_t> defaultSizeTuple(defaultSize, array.dimensionCount());

    // Create result array
    Array<T> result(size.size() == 0 ? defaultSizeTuple : size, array.componentCount());

    // Copy metadata
    result.globalTagList() = array.globalTagList();
    for (size_t i = 0; i < array.dimensionCount(); i++)
        result.dimensionTagList(i) = array.dimensionTagList(i);
    for (size_t i = 0; i < array.componentCount(); i++)
        result.componentTagList(i) = array.componentTagList(i);

    // Check for early exit
    if (result.dimensionCount() == 0 || result.elementSize() == 0)
        return result;

    // Copy data
    size_t resultIndex[array.dimensionCount()];
    for (size_t d = 0; d < array.dimensionCount(); d++)
        resultIndex[d] = 0;
    size_t arrayIndex[array.dimensionCount()];
    for (size_t d = 0; d < array.dimensionCount(); d++)
        arrayIndex[d] = (index.size() == 0 ? 0 : index[d]);
    unsigned char* resultData = static_cast<unsigned char*>(result.data());
    const unsigned char* arrayData = static_cast<const unsigned char*>(array.data());
    for (;;) {
        for (size_t d = 0; d < array.dimensionCount(); d++)
            arrayIndex[d] = (index.size() == 0 ? 0 : index[d]) + resultIndex[d];
        size_t elementsToCopy, elementsToSet;
        if (index.size() > 0 && index[0] >= array.dimension(0)) {
            elementsToCopy = 0;
            elementsToSet = result.dimension(0);
        } else {
            elementsToCopy = std::min(result.dimension(0), array.dimension(0) - (index.size() == 0 ? 0 : index[0]));
            elementsToSet = result.dimension(0) - elementsToCopy;
        }
        // copy
        std::memcpy(resultData + result.elementOffset(std::vector<size_t>(resultIndex, resultIndex + array.dimensionCount())),
                arrayData + result.elementOffset(std::vector<size_t>(arrayIndex, arrayIndex + array.dimensionCount())),
                elementsToCopy * result.elementSize());
        // set
        T* dataToSet = result.get(std::vector<size_t>(resultIndex, resultIndex + array.dimensionCount()));
        if (element.size() == 0) {
            std::memset(dataToSet, 0, elementsToSet * array.elementSize());
        } else {
            for (size_t e = 0; e < elementsToSet; e++)
                std::memcpy(dataToSet + e * result.elementSize(), element.data(), result.elementSize());
        }
        // inc indices
        size_t dimToInc = 1;
        bool inced = false;
        for (; dimToInc < result.dimensionCount(); ) {
            if (resultIndex[dimToInc] + 1 < result.dimension(dimToInc)) {
                resultIndex[dimToInc]++;
                for (size_t d = 0 ; d < dimToInc; d++)
                    resultIndex[d] = 0;
                inced = true;
                break;
            }
        }
        if (!inced)
            break;
    }
    return result;
}

/*! \brief Copy the contents of \a source into \a dest at \a index (zero by default). */
inline void setSubArray(ArrayContainer& dest, const ArrayContainer& source, const std::vector<size_t>& index = {})
{
    assert(source.dimensionCount() == dest.dimensionCount());
    assert(source.componentCount() == dest.componentCount());
    assert(source.componentType() == dest.componentType());
    assert(index.size() == 0 || index.size() == dest.dimensionCount());
    if (source.dimensionCount() == 0)
        return;
    for (size_t d = 0; d < source.dimensionCount(); d++) {
        if (source.dimension(d) == 0)
            return;
        if (index.size() > 0 && index[d] >= dest.dimension(d))
            return;
    }
    size_t destIndex[dest.dimensionCount()];
    for (size_t d = 0; d < dest.dimensionCount(); d++)
        destIndex[d] = (index.size() == 0 ? 0 : index[d]);
    size_t sourceIndex[dest.dimensionCount()];
    for (size_t d = 0; d < dest.dimensionCount(); d++)
        sourceIndex[d] = 0;
    unsigned char* destData = static_cast<unsigned char*>(dest.data());
    const unsigned char* sourceData = static_cast<const unsigned char*>(source.data());
    for (;;) {
        for (size_t d = 0; d < dest.dimensionCount(); d++)
            sourceIndex[d] = destIndex[d] - (index.size() == 0 ? 0 : index[d]);
        size_t elementsToCopy;
        if (index.size() > 0 && index[0] >= dest.dimension(0))
            elementsToCopy = 0;
        else
            elementsToCopy = std::min(source.dimension(0), dest.dimension(0) - index[0]);
        std::memcpy(destData + dest.elementOffset(std::vector<size_t>(destIndex, destIndex + dest.dimensionCount())),
                sourceData + source.elementOffset(std::vector<size_t>(sourceIndex, sourceIndex + dest.dimensionCount())),
                elementsToCopy * source.elementSize());
        size_t dimToInc = 1;
        bool inced = false;
        for (; dimToInc < dest.dimensionCount(); ) {
            if (destIndex[dimToInc] + 1 < dest.dimension(dimToInc)) {
                destIndex[dimToInc]++;
                for (size_t d = 0 ; d < dimToInc; d++)
                    destIndex[d] = (index.size() == 0 ? 0 : index[d]);
                inced = true;
                break;
            }
        }
        if (!inced)
            break;
    }
}

/*! \brief Merge arrays \a a0 and \a a1 along \a dimension. For example,
 * merging to images along the first dimension places them next to each other,
 * merging them along the second dimension places them on top of each other. */
inline ArrayContainer merge(const ArrayContainer& a0, const ArrayContainer& a1, size_t dimension = 0)
{
    assert(a0.dimensionCount() == a1.dimensionCount());
    assert(a0.componentCount() == a1.componentCount());
    assert(a0.componentType() == a1.componentType());
    assert(dimension < a0.dimensionCount());

    size_t newDims[a0.dimensionCount()];
    for (size_t d = 0; d < a0.dimensionCount(); d++) {
        assert(d == dimension || a0.dimension(d) == a1.dimension(d));
        newDims[d] = a0.dimension(d);
        if (d == dimension)
            newDims[d] += a1.dimension(d);
    }

    // Create result array
    ArrayContainer result(std::vector<size_t>(newDims, newDims + a0.dimensionCount()), a0.componentCount(), a0.componentType());

    // Copy metadata
    result.globalTagList() = a0.globalTagList();
    for (size_t i = 0; i < a0.dimensionCount(); i++)
        result.dimensionTagList(i) = a0.dimensionTagList(i);
    for (size_t i = 0; i < a0.componentCount(); i++)
        result.componentTagList(i) = a0.componentTagList(i);

    // Check for early exit
    if (result.dimensionCount() == 0 || result.elementSize() == 0)
        return result;

    // Copy data
    setSubArray(result, a0);
    size_t a1Index[a0.dimensionCount()];
    for (size_t d = 0; d < a0.dimensionCount(); d++)
        a1Index[d] = (d == dimension ? a0.dimension(d) : 0);
    setSubArray(result, a1, std::vector<size_t>(a1Index, a1Index + a0.dimensionCount()));

    return result;
}

#if 0

TODO: port remaining gtatool functionality:

dimension-extract  Extract dimension from arrays (e.g. slice from volume)
dimension-merge    Merge arrays along dimension (e.g. slices to volume)

dimension-reorder  Reorder array dimensions (e.g. transpose matrix)
dimension-reverse  Reverse array dimension (e.g. flip image)

dimension-split    Split arrays along dimension (e.g. volume to slices)

component-add      Add components to arrays (e.g. add A to RGB)
component-extract  Extract components from arrays (e.g. R from RGB)

component-merge    Merge arrays on component level (e.g. R,G,B to RGB)
component-reorder  Reorder array components (e.g. RGB to BGR)
component-set      Set array component values
component-split    Split arrays along components (e.g. RGB to R,G,B)

#endif

}
#endif