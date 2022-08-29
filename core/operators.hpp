/*
 * Copyright (C) 2018, 2019, 2020, 2021, 2022
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

#ifndef TGD_OPERATORS_HPP
#define TGD_OPERATORS_HPP

/**
 * \file operators.hpp
 * \brief Overloading of common operators and functions for arrays.
 */

#include <algorithm>

#include "array.hpp"
#include "foreach.hpp"

namespace TGD {

template<typename T>
Array<T> operator-(const Array<T>& a)
{
    return forEachComponent(a, [] (T v) -> T { return -v; });
}

template<typename T>
Array<T> abs(const Array<T>& a)
{
    return forEachComponent(a, [] (T v) -> T { return std::abs(v); });
}

template<typename T>
Array<T> absInplace(Array<T>& a)
{
    return forEachComponentInplace(a, [] (T v) -> T { return std::abs(v); });
}

template<typename T>
Array<T> operator+(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u + v; });
}

template<typename T>
Array<T> operator+(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u + v; });
}

template<typename T>
Array<T> operator+=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u + v; });
}

template<typename T>
Array<T> operator+=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u + v; });
}

template<typename T>
Array<T> operator-(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u - v; });
}

template<typename T>
Array<T> operator-(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u - v; });
}

template<typename T>
Array<T> operator-=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u - v; });
}

template<typename T>
Array<T> operator-=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u - v; });
}

template<typename T>
Array<T> operator*(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u * v; });
}

template<typename T>
Array<T> operator*(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u * v; });
}

template<typename T>
Array<T> operator*=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u * v; });
}

template<typename T>
Array<T> operator*=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u * v; });
}

template<typename T>
Array<T> operator/(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u / v; });
}

template<typename T>
Array<T> operator/(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u / v; });
}

template<typename T>
Array<T> operator/=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u / v; });
}

template<typename T>
Array<T> operator/=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u / v; });
}

template<typename T>
Array<T> operator%(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u % v; });
}

template<typename T>
Array<T> operator%(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u % v; });
}

template<typename T>
Array<T> operator%=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u % v; });
}

template<typename T>
Array<T> operator%=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u % v; });
}

template<typename T>
Array<T> operator&(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u & v; });
}

template<typename T>
Array<T> operator&(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u & v; });
}

template<typename T>
Array<T> operator&=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u & v; });
}

template<typename T>
Array<T> operator&=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u & v; });
}

template<typename T>
Array<T> operator|(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u | v; });
}

template<typename T>
Array<T> operator|(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u | v; });
}

template<typename T>
Array<T> operator|=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u | v; });
}

template<typename T>
Array<T> operator|=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u | v; });
}

template<typename T>
Array<T> operator^(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u ^ v; });
}

template<typename T>
Array<T> operator^(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return u ^ v; });
}

template<typename T>
Array<T> operator^=(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u ^ v; });
}

template<typename T>
Array<T> operator^=(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return u ^ v; });
}

template<typename T>
Array<T> min(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return std::min(u, v); });
}

template<typename T>
Array<T> min(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return std::min(u, v); });
}

template<typename T>
Array<T> minInplace(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return std::min(u, v); });
}

template<typename T>
Array<T> minInplace(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return std::min(u, v); });
}

template<typename T>
Array<T> max(const Array<T>& a, const Array<T>& b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return std::max(u, v); });
}

template<typename T>
Array<T> max(const Array<T>& a, T b)
{
    return forEachComponent(a, b, [] (T u, T v) -> T { return std::max(u, v); });
}

template<typename T>
Array<T> maxInplace(Array<T>& a, const Array<T>& b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return std::max(u, v); });
}

template<typename T>
Array<T> maxInplace(Array<T>& a, T b)
{
    return forEachComponentInplace(a, b, [] (T u, T v) -> T { return std::max(u, v); });
}

}
#endif
