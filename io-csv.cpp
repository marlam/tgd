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

#include <cmath>
#include <limits>
#include <locale>

#include "io-csv.hpp"
#include "tools.hpp"


namespace TAD {

FormatImportExportCSV::FormatImportExportCSV() :
    _f(nullptr),
    _arrayCount(-2)
{
}

FormatImportExportCSV::~FormatImportExportCSV()
{
    close();
}

Error FormatImportExportCSV::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        _f = stdin;
    else
        _f = fopen(fileName.c_str(), "rb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportCSV::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (fileName == "-")
        _f = stdout;
    else
        _f = fopen(fileName.c_str(), append ? "ab" : "wb");
    return _f ? ErrorNone : ErrorSysErrno;
}

void FormatImportExportCSV::close()
{
    if (_f) {
        if (_f != stdin && _f != stdout) {
            fclose(_f);
        }
        _f = nullptr;
    }
}

int FormatImportExportCSV::arrayCount()
{
    if (_arrayCount >= -1)
        return _arrayCount;

    // find offsets of all CSVs in the file
    off_t curPos = ftello(_f);
    if (curPos < 0) {
        _arrayCount = -1;
        return _arrayCount;
    }
    rewind(_f);
    while (hasMore()) {
        off_t arrayPos = ftello(_f);
        if (arrayPos < 0) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
        Error err = ErrorNone;
        ArrayContainer tmp = readArray(&err, -1);
        if (err != ErrorNone) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
        _arrayOffsets.push_back(arrayPos);
        if (_arrayOffsets.size() == size_t(std::numeric_limits<int>::max()) && hasMore()) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
    }
    if (fseeko(_f, curPos, SEEK_SET) < 0) {
        _arrayOffsets.clear();
        _arrayCount = -1;
        return -1;
    }
    _arrayCount = _arrayOffsets.size();
    return _arrayCount;
}

static float readFloat(const char* s, char** nextChar = nullptr)
{
    char* endptr = nullptr;
    float v = std::strtof(s, &endptr);
    if (endptr != s) {
        if (nextChar)
            *nextChar = endptr;
        return v;
    } else {
        if (nextChar)
            *nextChar = nullptr;
        return std::numeric_limits<float>::quiet_NaN();
    }
}

ArrayContainer FormatImportExportCSV::readArray(Error* error, int arrayIndex)
{
    // Seek if necessary
    if (arrayIndex >= 0) {
        if (arrayCount() < 0) {
            *error = ErrorSeekingNotSupported;
            return ArrayContainer();
        }
        if (arrayIndex >= arrayCount()) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        if (fseeko(_f, _arrayOffsets[arrayIndex], SEEK_SET) < 0) {
            *error = ErrorSysErrno;
            return ArrayContainer();
        }
    }

    // Read the data (in "C" locale!)
    std::string localebak = std::string(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    bool determinedDelimiter = false;
    char delimiter = ',';
    std::vector<std::vector<std::vector<float>>> values;
    size_t maxElementsInLine = 0;
    size_t maxComponentsInElement = 0;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    bool allValuesAreFinite = true;
    bool allValuesAreInteger = true;
    bool haveFiniteValue = false;
    for (;;) {
        // read a line, without newline
        std::string line;
        int c;
        while ((c = std::fgetc(_f)) != EOF) {
            if (c == '\n')
                break;
            line.push_back(c);
        }
        if (line.length() > 0 && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);
        // are we finished?
        if (line.length() == 0) {
            break;
        }
        // parse one line of values
        values.push_back(std::vector<std::vector<float>>());
        size_t i = 0;
        for (;;) {
            while (line[i] == ' ' || (delimiter != '\t' && line[i] == '\t'))
                i++;
            if (line[i] == '\0') // line ends; we are done
                break;
            std::vector<float> element;
            if (line[i] == '"') { // quoted value
                size_t j = i + 1;
                size_t nextDQuote = line.find('"', i + 1);
                if (nextDQuote == std::string::npos) {
                    i = line.length();
                } else {
                    i = nextDQuote + 1;
                    i = line.find(delimiter, i);
                    if (i == std::string::npos)
                        i = line.length();
                }
                if (!determinedDelimiter) {
                    if (line[i] >= 33 && line[i] < 127)
                        delimiter = line[i];
                    determinedDelimiter = true;
                }
                for (;;) {
                    size_t last_j = j;
                    //fprintf(stderr, "j=%zu, nextDQuote=%zu, rest='%s'\n", j, nextDQuote, &(line[j]));
                    char* nextChar;
                    float value = readFloat(&(line[j]), &nextChar);
                    //fprintf(stderr, "v=%g\n", value);
                    element.push_back(value);
                    if (nextChar)
                        j = nextChar - line.c_str();
                    j = line.find(delimiter, j);
                    if (j == std::string::npos || j >= nextDQuote)
                        break;
                    if (j == last_j)
                        j++;
                }
            } else { // unquoted value
                //fprintf(stderr, "i=%zu, rest='%s'\n", i, &(line[i]));
                char* nextChar;
                float value = readFloat(&(line[i]), &nextChar);
                //fprintf(stderr, "v=%g\n", value);
                element.push_back(value);
                if (!determinedDelimiter && nextChar) {
                    if (*nextChar >= 33 && *nextChar < 127)
                        delimiter = *nextChar;
                    determinedDelimiter = true;
                }
                if (nextChar)
                    i = nextChar - line.c_str();
                i = line.find(delimiter, i);
                if (i == std::string::npos)
                    i = line.length();
            }
            // check element properties and store it
            for (size_t k = 0; k < element.size(); k++) {
                if (!std::isfinite(element[k])) {
                    allValuesAreFinite = false;
                    allValuesAreInteger = false;
                } else {
                    if (!haveFiniteValue) {
                        minValue = element[k];
                        maxValue = element[k];
                        haveFiniteValue = true;
                    } else if (element[k] < minValue) {
                        minValue = element[k];
                    } else if (element[k] > maxValue) {
                        maxValue = element[k];
                    }
                    if (std::nearbyint(element[k]) != element[k]) {
                        allValuesAreInteger = false;
                    }
                }
            }
            values.back().push_back(element);
            if (values.back().back().size() > maxComponentsInElement)
                maxComponentsInElement = values.back().back().size();
            // skip to next value
            if (i < line.length())
                i++;
        }
        if (values.back().size() > maxElementsInLine)
            maxElementsInLine = values.back().size();
    }
    setlocale(LC_NUMERIC, localebak.c_str());
    if (std::ferror(_f)) {
        *error = ErrorSysErrno;
        return ArrayContainer();
    }

    // Store the data in a floating point array
    ArrayContainer r;
    if (values.size() == 0 || maxElementsInLine == 0 || maxComponentsInElement == 0) {
        // do nothing
    } else if (values.size() == 1) {
        Array<float> rf({ maxElementsInLine }, maxComponentsInElement);
        for (size_t e = 0; e < maxElementsInLine; e++) {
            std::memcpy(rf.get(e), values[0][e].data(), values[0][e].size() * sizeof(float));
            if (values[0][e].size() < maxComponentsInElement)
                allValuesAreFinite = false;
            for (size_t c = values[0][e].size(); c < maxComponentsInElement; c++)
                rf[e][c] = std::numeric_limits<float>::quiet_NaN();
        }
        r = rf;
    } else {
        size_t height = values.size();
        size_t width = maxElementsInLine;
        size_t comps = maxComponentsInElement;
        Array<float> rf({ width, height }, comps);
        for (size_t y = 0; y < height; y++) {
            size_t ry = height - 1 - y;
            for (size_t x = 0; x < values[ry].size(); x++) {
                std::memcpy(rf.get({ x, y }), values[ry][x].data(), values[ry][x].size() * sizeof(float));
                if (values[ry][x].size() < comps)
                    allValuesAreFinite = false;
                for (size_t c = values[ry][x].size(); c < comps; c++)
                    rf[{ x, y }][c] = std::numeric_limits<float>::quiet_NaN();
            }
            if (values[ry].size() < width)
                allValuesAreFinite = false;
            for (size_t x = values[ry].size(); x < width; x++) {
                for (size_t c = 0; c < comps; c++)
                    rf[{ x, y }][c] = std::numeric_limits<float>::quiet_NaN();
            }
        }
        r = rf;
    }

    // Convert the data to a simple integer type if possible
    if (allValuesAreFinite && allValuesAreInteger) {
        if (minValue >= 0) {
            if (maxValue <= std::numeric_limits<uint8_t>::max()) {
                r = convert(r, uint8);
            } else if (maxValue <= std::numeric_limits<uint16_t>::max()) {
                r = convert(r, uint16);
            }
        } else {
            if (minValue >= std::numeric_limits<int8_t>::min() && maxValue <= std::numeric_limits<int8_t>::max()) {
                r = convert(r, int8);
            } else if (minValue >= std::numeric_limits<int16_t>::min() && maxValue <= std::numeric_limits<int16_t>::max()) {
                r = convert(r, int16);
            }
        }
    }

    // Return the result
    return r;
}

bool FormatImportExportCSV::hasMore()
{
    int c = fgetc(_f);
    if (c == EOF) {
        return false;
    } else {
        ungetc(c, _f);
        return true;
    }
}

template<typename T>
std::string valueToString(T value)
{
    return std::to_string(value);
}
template<> std::string valueToString<float>(float value)
{
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%.7g", value);
    return buf;
}
template<> std::string valueToString<double>(double value)
{
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%.15g", value);
    return buf;
}

template<typename T>
std::string rowToString(const T* data, size_t ne, size_t nc)
{
    std::string s;
    for (size_t e = 0; e < ne; e++) {
        if (nc == 1) {
            s += valueToString(data[e]);
        } else {
            s += '"';
            for (size_t c = 0; c < nc; c++) {
                s += valueToString(data[e * nc + c]);
                if (c + 1 < nc)
                    s += ',';
            }
            s += '"';
        }
        if (e + 1 < ne)
            s += ',';
    }
    s += "\r\n";
    return s;
}

static std::string rowToString(const void* data, Type type, size_t ne, size_t nc)
{
    std::string s;
    switch (type) {
    case int8:
        s = rowToString<int8_t>(static_cast<const int8_t*>(data), ne, nc);
        break;
    case uint8:
        s = rowToString<uint8_t>(static_cast<const uint8_t*>(data), ne, nc);
        break;
    case int16:
        s = rowToString<int16_t>(static_cast<const int16_t*>(data), ne, nc);
        break;
    case uint16:
        s = rowToString<uint16_t>(static_cast<const uint16_t*>(data), ne, nc);
        break;
    case int32:
        s = rowToString<int32_t>(static_cast<const int32_t*>(data), ne, nc);
        break;
    case uint32:
        s = rowToString<uint32_t>(static_cast<const uint32_t*>(data), ne, nc);
        break;
    case int64:
        s = rowToString<int64_t>(static_cast<const int64_t*>(data), ne, nc);
        break;
    case uint64:
        s = rowToString<uint64_t>(static_cast<const uint64_t*>(data), ne, nc);
        break;
    case float32:
        s = rowToString<float>(static_cast<const float*>(data), ne, nc);
        break;
    case float64:
        s = rowToString<double>(static_cast<const double*>(data), ne, nc);
        break;
    }
    return s;
}

Error FormatImportExportCSV::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() < 1 || array.dimensionCount() > 2
            || array.dimension(0) < 1 || (array.dimensionCount() == 2 && array.dimension(1) < 1)
            || array.componentCount() < 1) {
        return ErrorFeaturesUnsupported;
    }

    std::string localebak = std::string(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    if (array.dimensionCount() == 1) {
        std::fputs(rowToString(array.data(), array.componentType(),
                    array.elementCount(), array.componentCount()).c_str(), _f);
    } else {
        for (size_t row = 0; row < array.dimension(1); row++) {
            size_t y = array.dimension(1) - 1 - row;
            std::fputs(rowToString(array.get(y * array.dimension(0)), array.componentType(),
                        array.dimension(0), array.componentCount()).c_str(), _f);
        }
    }
    setlocale(LC_NUMERIC, localebak.c_str());
    std::fputs("\r\n", _f);
    if (std::fflush(_f) != 0) {
        return ErrorSysErrno;
    }

    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_csv()
{
    return new FormatImportExportCSV();
}

}
