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

#include <cerrno>
#include <string>

#include "io-fits.hpp"


namespace TAD {

FormatImportExportFITS::FormatImportExportFITS() : _f(nullptr)
{
    close();
}

FormatImportExportFITS::~FormatImportExportFITS()
{
    close();
}

Error FormatImportExportFITS::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        return ErrorInvalidData;

    int status = 0;
    fits_open_file(&_f, fileName.c_str(), READONLY, &status);
    if (status) {
        return ErrorInvalidData;
    }

    return ErrorNone;
}

Error FormatImportExportFITS::openForWriting(const std::string&, bool, const TagList&)
{
    return ErrorFeaturesUnsupported;
}

void FormatImportExportFITS::close()
{
    if (_f) {
        int status = 0;
        fits_close_file(_f, &status);
        _f = nullptr;
    }
    _haveArrayCount = false;
    _imgHDUs.clear();
    _indexOfLastReadArray = -1;
}

int FormatImportExportFITS::arrayCount()
{
    if (!_haveArrayCount) {
        int status = 0;
        int hdunum;
        fits_get_num_hdus(_f, &hdunum, &status);
        for (int i = 1; i <= hdunum; i++) {
            int hdutype = ANY_HDU;
            fits_movabs_hdu(_f, i, &hdutype, &status);
            if (hdutype == IMAGE_HDU)
                _imgHDUs.push_back(i);
        }
        if (status)
            _imgHDUs.clear();
        _haveArrayCount = true;
    }
    return _imgHDUs.size();
}

ArrayContainer FormatImportExportFITS::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    int status = 0;
    if (arrayIndex >= 0) {
        fits_movabs_hdu(_f, _imgHDUs[arrayIndex], nullptr, &status);
    } else {
        fits_movabs_hdu(_f, _imgHDUs[_indexOfLastReadArray + 1], nullptr, &status);
    }
    if (status) {
        *error = ErrorSeekingNotSupported;
        return ArrayContainer();
    }

    int fitstype;
    fits_get_img_type(_f, &fitstype, &status);
    if (status) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    Type type;
    int fitsttype;
    if (fitstype == SBYTE_IMG) {
        type = int8;
        fitsttype = TSBYTE;
    } else if (fitstype == BYTE_IMG) {
        type = uint8;
        fitsttype = TBYTE;
    } else if (fitstype == SHORT_IMG) {
        type = int16;
        fitsttype = TSHORT;
    } else if (fitstype == USHORT_IMG) {
        type = uint16;
        fitsttype = TUSHORT;
    } else if (fitstype == LONG_IMG) {
        type = int32;
        fitsttype = TINT;
    } else if (fitstype == ULONG_IMG) {
        type = uint32;
        fitsttype = TUINT;
    } else if (fitstype == LONGLONG_IMG) {
        type = int64;
        fitsttype = TLONGLONG;
    } else if (fitstype == LONGLONG_IMG) {
        type = uint64;
        fitsttype = TULONGLONG;
    } else if (fitstype == FLOAT_IMG) {
        type = float32;
        fitsttype = TFLOAT;
    } else if (fitstype == DOUBLE_IMG) {
        type = float64;
        fitsttype = TDOUBLE;
    } else {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }

    int fitsdimcount;
    fits_get_img_dim(_f, &fitsdimcount, &status);
    if (status) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    if (fitsdimcount < 1) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }
    std::vector<long> fitsdims(fitsdimcount, -1);
    fits_get_img_size(_f, fitsdimcount, fitsdims.data(), &status);
    if (status) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    std::vector<size_t> dims(fitsdims.size());
    for (int i = 0; i < fitsdimcount; i++) {
        if (fitsdims[i] < 1) {
            *error = ErrorFeaturesUnsupported;
            return ArrayContainer();
        }
        dims[i] = fitsdims[i];
    }

    ArrayContainer r(dims, 1, type);
    for (int i = 0; i < fitsdimcount; i++)
        fitsdims[i] = 1;
    fits_read_pix(_f, fitsttype, fitsdims.data(), r.elementCount(),
            nullptr, r.data(), nullptr, &status);
    if (status) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    //reverseY(r);
    if (arrayIndex >= 0) {
        _indexOfLastReadArray = arrayIndex;
    } else {
        _indexOfLastReadArray++;
    }
    return r;
}

bool FormatImportExportFITS::hasMore()
{
    return (_indexOfLastReadArray < arrayCount() - 1);
}

Error FormatImportExportFITS::writeArray(const ArrayContainer&)
{
    return ErrorFeaturesUnsupported;
}

extern "C" FormatImportExport* FormatImportExportFactory_fits()
{
    return new FormatImportExportFITS();
}

}
