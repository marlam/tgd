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

#include <gta/gta.hpp>

#include "io-gta.hpp"


namespace TAD {

FormatImportExportGTA::FormatImportExportGTA() :
    _f(nullptr),
    _arrayCount(-2)
{
}

FormatImportExportGTA::~FormatImportExportGTA()
{
    close();
}

Error FormatImportExportGTA::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        _f = stdin;
    else
        _f = fopen(fileName.c_str(), "rb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportGTA::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (fileName == "-")
        _f = stdout;
    else
        _f = fopen(fileName.c_str(), append ? "ab" : "wb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportGTA::close()
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

int FormatImportExportGTA::arrayCount()
{
    if (_arrayCount >= -1)
        return _arrayCount;

    if (!_f)
        return -1;

    // find offsets of all GTAs in the file
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
        try {
            gta::header hdr;
            hdr.read_from(_f);
            hdr.skip_data(_f);
        }
        catch (std::exception& e) {
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

static void copyFromGtaTagList(TagList& tl, const gta::taglist& gtatl)
{
    for (uintmax_t i = 0; i < gtatl.tags(); i++)
        tl.set(gtatl.name(i), gtatl.value(i));
}

static void copyToGtaTagList(gta::taglist& gtatl, const TagList& tl)
{
    for (auto it = tl.cbegin(); it != tl.cend(); it++)
        gtatl.set(it->first.c_str(), it->second.c_str());
}

static Type typeFromGtaType(gta::type gtat)
{
    Type t = uint8;
    switch (gtat) {
    case gta::int8:
        t = int8;
        break;
    case gta::uint8:
        t = uint8;
        break;
    case gta::int16:
        t = int16;
        break;
    case gta::uint16:
        t = uint16;
        break;
    case gta::int32:
        t = int32;
        break;
    case gta::uint32:
        t = uint32;
        break;
    case gta::int64:
        t = int64;
        break;
    case gta::uint64:
        t = uint64;
        break;
    case gta::float32:
        t = float32;
        break;
    case gta::float64:
        t = float64;
        break;
    default:
        break;
    }
    return t;
}

static gta::type typeToGtaType(Type t)
{
    gta::type gtat = gta::uint8;
    switch (t) {
    case int8:
        gtat = gta::int8;
        break;
    case uint8:
        gtat = gta::uint8;
        break;
    case int16:
        gtat = gta::int16;
        break;
    case uint16:
        gtat = gta::uint16;
        break;
    case int32:
        gtat = gta::int32;
        break;
    case uint32:
        gtat = gta::uint32;
        break;
    case int64:
        gtat = gta::int64;
        break;
    case uint64:
        gtat = gta::uint64;
        break;
    case float32:
        gtat = gta::float32;
        break;
    case float64:
        gtat = gta::float64;
        break;
    default:
        break;
    }
    return gtat;
}

ArrayContainer FormatImportExportGTA::readArray(Error* error, int arrayIndex)
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

    // Read the GTA header
    gta::header hdr;
    Error e = ErrorNone;
    try {
        hdr.read_from(_f);
        // Check if we can handle this GTA
        if (hdr.dimensions() > std::numeric_limits<size_t>::max()
                || hdr.components() > std::numeric_limits<size_t>::max()
                || hdr.data_size() > std::numeric_limits<size_t>::max()) {
            e = ErrorFeaturesUnsupported;
        } else if (hdr.components() > 0) {
            for (size_t c = 0; c < hdr.components(); c++) {
                if (hdr.component_type(c) == gta::blob
                        || hdr.component_type(c) == gta::cfloat32
                        || hdr.component_type(c) == gta::cfloat64
                        || hdr.component_type(c) == gta::int128
                        || hdr.component_type(c) == gta::uint128
                        || hdr.component_type(c) == gta::float128
                        || hdr.component_type(c) == gta::cfloat128
                        || (c > 1 && hdr.component_type(c) != hdr.component_type(0))) {
                    e = ErrorFeaturesUnsupported;
                    break;
                }
            }
        }
    }
    catch (gta::exception& exc) {
        switch (exc.result()) {
        case gta::system_error:
            e = ErrorSysErrno;
            break;
        case gta::ok:
        case gta::overflow:
        case gta::unexpected_eof:
        case gta::invalid_data:
        default:
            e = ErrorInvalidData;
            break;
        }
    }
    catch (std::exception& exc) {
        e = ErrorSysErrno;
    }
    if (e != ErrorNone) {
        *error = e;
        return ArrayContainer();
    }

    // Create the array
    std::vector<size_t> dimensions(hdr.dimensions());
    for (size_t d = 0; d < hdr.dimensions(); d++)
        dimensions[d] = hdr.dimension_size(d);
    Type t = uint8;
    if (hdr.components() > 0)
        t = typeFromGtaType(hdr.component_type(0));
    ArrayContainer r(dimensions, hdr.components(), t);

    // Read the data
    try {
        hdr.read_data(_f, r.data());
    }
    catch (gta::exception& exc) {
        switch (exc.result()) {
        case gta::system_error:
            e = ErrorSysErrno;
            break;
        case gta::ok:
        case gta::overflow:
        case gta::unexpected_eof:
        case gta::invalid_data:
        default:
            e = ErrorInvalidData;
            break;
        }
    }
    catch (std::exception& exc) {
        e = ErrorSysErrno;
    }
    if (e != ErrorNone) {
        *error = e;
        return ArrayContainer();
    }

    // Transfer meta data
    copyFromGtaTagList(r.globalTagList(), hdr.global_taglist());
    for (size_t d = 0; d < r.dimensionCount(); d++)
        copyFromGtaTagList(r.dimensionTagList(d), hdr.dimension_taglist(d));
    for (size_t c = 0; c < r.componentCount(); c++)
        copyFromGtaTagList(r.componentTagList(c), hdr.component_taglist(c));

    // Return the array
    return r;
}

bool FormatImportExportGTA::hasMore()
{
    int c = fgetc(_f);
    if (c == EOF) {
        return false;
    } else {
        ungetc(c, _f);
        return true;
    }
}

Error FormatImportExportGTA::writeArray(const ArrayContainer& array)
{
    Error e = ErrorNone;
    try {
        gta::header hdr;
        std::vector<uintmax_t> dimensions(array.dimensionCount());
        for (size_t d = 0; d < array.dimensionCount(); d++)
            dimensions[d] = array.dimension(d);
        hdr.set_dimensions(dimensions.size(), dimensions.data());
        std::vector<gta::type> components(array.componentCount(), typeToGtaType(array.componentType()));
        hdr.set_components(components.size(), components.data());
        copyToGtaTagList(hdr.global_taglist(), array.globalTagList());
        for (size_t d = 0; d < array.dimensionCount(); d++)
            copyToGtaTagList(hdr.dimension_taglist(d), array.dimensionTagList(d));
        for (size_t c = 0; c < array.componentCount(); c++)
            copyToGtaTagList(hdr.component_taglist(c), array.componentTagList(c));
        hdr.write_to(_f);
        hdr.write_data(_f, array.data());
    }
    catch (gta::exception& exc) {
        switch (exc.result()) {
        case gta::system_error:
            e = ErrorSysErrno;
            break;
        case gta::ok:
        case gta::overflow:
        case gta::unexpected_eof:
        case gta::invalid_data:
        default:
            e = ErrorInvalidData;
            break;
        }
    }
    catch (std::exception& exc) {
        e = ErrorSysErrno;
    }
    return e;
}

extern "C" FormatImportExport* FormatImportExportFactory_gta()
{
    return new FormatImportExportGTA();
}

}
