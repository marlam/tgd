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

#ifndef TAD_ARRAY_HPP
#define TAD_ARRAY_HPP

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include <initializer_list>
#include <memory>

#include "taglist.hpp"

/**
 * \file array.hpp
 * \brief The libtad C++ interface.
 */

/**
 * \mainpage libtad Reference
 *
 * \section overview Tagged Array Data (TAD): Overview
 *
 * TAD manages multidimensional arrays. Each array element consists of a fixed
 * number of components. All components are of the same data type (integer or
 * floating point). For example, an array holding image data could be a
 * two-dimensional array with size 800x600, and each element could consist of
 * three uint8 components representing the R, G, and B channels.
 *
 * Metadata is managed using tags, which are key/value pairs where both key and
 * value are represented as strings. An array has a global tag lists (typically
 * describing the array data as a whole), dimension-specific tag lists (typically
 * describing the dimensions), and component-specific tag lists (typically
 * describing the interpretation of each component).
 *
 * \section lib The libtad Library
 *
 * The library works with the following classes: \a TAD::TagList to manage metadata,
 * \a TAD::ArrayDescription to handle all metadata describing an array, \a TAD::ArrayContainer
 * to handle array data of any type, and finally \a TAD::Array to handle arrays with
 * a specific data type so that convenient operations on the data are possible.
 *
 * All array data is shared by default when making copies of an array. If you want
 * a copy of the data, use \a TAD::ArrayContainer::deepCopy() or \a TAD::Array::deepCopy().
 *
 * Input and output are managed by the \a TAD::Importer and \a TAD::Exporter classes,
 * and there are shortcuts \a TAD::load() and \a TAD::save() to read and write arrays
 * in just one line of code.
 */

namespace TAD {

/*! \brief The data type that array element components can represent. */
enum Type
{
    int8  = 0,     /**< \brief int8_t */
    uint8 = 1,     /**< \brief uint8_t */
    int16 = 2,     /**< \brief int16_t */
    uint16 = 3,    /**< \brief uint16_t */
    int32 = 4,     /**< \brief int32_t */
    uint32 = 5,    /**< \brief uint32_t */
    int64 = 6,     /**< \brief int64_t */
    uint64 = 7,    /**< \brief uint64_t */
    float32 = 8,   /**< \brief IEEE 754 single precision floating point (on all relevant platforms: float) */
    float64 = 9,   /**< \brief IEEE 754 double precision floating point (on all relevant platforms: double) */
};

/*! \brief Returns the size of a TAD type. */
inline constexpr size_t typeSize(Type t)
{
    static_assert(sizeof(int8_t) == 1);
    static_assert(sizeof(uint8_t) == 1);
    static_assert(sizeof(int16_t) == 2);
    static_assert(sizeof(uint16_t) == 2);
    static_assert(sizeof(int32_t) == 4);
    static_assert(sizeof(uint32_t) == 4);
    static_assert(sizeof(int64_t) == 8);
    static_assert(sizeof(uint64_t) == 8);
    static_assert(sizeof(float) == 4);
    static_assert(sizeof(double) == 8);
    return (t == int8 ? sizeof(int8_t)
            : t == uint8 ? sizeof(uint8_t)
            : t == int16 ? sizeof(int16_t)
            : t == uint16 ? sizeof(uint16_t)
            : t == int32 ? sizeof(int32_t)
            : t == uint32 ? sizeof(uint32_t)
            : t == int64 ? sizeof(int64_t)
            : t == uint64 ? sizeof(uint64_t)
            : t == float32 ? sizeof(float)
            : t == float64 ? sizeof(double)
            : 0);
}

/*! \brief Returns the TAD component type that corresponds to the template parameter. */
template<typename T> inline constexpr Type typeFromTemplate();
/*! \cond */
template<> inline constexpr Type typeFromTemplate<int8_t>() { return int8; }
template<> inline constexpr Type typeFromTemplate<uint8_t>() { return uint8; }
template<> inline constexpr Type typeFromTemplate<int16_t>() { return int16; }
template<> inline constexpr Type typeFromTemplate<uint16_t>() { return uint16; }
template<> inline constexpr Type typeFromTemplate<int32_t>() { return int32; }
template<> inline constexpr Type typeFromTemplate<uint32_t>() { return uint32; }
template<> inline constexpr Type typeFromTemplate<int64_t>() { return int64; }
template<> inline constexpr Type typeFromTemplate<uint64_t>() { return uint64; }
template<> inline constexpr Type typeFromTemplate<float>() { return float32; }
template<> inline constexpr Type typeFromTemplate<double>() { return float64; }
/*! \endcond */

/*! \brief Determine the TAD component type described in the string, return false if this fails. */
inline bool typeFromString(const std::string& s, Type* t)
{
    bool ok = true;
    if (s == "int8")
        *t = int8;
    else if (s == "uint8")
        *t = uint8;
    else if (s == "int16")
        *t = int16;
    else if (s == "uint16")
        *t = uint16;
    else if (s == "int32")
        *t = int32;
    else if (s == "uint32")
        *t = uint32;
    else if (s == "int64")
        *t = int64;
    else if (s == "uint64")
        *t = uint64;
    else if (s == "float32")
        *t = float32;
    else if (s == "float64")
        *t = float64;
    else
        ok = false;
    return ok;
}

/*! \brief Return the name of the given type */
inline const char* typeToString(Type t)
{
    const char* p = nullptr;
    switch (t) {
    case int8:
        p = "int8";
        break;
    case uint8:
        p = "uint8";
        break;
    case int16:
        p = "int16";
        break;
    case uint16:
        p = "uint16";
        break;
    case int32:
        p = "int32";
        break;
    case uint32:
        p = "uint32";
        break;
    case int64:
        p = "int64";
        break;
    case uint64:
        p = "uint64";
        break;
    case float32:
        p = "float32";
        break;
    case float64:
        p = "float64";
        break;
    }
    return p;
}

/*! \brief The ArrayDescription manages array metadata. */
class ArrayDescription
{
private:
    // defining properties
    std::vector<size_t> _dimensions;
    size_t _componentCount;
    Type _componentType;
    // computed properties
    unsigned int _componentSize;
    size_t _elementSize;
    size_t _elementCount;
    // meta data
    TagList _globalTagList;
    std::vector<TagList> _dimensionTagLists;
    std::vector<TagList> _componentTagLists;

    size_t initElementCount() const
    {
        size_t n = 0;
        if (dimensionCount() > 0) {
            n = dimension(0);
            for (size_t d = 1; d < dimensionCount(); d++)
                n *= dimension(d);
        }
        return n;
    }

public:
    /**
     * \name Constructors / Destructor
     */
    /*@{*/

    /*! \brief Constructor for an empty array description. */
    ArrayDescription() :
        ArrayDescription({}, 0, int8)
    {
    }

    /*! \brief Constructor for an array description
     * \param dimensions        List of sizes, defining both the number of dimensions and the array size.
     * \param componentCount    Number of components in one array element
     * \param componentType     Data type of the components
     *
     * The array dimensions and the type and number of the element components must be specified.
     * For example, for an image with 800x600 RGB pixels one might construct the following array:
     * `Array image({ 800, 600}, 3, uint8);`
     */
    ArrayDescription(const std::vector<size_t>& dimensions, size_t componentCount, Type componentType) :
        _dimensions(dimensions),
        _componentCount(componentCount),
        _componentType(componentType),
        _componentSize(typeSize(componentType)),
        _elementSize(_componentCount * _componentSize),
        _elementCount(initElementCount()),
        _dimensionTagLists(dimensions.size()),
        _componentTagLists(_componentCount)
    {
    }

    /*! \brief Constructor for an array description
     * \param descr     Existing array description
     * \param type      New type
     *
     * Constructs an array description that is a copy of the given description except that it has
     * the given new type.
     */
    explicit ArrayDescription(const ArrayDescription& descr, Type type) :
        ArrayDescription(descr)
    {
        _componentType = type;
        _componentSize = typeSize(type);
        _elementSize = componentCount() * componentSize();
    }

    /*@}*/

    /**
     * \name Access to array data specifications.
     */
    /*@{*/

    /*! \brief Returns the number of dimensions. */
    size_t dimensionCount() const
    {
        return _dimensions.size();
    }

    /*! \brief Returns the size of dimension \a d. */
    size_t dimension(size_t d) const
    {
        assert(d < dimensionCount());
        return _dimensions[d];
    }

    /*! \brief Returns the list of dimensions. */
    const std::vector<size_t>& dimensions() const
    {
        return _dimensions;
    }

    /*! \brief Returns the number of components in each element. */
    size_t componentCount() const
    {
        return _componentCount;
    }

    /*! \brief Returns the type represented by each element component. */
    Type componentType() const
    {
        return _componentType;
    }
 
    /*! \brief Returns the size of a component. */
    size_t componentSize() const
    {
        return _componentSize;
    }

    /*! \brief Returns the size of an element. */
    size_t elementSize() const
    {
        return _elementSize;
    }

    /*! \brief Returns the number of elements in the array. */
    size_t elementCount() const
    {
        return _elementCount;
    }

    /*! \brief Returns the total data size. */
    size_t dataSize() const
    {
        return elementCount() * elementSize();
    }

    /*! \brief Returns whether the dimensions and components of array \a match those of this array. */
    bool isCompatible(const ArrayDescription& a) const
    {
        return (componentType() == a.componentType()
                && componentCount() == a.componentCount()
                && elementCount() == a.elementCount());
    }

    /*! \brief Returns this as a description. This is useful for derived classes. */
    const ArrayDescription& description() const
    {
        return *this;
    }

    /*@}*/

    /**
     * \name Index management
     */
    /*@{*/

    /*! \brief Convert the given multidimensional element index to a linear
     * element index. For example, for a 800x600 array, the multidimensional
     * index { 1, 1 } is converted to linear index 601.
     */
    size_t toLinearIndex(const std::vector<size_t>& elementIndex) const
    {
        assert(elementIndex.size() == dimensionCount());
        for (size_t d = 0; d < elementIndex.size(); d++)
            assert(elementIndex[d] < dimension(d));

        size_t dimProduct = 1;
        size_t index = elementIndex[0];
        for (size_t i = 1; i < elementIndex.size(); i++) {
            dimProduct *= dimension(i - 1);
            index += elementIndex[i] * dimProduct;
        }

        return index;
    }

    /*! \brief Convert the given multidimensional element index to a linear
     * element index. For example, for a 800x600 array, the multidimensional
     * index { 1, 1 } is converted to linear index 601.
     */
    size_t toLinearIndex(const std::initializer_list<size_t>& elementIndex) const
    {
        assert(elementIndex.size() == dimensionCount());
        for (size_t d = 0; d < elementIndex.size(); d++)
            assert(elementIndex.begin()[d] < dimension(d));

        size_t dimProduct = 1;
        size_t index = elementIndex.begin()[0];
        for (size_t i = 1; i < elementIndex.size(); i++) {
            dimProduct *= dimension(i - 1);
            index += elementIndex.begin()[i] * dimProduct;
        }

        return index;
    }

    /*! \brief Convert the given linear element index to a multidimensional
     * index. For example, for a 800x600 array, the linear index 601 is
     * converted to the multidimensional index { 1, 1 }.
     */
    void toVectorIndex(size_t elementIndex, size_t* vectorIndex) const
    {
        assert(elementIndex < elementCount());

        size_t multipliedDimSizes = elementCount();
        for (size_t i = 0; i < dimensionCount(); i++) {
            size_t j = dimensionCount() - 1 - i;
            multipliedDimSizes /= dimension(j);
            vectorIndex[j] = elementIndex / multipliedDimSizes;
            elementIndex -= vectorIndex[j] * multipliedDimSizes;
        }
    }

    /*! \brief Returns the offset of the element with index \a elementIndex within the data. */
    size_t elementOffset(size_t elementIndex) const
    {
        assert(elementIndex < elementCount());
        return elementIndex * elementSize();
    }

    /*! \brief Returns the offset of the element with index \a elementIndex within the data. */
    size_t elementOffset(const std::vector<size_t>& elementIndex) const
    {
        return elementOffset(toLinearIndex(elementIndex));
    }

    /*! \brief Returns the offset of the element with index \a elementIndex within the data. */
    size_t elementOffset(const std::initializer_list<size_t>& elementIndex) const
    {
        return elementOffset(toLinearIndex(elementIndex));
    }

    /*! \brief Returns the offset of the component with index \a componentIndex within an array element. */
    size_t componentOffset(size_t componentIndex) const
    {
        assert(componentIndex < componentCount());
        return componentIndex * componentSize();
    }

    /*! \brief Returns the offset of the component with index \a componentIndex
     * in the element with index \a elementIndex within an array element. */
    size_t componentOffset(size_t elementIndex, size_t componentIndex) const
    {
        return elementOffset(elementIndex) + componentOffset(componentIndex);
    }

    /*! \brief Returns the offset of the component with index \a componentIndex
     * in the element with index \a elementIndex within an array element. */
    size_t componentOffset(const std::vector<size_t>& elementIndex, size_t componentIndex) const
    {
        return elementOffset(elementIndex) + componentOffset(componentIndex);
    }

    /*! \brief Returns the offset of the component with index \a componentIndex
     * in the element with index \a elementIndex within an array element. */
    size_t componentOffset(const std::initializer_list<size_t>& elementIndex, size_t componentIndex) const
    {
        return elementOffset(elementIndex) + componentOffset(componentIndex);
    }

    /*@}*/

    /**
     * \name Metadata management
     */
    /*@{*/

    /*! \brief Returns the global tag list. */
    const TagList& globalTagList() const
    {
        return _globalTagList;
    }

    /*! \brief Returns the global tag list. */
    TagList& globalTagList()
    {
        return _globalTagList;
    }

    /*! \brief Returns the tag list for dimension \a d. */
    const TagList& dimensionTagList(size_t d) const
    {
        assert(d < dimensionCount());
        return _dimensionTagLists[d];
    }

    /*! \brief Returns the tag list for dimension \a d. */
    TagList& dimensionTagList(size_t d)
    {
        assert(d < dimensionCount());
        return _dimensionTagLists[d];
    }

    /*! \brief Returns the tag list for component \a c. */
    const TagList& componentTagList(size_t c) const
    {
        assert(c < componentCount());
        return _componentTagLists[c];
    }

    /*! \brief Returns the tag list for component \a c. */
    TagList& componentTagList(size_t c)
    {
        assert(c < componentCount());
        return _componentTagLists[c];
    }

    /*@}*/
};

/*! \brief The ArrayContainer class manages arrays with arbitrary component data types. */
class ArrayContainer : public ArrayDescription
{
    /*! \cond */
private:
    std::shared_ptr<unsigned char[]> _data;

protected:
    template<typename T>
    bool typeMatchesTemplate() const
    {
        return (typeFromTemplate<T>() == componentType());
    }
    /*! \endcond */

public:
    /**
     * \name Constructors / Destructor
     */
    /*@{*/

    /*! \brief Constructor for an empty array container. */
    ArrayContainer() :
        ArrayDescription(), _data(nullptr)
    {
    }

    /*! \brief Constructor for an array container. */
    explicit ArrayContainer(const ArrayDescription& desc) :
        ArrayDescription(desc), _data(new unsigned char[dataSize()])
    {
    }

    /*! \brief Constructor for an array container. */
    ArrayContainer(const std::vector<size_t>& dimensions, size_t components, Type componentType) :
        ArrayContainer(ArrayDescription(dimensions, components, componentType))
    {
    }

    /*! \brief Construct an array and perform deep copy of data */
    ArrayContainer deepCopy() const
    {
        ArrayDescription rDescr(*this);
        ArrayContainer r(rDescr);
        std::memcpy(r._data.get(), this->_data.get(), this->dataSize());
        return r;
    }

    /*@}*/

    /**
     * \name Data access
     */
    /*@{*/

    /*! \brief Returns a pointer to the data. This will return a null pointer
     * as long as the data was not allocated yet; see \a allocateData(). */
    const void* data() const
    {
        return static_cast<const void*>(_data.get());;
    }

    /*! \brief Returns a pointer to the data. This will return a null pointer
     * as long as the data was not allocated yet; see \a allocateData(). */
    void* data()
    {
        return static_cast<void*>(_data.get());
    }

    /*! \brief Returns a pointer to the element with index \a elementIndex.
     * Note that the data must be allocated, see \a createData(). */
    template<typename T>
    const T* get(size_t elementIndex) const
    {
        assert(typeMatchesTemplate<T>());
        return reinterpret_cast<const T*>(_data.get() + elementOffset(elementIndex));
    }
    /*! \cond */
    const void* get(size_t elementIndex) const
    {
        return static_cast<const void*>(_data.get() + elementOffset(elementIndex));
    }
    /*! \endcond */

    /*! \brief Returns a pointer to the element with index \a elementIndex.
     * Note that the data must be allocated, see \a createData(). */
    template<typename T>
    const T* get(const std::vector<size_t>& elementIndex) const
    {
        return get<T>(toLinearIndex(elementIndex));
    }
    /*! \cond */
    const void* get(const std::vector<size_t>& elementIndex) const
    {
        return get(toLinearIndex(elementIndex));
    }
    /*! \endcond */

    /*! \brief Returns a pointer to the element with index \a elementIndex.
     * Note that the data must be allocated, see \a createData(). */
    template<typename T>
    const T* get(const std::initializer_list<size_t>& elementIndex) const
    {
        return get<T>(toLinearIndex(elementIndex));
    }
    /*! \cond */
    const void* get(const std::initializer_list<size_t>& elementIndex) const
    {
        return get(toLinearIndex(elementIndex));
    }
    /*! \endcond */

    /*! \brief Returns a pointer to the element with index \a elementIndex.
     * Note that the data must be allocated, see \a createData(). */
    template<typename T>
    T* get(size_t elementIndex)
    {
        assert(typeMatchesTemplate<T>());
        return reinterpret_cast<T*>(_data.get() + elementOffset(elementIndex));
    }
    /*! \cond */
    void* get(size_t elementIndex)
    {
        return static_cast<void*>(_data.get() + elementOffset(elementIndex));
    }
    /*! \endcond */

    /*! \brief Returns a pointer to the element with index \a elementIndex.
     * Note that the data must be allocated, see \a createData(). */
    template<typename T>
    T* get(const std::vector<size_t>& elementIndex)
    {
        return get<T>(toLinearIndex(elementIndex));
    }
    /*! \cond */
    void* get(const std::vector<size_t>& elementIndex)
    {
        return get(toLinearIndex(elementIndex));
    }
    /*! \endcond */

    /*! \brief Returns a pointer to the element with index \a elementIndex.
     * Note that the data must be allocated, see \a createData(). */
    template<typename T>
    T* get(const std::initializer_list<size_t>& elementIndex)
    {
        return get<T>(toLinearIndex(elementIndex));
    }
    /*! \cond */
    void* get(const std::initializer_list<size_t>& elementIndex)
    {
        return get(toLinearIndex(elementIndex));
    }
    /*! \endcond */

    /*! \brief Sets the components of the element with index \a elementIndex to the given \a values. */
    template<typename T>
    void set(size_t elementIndex, const std::vector<T>& elementValue)
    {
        assert(elementValue.size() == componentCount());
        T* ptr = get<T>(elementIndex);
        for (size_t c = 0; c < componentCount(); c++)
            ptr[c] = elementValue[c];
    }

    /*! \brief Sets the components of the element with index \a elementIndex to the given \a values. */
    template<typename T>
    void set(size_t elementIndex, const std::initializer_list<T>& elementValue)
    {
        assert(elementValue.size() == componentCount());
        T* ptr = get<T>(elementIndex);
        for (size_t c = 0; c < componentCount(); c++)
            ptr[c] = elementValue.begin()[c];
    }

    /*! \brief Sets the components of the element with index \a elementIndex to the given \a values. */
    template<typename T>
    void set(const std::vector<size_t>& elementIndex, const std::vector<T>& elementValue)
    {
        set(toLinearIndex(elementIndex), elementValue);
    }

    /*! \brief Sets the components of the element with index \a elementIndex to the given \a values. */
    template<typename T>
    void set(const std::vector<size_t>& elementIndex, const std::initializer_list<T>& elementValue)
    {
        set(toLinearIndex(elementIndex), elementValue);
    }

    /*! \brief Sets the components of the element with index \a elementIndex to the given \a values. */
    template<typename T>
    void set(const std::initializer_list<size_t>& elementIndex, const std::vector<T>& elementValue)
    {
        set(toLinearIndex(elementIndex), elementValue);
    }

    /*! \brief Sets the components of the element with index \a elementIndex to the given \a values. */
    template<typename T>
    void set(const std::initializer_list<size_t>& elementIndex, const std::initializer_list<T>& elementValue)
    {
        set(toLinearIndex(elementIndex), elementValue);
    }

    /*! \brief Returns the value of the component with index \a componentIndex within the element with index \a elementIndex. */
    template<typename T>
    T get(size_t elementIndex, size_t componentIndex) const
    {
        assert(componentIndex < componentCount());
        return get<T>(elementIndex)[componentIndex];
    }

    /*! \brief Returns the value of the component with index \a componentIndex within the element with index \a elementIndex. */
    template<typename T>
    T get(const std::vector<size_t>& elementIndex, size_t componentIndex) const
    {
        return get<T>(toLinearIndex(elementIndex), componentIndex);
    }

    /*! \brief Returns the value of the component with index \a componentIndex within the element with index \a elementIndex. */
    template<typename T>
    T get(const std::initializer_list<size_t>& elementIndex, size_t componentIndex) const
    {
        return get<T>(toLinearIndex(elementIndex), componentIndex);
    }

    /*! \brief Sets the component with index \a componentIndex within the element with index \a elementIndex to \a value. */
    template<typename T>
    void set(size_t elementIndex, size_t componentIndex, T value)
    {
        assert(componentIndex < componentCount());
        get<T>(elementIndex)[componentIndex] = value;
    }

    /*! \brief Sets the component with index \a componentIndex within the element with index \a elementIndex to \a value. */
    template<typename T>
    void set(const std::vector<size_t>& elementIndex, size_t componentIndex, T value)
    {
        set(toLinearIndex(elementIndex), componentIndex, value);
    }

    /*! \brief Sets the component with index \a componentIndex within the element with index \a elementIndex to \a value. */
    template<typename T>
    void set(const std::initializer_list<size_t>& elementIndex, size_t componentIndex, T value)
    {
        set(toLinearIndex(elementIndex), componentIndex, value);
    }
};

/*! \brief An Array with a specific component data type given the template parameter.
 * This is the main class to work with since it allows useful for operations on arrays
 * (e.g. component-wise addition) and provides iterators. */
template<typename T> class Array : public ArrayContainer
{
public:
    /**
     * \name Constructors etc
     */

    /*@{*/

    /*! \brief Constructor for an empty array. */
    Array() :
        ArrayContainer()
    {
    }

    /*! \brief Constructor for an array. */
    explicit Array(const std::vector<size_t>& dimensions, size_t components)
        : ArrayContainer(dimensions, components, typeFromTemplate<T>())
    {
    }

    /*! \brief Constructor for an array. */
    explicit Array(const ArrayDescription& desc) :
        ArrayContainer(desc)
    {
        assert(typeMatchesTemplate<T>());
    }

    /*! \brief Constructor for an array. */
    Array(const ArrayContainer& container) :
        ArrayContainer(container)
    {
        assert(typeMatchesTemplate<T>());
    }

    /*! \brief Construct an array and perform deep copy of data */
    Array deepCopy() const
    {
        return ArrayContainer::deepCopy();
    }

    /*@}*/

    /**
     * \name Iterators
     */

    /*@{*/

    /*! \brief Iterator over all components in the array. This is a random access iterator. */
    class ComponentIterator : public std::iterator<std::random_access_iterator_tag, T>
    {
    private:
        T* _ptr;

    public:
        using difference_type = typename std::iterator<std::random_access_iterator_tag, T>::difference_type;

        ComponentIterator() : _ptr(nullptr) {}
        ComponentIterator(T* rhs) : _ptr(rhs) {}
        ComponentIterator(const ComponentIterator& rhs) : _ptr(rhs._ptr) {}
        ComponentIterator& operator+=(difference_type rhs) { _ptr += rhs; return *this; }
        ComponentIterator& operator-=(difference_type rhs) { _ptr -= rhs; return *this; }
        T& operator*() const { return *_ptr; }
        T* operator->() const { return _ptr; }
        T& operator[](difference_type rhs) const { return _ptr[rhs]; }

        ComponentIterator& operator++() { ++_ptr; return *this; }
        ComponentIterator& operator--() { --_ptr; return *this; }
        ComponentIterator operator++(int) { ComponentIterator tmp(*this); ++_ptr; return tmp; }
        ComponentIterator operator--(int) { ComponentIterator tmp(*this); --_ptr; return tmp; }
        difference_type operator-(const ComponentIterator& rhs) const { return _ptr - rhs._ptr; }
        ComponentIterator operator+(difference_type rhs) const { return ComponentIterator(_ptr + rhs); }
        ComponentIterator operator-(difference_type rhs) const { return ComponentIterator(_ptr - rhs); }
        friend ComponentIterator operator+(difference_type lhs, const ComponentIterator& rhs) { return ComponentIterator(lhs + rhs._ptr); }
        friend ComponentIterator operator-(difference_type lhs, const ComponentIterator& rhs) { return ComponentIterator(lhs - rhs._ptr); }

        bool operator==(const ComponentIterator& rhs) const { return _ptr == rhs._ptr; }
        bool operator!=(const ComponentIterator& rhs) const { return _ptr != rhs._ptr; }
        bool operator>(const ComponentIterator& rhs) const { return _ptr > rhs._ptr; }
        bool operator<(const ComponentIterator& rhs) const { return _ptr < rhs._ptr; }
        bool operator>=(const ComponentIterator& rhs) const { return _ptr >= rhs._ptr; }
        bool operator<=(const ComponentIterator& rhs) const { return _ptr <= rhs._ptr; }
    };

    /*! \brief Const iterator over all components in the array. This is a random access iterator. */
    class ConstComponentIterator : public std::iterator<std::random_access_iterator_tag, T>
    {
    private:
        const T* _ptr;

    public:
        using difference_type = typename std::iterator<std::random_access_iterator_tag, T>::difference_type;

        ConstComponentIterator() : _ptr(nullptr) {}
        ConstComponentIterator(const T* rhs) : _ptr(rhs) {}
        ConstComponentIterator(const ConstComponentIterator& rhs) : _ptr(rhs._ptr) {}
        ConstComponentIterator& operator+=(difference_type rhs) { _ptr += rhs; return *this; }
        ConstComponentIterator& operator-=(difference_type rhs) { _ptr -= rhs; return *this; }
        const T& operator*() const { return *_ptr; }
        const T* operator->() const { return _ptr; }
        const T& operator[](difference_type rhs) const { return _ptr[rhs]; }

        ConstComponentIterator& operator++() { ++_ptr; return *this; }
        ConstComponentIterator& operator--() { --_ptr; return *this; }
        ConstComponentIterator operator++(int) { ConstComponentIterator tmp(*this); ++_ptr; return tmp; }
        ConstComponentIterator operator--(int) { ConstComponentIterator tmp(*this); --_ptr; return tmp; }
        difference_type operator-(const ConstComponentIterator& rhs) const { return _ptr - rhs._ptr; }
        ConstComponentIterator operator+(difference_type rhs) const { return ConstComponentIterator(_ptr + rhs); }
        ConstComponentIterator operator-(difference_type rhs) const { return ConstComponentIterator(_ptr - rhs); }
        friend ConstComponentIterator operator+(difference_type lhs, const ConstComponentIterator& rhs) { return ConstComponentIterator(lhs + rhs._ptr); }
        friend ConstComponentIterator operator-(difference_type lhs, const ConstComponentIterator& rhs) { return ConstComponentIterator(lhs - rhs._ptr); }

        bool operator==(const ConstComponentIterator& rhs) const { return _ptr == rhs._ptr; }
        bool operator!=(const ConstComponentIterator& rhs) const { return _ptr != rhs._ptr; }
        bool operator>(const ConstComponentIterator& rhs) const { return _ptr > rhs._ptr; }
        bool operator<(const ConstComponentIterator& rhs) const { return _ptr < rhs._ptr; }
        bool operator>=(const ConstComponentIterator& rhs) const { return _ptr >= rhs._ptr; }
        bool operator<=(const ConstComponentIterator& rhs) const { return _ptr <= rhs._ptr; }
    };

    /*! \brief Iterator over all elements in the array. When dereferenced, this returns a pointer
     * to the components of an element. This is a random access iterator. */
    class ElementIterator : public std::iterator<std::random_access_iterator_tag, T>
    {
    private:
        T* _ptr;
        const size_t _compCount;

    public:
        using difference_type = typename std::iterator<std::random_access_iterator_tag, T>::difference_type;

        ElementIterator() : _ptr(nullptr) {}
        ElementIterator(T* rhs, size_t componentCount = 1) : _ptr(rhs), _compCount(componentCount) {}
        ElementIterator(const ElementIterator& rhs) : _ptr(rhs._ptr), _compCount(rhs._compCount) {}
        ElementIterator& operator+=(difference_type rhs) { _ptr += _compCount * rhs; return *this; }
        ElementIterator& operator-=(difference_type rhs) { _ptr -= _compCount * rhs; return *this; }
        T* operator*() const { return _ptr; }
        T* operator->() const { return _ptr; }
        T* operator[](difference_type rhs) const { return _ptr + _compCount * rhs; }

        ElementIterator& operator++() { _ptr += _compCount; return *this; }
        ElementIterator& operator--() { _ptr -= _compCount; return *this; }
        ElementIterator operator++(int) { ElementIterator tmp(*this); _ptr += _compCount; return tmp; }
        ElementIterator operator--(int) { ElementIterator tmp(*this); _ptr -= _compCount; return tmp; }
        difference_type operator-(const ElementIterator& rhs) const { return (_ptr - rhs._ptr) / _compCount; }
        ElementIterator operator+(difference_type rhs) const { return ElementIterator(_ptr + _compCount * rhs); }
        ElementIterator operator-(difference_type rhs) const { return ElementIterator(_ptr - _compCount * rhs); }
        friend ElementIterator operator+(difference_type lhs, const ElementIterator& rhs) { return ElementIterator(lhs * rhs._compCount + rhs._ptr); }
        friend ElementIterator operator-(difference_type lhs, const ElementIterator& rhs) { return ElementIterator(lhs * rhs._compCount - rhs._ptr); }

        bool operator==(const ElementIterator& rhs) const { return _ptr == rhs._ptr; }
        bool operator!=(const ElementIterator& rhs) const { return _ptr != rhs._ptr; }
        bool operator>(const ElementIterator& rhs) const { return _ptr > rhs._ptr; }
        bool operator<(const ElementIterator& rhs) const { return _ptr < rhs._ptr; }
        bool operator>=(const ElementIterator& rhs) const { return _ptr >= rhs._ptr; }
        bool operator<=(const ElementIterator& rhs) const { return _ptr <= rhs._ptr; }
    };

    /*! \brief Const iterator over all elements in the array. When dereferenced, this returns a pointer
     * to the components of an element. This is a random access iterator. */
    class ConstElementIterator : public std::iterator<std::random_access_iterator_tag, T>
    {
    private:
        const T* _ptr;
        const size_t _compCount;

    public:
        using difference_type = typename std::iterator<std::random_access_iterator_tag, T>::difference_type;

        ConstElementIterator() : _ptr(nullptr) {}
        ConstElementIterator(const T* rhs, size_t componentCount = 1) : _ptr(rhs), _compCount(componentCount) {}
        ConstElementIterator(const ConstElementIterator& rhs) : _ptr(rhs._ptr), _compCount(rhs._compCount) {}
        ConstElementIterator& operator+=(difference_type rhs) { _ptr += _compCount * rhs; return *this; }
        ConstElementIterator& operator-=(difference_type rhs) { _ptr -= _compCount * rhs; return *this; }
        const T* operator*() const { return _ptr; }
        const T* operator->() const { return _ptr; }
        const T* operator[](difference_type rhs) const { return _ptr + _compCount * rhs; }

        ConstElementIterator& operator++() { _ptr += _compCount; return *this; }
        ConstElementIterator& operator--() { _ptr -= _compCount; return *this; }
        ConstElementIterator operator++(int) { ConstElementIterator tmp(*this); _ptr += _compCount; return tmp; }
        ConstElementIterator operator--(int) { ConstElementIterator tmp(*this); _ptr -= _compCount; return tmp; }
        difference_type operator-(const ConstElementIterator& rhs) const { return (_ptr - rhs._ptr) / _compCount; }
        ConstElementIterator operator+(difference_type rhs) const { return ConstElementIterator(_ptr + _compCount * rhs); }
        ConstElementIterator operator-(difference_type rhs) const { return ConstElementIterator(_ptr - _compCount * rhs); }
        friend ConstElementIterator operator+(difference_type lhs, const ConstElementIterator& rhs) { return ConstElementIterator(lhs * rhs._compCount + rhs._ptr); }
        friend ConstElementIterator operator-(difference_type lhs, const ConstElementIterator& rhs) { return ConstElementIterator(lhs * rhs._compCount - rhs._ptr); }

        bool operator==(const ConstElementIterator& rhs) const { return _ptr == rhs._ptr; }
        bool operator!=(const ConstElementIterator& rhs) const { return _ptr != rhs._ptr; }
        bool operator>(const ConstElementIterator& rhs) const { return _ptr > rhs._ptr; }
        bool operator<(const ConstElementIterator& rhs) const { return _ptr < rhs._ptr; }
        bool operator>=(const ConstElementIterator& rhs) const { return _ptr >= rhs._ptr; }
        bool operator<=(const ConstElementIterator& rhs) const { return _ptr <= rhs._ptr; }
    };

    /*! \brief Returns an iterator pointing to the first component of the first element. */
    ComponentIterator componentBegin() noexcept { return ComponentIterator(get<T>(0)); };
    /*! \brief Returns an iterator pointing past the last component of the last element. */
    ComponentIterator componentEnd() noexcept { return ComponentIterator(get<T>(0) + elementCount() * componentCount()); };
    /*! \brief Returns an iterator pointing to the first element. */
    ElementIterator elementBegin() noexcept { return ElementIterator(get<T>(0), componentCount()); };
    /*! \brief Returns an iterator pointing past the last element. */
    ElementIterator elementEnd() noexcept { return ElementIterator(get<T>(0) + elementCount() * componentCount(), componentCount()); };

    /*! \brief Returns an iterator pointing to the first component of the first element. */
    ConstComponentIterator constComponentBegin() const noexcept { return ConstComponentIterator(get<T>(0)); };
    /*! \brief Returns an iterator pointing past the last component of the last element. */
    ConstComponentIterator constComponentEnd() const noexcept { return ConstComponentIterator(get<T>(0) + elementCount() * componentCount()); };
    /*! \brief Returns an iterator pointing to the first element. */
    ConstElementIterator constElementBegin() const noexcept { return ConstElementIterator(get<T>(0), componentCount()); };
    /*! \brief Returns an iterator pointing past the last element. */
    ConstElementIterator constElementEnd() const noexcept { return ConstElementIterator(get<T>(0) + elementCount() * componentCount(), componentCount()); };

    /*@}*/

    /**
     * \name Data access
     */

    /*! \brief Returns a pointer to the element with index \a elementIndex. */
    const T* operator[](size_t elementIndex) const
    {
        return get<T>(elementIndex);
    }

    /*! \brief Returns a pointer to the element with index \a elementIndex. */
    const T* operator[](const std::vector<size_t>& elementIndex) const
    {
        return get<T>(elementIndex);
    }

    /*! \brief Returns a pointer to the element with index \a elementIndex. */
    const T* operator[](const std::initializer_list<size_t>& elementIndex) const
    {
        return get<T>(elementIndex);
    }

    /*! \brief Returns a pointer to the element with index \a elementIndex. */
    T* operator[](size_t elementIndex)
    {
        return get<T>(elementIndex);
    }

    /*! \brief Returns a pointer to the element with index \a elementIndex. */
    T* operator[](const std::vector<size_t>& elementIndex)
    {
        return get<T>(elementIndex);
    }

    /*! \brief Returns a pointer to the element with index \a elementIndex. */
    T* operator[](const std::initializer_list<size_t>& elementIndex)
    {
        return get<T>(elementIndex);
    }

    /*@}*/
};

}

#endif
