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

#ifndef TAD_TAGLIST_HPP
#define TAD_TAGLIST_HPP

/**
 * \file taglist.hpp
 * \brief libtad metadata management via lists of tags
 */

#include <cstdlib>
#include <cerrno>
#include <string>
#include <limits>
#include <map>
#include <initializer_list>

namespace TAD {

/*! \brief A tag list to store key/value pairs, where both key and value are strings. */
class TagList
{
private:
    std::map<std::string, std::string> _tags;

    // Conversion helper: generic string to type function
    template<typename T> static T strtox(const char* nptr, char** endptr, int base)
    {
        if (std::is_same<T, signed char>::value
                || std::is_same<T, short>::value
                || std::is_same<T, int>::value) {
            long x = std::strtol(nptr, endptr, base);
            if (x < std::numeric_limits<T>::min()) {
                x = std::numeric_limits<T>::min();
                errno = ERANGE;
            } else if (x > std::numeric_limits<T>::max()) {
                x = std::numeric_limits<T>::max();
                errno = ERANGE;
            }
            return x;
        } else if (std::is_same<T, unsigned char>::value
                || std::is_same<T, unsigned short>::value
                || std::is_same<T, unsigned int>::value) {
            unsigned long x = std::strtoul(nptr, endptr, base);
            if (x > std::numeric_limits<T>::max()) {
                x = std::numeric_limits<T>::max();
                errno = ERANGE;
            }
            return x;
        } else if (std::is_same<T, long>::value) {
            return std::strtol(nptr, endptr, base);
        } else if (std::is_same<T, unsigned long>::value) {
            return std::strtoul(nptr, endptr, base);
        } else if (std::is_same<T, long long>::value) {
            return std::strtoll(nptr, endptr, base);
        } else if (std::is_same<T, unsigned long long>::value) {
            return std::strtoull(nptr, endptr, base);
        } else if (std::is_same<T, float>::value) {
            return std::strtof(nptr, endptr);
        } else if (std::is_same<T, double>::value) {
            return std::strtod(nptr, endptr);
        } else if (std::is_same<T, long double>::value) {
            return std::strtold(nptr, endptr);
        } else {
            return 0;
        }
    }

    // Conversion helper: return whether conversion worked
    template<typename T>
    bool valueHelper(const std::string& key, T* result) const
    {
        if (!contains(key))
            return false;
        T r;
        const std::string& str = value(key, std::string());
        char* p;
        int errnobak = errno;
        errno = 0;
        r = strtox<T>(str.c_str(), &p, 0);
        if (p == str.c_str() || errno == ERANGE || *p != '\0') {
            errno = errnobak;
            return false;
        }
        errno = errnobak;
        *result = r;
        return true;
    }

    // Conversion helper: return value, fall back on default value if necessary
    template<typename T>
    T valueHelper(const std::string& key, T defaultValue) const
    {
        T r;
        bool ok = value(key, &r);
        return (ok ? r : defaultValue);
    }

public:
    /*! \brief Constructor. */
    TagList() {}
    /*! \brief Construct a tag list from a list of key/value pairs */
    TagList(std::initializer_list<std::pair<std::string, std::string>> tags)
    {
        const std::pair<std::string, std::string>* tagData = tags.begin();
        for (size_t t = 0; t < tags.size(); t++) {
            set(tagData[t].first, tagData[t].second);
        }
    }

    /*! \brief Returns the number of key/value pairs in this tag list. */
    size_t size() const { return _tags.size(); }
    /*! \brief Iterator, for accessing all key/value pairs in this tag list. */
    std::map<std::string, std::string>::const_iterator cbegin() const noexcept { return _tags.cbegin(); }
    /*! \brief Iterator, for accessing all key/value pairs in this tag list. */
    std::map<std::string, std::string>::const_iterator cend() const noexcept { return _tags.cend(); }

    /*! \brief Clear the tag list. */
    void clear()
    {
        _tags.clear();
    }

    /*! \brief Set a \a key to a \a value. */
    void set(const std::string& key, const std::string& value)
    {
        _tags.insert(std::make_pair(key, value));
    }

    /*! \brief Unset a \a key. */
    void unset(const std::string& key)
    {
        _tags.erase(key);
    }

    /*! \brief Check if this list contains a given \a key. */
    bool contains(const std::string& key) const
    {
        return (_tags.find(key) != _tags.end());
    }

    /*! \brief Return the value to a given \a key, or the \a defaultValue if the \a key is not set. */
    const std::string& value(const std::string& key, const std::string& defaultValue = std::string()) const
    {
        auto it = _tags.find(key);
        return (it == _tags.end() ? defaultValue : it->second);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, signed char* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    signed char value(const std::string& key, signed char defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, unsigned char* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    unsigned char value(const std::string& key, unsigned char defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, signed short* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    signed short value(const std::string& key, signed short defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, unsigned short* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    unsigned short value(const std::string& key, unsigned short defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, signed int* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    signed int value(const std::string& key, signed int defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, unsigned int* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    unsigned int value(const std::string& key, unsigned int defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, signed long* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    signed long value(const std::string& key, signed long defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, unsigned long* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    unsigned long value(const std::string& key, unsigned long defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, signed long long* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    signed long long value(const std::string& key, signed long long defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, unsigned long long* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    unsigned long long value(const std::string& key, unsigned long long defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, float* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    float value(const std::string& key, float defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, double* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    double value(const std::string& key, double defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }

    /*! \brief Return the value to a given \a key in \a result and return true, or return false if the key is not set. */
    bool value(const std::string& key, long double* result) const
    {
        return valueHelper(key, result);
    }
    /*! \brief Return the value to a given \a key, falling back to \a defaultValue if necessary. */
    long double value(const std::string& key, long double defaultValue) const
    {
        return valueHelper(key, defaultValue);
    }
};

}

#endif
