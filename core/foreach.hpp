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

#ifndef TGD_FOREACH_HPP
#define TGD_FOREACH_HPP

/**
 * \file foreach.hpp
 * \brief Apply functions to each component or element of an array.
 */

#include "array.hpp"

namespace TGD {

/*! \brief Apply \a func to all components in array \a a. */
template <typename T, typename FUNC>
Array<T> forEachComponent(const Array<T>& a, FUNC func)
{
    ArrayDescription rd(a);
    Array<T> r(rd);
    auto itr = r.componentBegin();
    for (auto ita = a.constComponentBegin(); ita != a.constComponentEnd(); ++ita, ++itr)
        *itr = func(*ita);
    return r;
}

/*! \brief Apply \a func to all components in array \a a, in place. */
template <typename T, typename FUNC>
Array<T>& forEachComponentInplace(Array<T>& a, FUNC func)
{
    for (auto ita = a.componentBegin(); ita != a.componentEnd(); ++ita)
        *ita = func(*ita);
    return a;
}

/*! \brief Apply \a func to all components in array \a a using value \a b. */
template <typename T, typename FUNC>
Array<T> forEachComponent(const Array<T>& a, T b, FUNC func)
{
    ArrayDescription rd(a);
    Array<T> r(rd);
    auto itr = r.componentBegin();
    for (auto ita = a.constComponentBegin(); ita != a.constComponentEnd(); ++ita, ++itr)
        *itr = func(*ita, b);
    return r;
}

/*! \brief Apply \a func to all components in array \a a using value \a b, in place. */
template <typename T, typename FUNC>
Array<T>& forEachComponentInplace(Array<T>& a, T b, FUNC func)
{
    for (auto ita = a.componentBegin(); ita != a.componentEnd(); ++ita)
        *ita = func(*ita, b);
    return a;
}

/*! \brief Apply \a func to all components in arrays \a a and \a b. */
template <typename T, typename FUNC>
Array<T> forEachComponent(const Array<T>& a, const Array<T>& b, FUNC func)
{
    assert(a.isCompatible(b));
    ArrayDescription rd(a);
    Array<T> r(rd);
    auto itb = b.constComponentBegin();
    auto itr = r.componentBegin();
    for (auto ita = a.constComponentBegin(); ita != a.constComponentEnd(); ++ita, ++itb, ++itr)
        *itr = func(*ita, *itb);
    return r;
}

/*! \brief Apply \a func to all components in arrays \a a and \a b, in place. */
template <typename T, typename FUNC>
Array<T>& forEachComponentInplace(Array<T>& a, const Array<T>& b, FUNC func)
{
    assert(a.isCompatible(b));
    auto itb = b.constComponentBegin();
    for (auto ita = a.componentBegin(); ita != a.componentEnd(); ++ita, ++itb)
        *ita = func(*ita, *itb);
    return a;
}

/*! \brief Apply \a func to all elements in array \a a. */
template <typename T, typename FUNC>
Array<T> forEachElement(const Array<T>& a, FUNC func)
{
    ArrayDescription rd(a);
    Array<T> r(rd);
    auto itr = r.elementBegin();
    for (auto ita = a.constElementBegin(); ita != a.constElementEnd(); ++ita, ++itr)
        func(*itr, *ita);
    return r;
}

/*! \brief Apply \a func to all elements in array \a a, in place. */
template <typename T, typename FUNC>
Array<T>& forEachElementInplace(Array<T>& a, FUNC func)
{
    for (auto ita = a.elementBegin(); ita != a.elementEnd(); ++ita)
        func(*ita);
    return a;
}

/*! \brief Apply \a func to all elements in array \a a using element \a b. */
template <typename T, typename FUNC>
Array<T> forEachElement(const Array<T>& a, const T* b, FUNC func)
{
    ArrayDescription rd(a);
    Array<T> r(rd);
    auto itr = r.elementBegin();
    for (auto ita = a.constElementBegin(); ita != a.constElementEnd(); ++ita, ++itr)
        func(*itr, *ita, b);
    return r;
}

/*! \brief Apply \a func to all elements in array \a a using element \a b, in place. */
template <typename T, typename FUNC>
Array<T>& forEachElementInplace(Array<T>& a, const T* b, FUNC func)
{
    for (auto ita = a.elementBegin(); ita != a.elementEnd(); ++ita)
        func(*ita, b);
    return a;
}

/*! \brief Apply \a func to all elements in arrays \a a and \a b. */
template <typename T, typename FUNC>
Array<T> forEachElement(const Array<T>& a, const Array<T>& b, FUNC func)
{
    assert(a.isCompatible(b));
    ArrayDescription rd(a);
    Array<T> r(rd);
    auto itb = b.constElementBegin();
    auto itr = r.elementBegin();
    for (auto ita = a.constElementBegin(); ita != a.constElementEnd(); ++ita, ++itb, ++itr)
        func(*itr, *ita, *itb);
    return r;
}

/*! \brief Apply \a func to all elements in arrays \a a and \a b, in place. */
template <typename T, typename FUNC>
Array<T>& forEachElementInplace(Array<T>& a, const Array<T>& b, FUNC func)
{
    assert(a.isCompatible(b));
    auto itb = b.constElementBegin();
    for (auto ita = a.elementBegin(); ita != a.elementEnd(); ++ita, ++itb)
        func(*ita, *itb);
    return a;
}

}
#endif
