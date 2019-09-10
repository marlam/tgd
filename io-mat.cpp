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

#include <cstdio>

#include "io-mat.hpp"
#include "io-utils.hpp"


namespace TAD {

FormatImportExportMAT::FormatImportExportMAT() : _mat(nullptr), _counter(0)
{
}

FormatImportExportMAT::~FormatImportExportMAT()
{
    close();
}

Error FormatImportExportMAT::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-") {
        return ErrorInvalidData;
    } else {
        FILE* f = fopen(fileName.c_str(), "rb");
        if (!f) {
            return ErrorSysErrno;
        }
        fclose(f);
        _mat = Mat_Open(fileName.c_str(), MAT_ACC_RDONLY);
        if (!_mat) {
            return ErrorInvalidData;
        }
        return ErrorNone;
    }
}

Error FormatImportExportMAT::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (append)
        return ErrorFeaturesUnsupported;
    if (fileName == "-") {
        return ErrorInvalidData;
    } else {
        FILE* f = fopen(fileName.c_str(), "wb");
        if (!f) {
            return ErrorSysErrno;
        }
        fclose(f);
        remove(fileName.c_str());
        _mat = Mat_Create(fileName.c_str(),
                "MATLAB 5.0 MAT-file, Platform: generic, Created By: libtadio-mat using libmatio");
        if (!_mat) {
            return ErrorLibrary;
        }
        return ErrorNone;
    }
}

void FormatImportExportMAT::close()
{
    if (_mat) {
        Mat_Close(_mat);
        _mat = nullptr;
    }
}

int FormatImportExportMAT::arrayCount()
{
    if (_varNames.size() == 0) {
        matvar_t* matvar;
        while ((matvar = Mat_VarReadNextInfo(_mat))) {
            _varNames.push_back(matvar->name);
            Mat_VarFree(matvar);
        }
        Mat_Rewind(_mat);
    }
    return _varNames.size();
}

ArrayContainer FormatImportExportMAT::readArray(Error* error, int arrayIndex)
{
    matvar_t* matvar;
    if (arrayIndex >= 0) {
        if (arrayIndex >= arrayCount()) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        matvar = Mat_VarRead(_mat, _varNames[arrayIndex].c_str());
    } else {
        matvar = Mat_VarReadNext(_mat);
        _counter++;
    }
    if (!matvar) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    if (matvar->isComplex) {
        *error = ErrorFormatUnsupported;
        return ArrayContainer();
    }
    Type type;
    if (matvar->data_type == MAT_T_INT8) {
        type = int8;
    } else if (matvar->data_type == MAT_T_UINT8) {
        type = uint8;
    } else if (matvar->data_type == MAT_T_INT16) {
        type = int16;
    } else if (matvar->data_type == MAT_T_UINT16) {
        type = uint16;
    } else if (matvar->data_type == MAT_T_INT32) {
        type = int32;
    } else if (matvar->data_type == MAT_T_UINT32) {
        type = uint32;
    } else if (matvar->data_type == MAT_T_INT64) {
        type = int64;
    } else if (matvar->data_type == MAT_T_UINT64) {
        type = uint64;
    } else if (matvar->data_type == MAT_T_SINGLE) {
        type = float32;
    } else if (matvar->data_type == MAT_T_DOUBLE) {
        type = float64;
    } else {
        *error = ErrorFormatUnsupported;
        return ArrayContainer();
    }
    std::vector<size_t> dimensions(matvar->rank);
    for (size_t i = 0; i < dimensions.size(); i++) {
        dimensions[i] = matvar->dims[i];
        if (dimensions[i] < 1) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
    }
    ArrayContainer r = reorderMatlabInputData(dimensions, type, matvar->data);
    if (matvar->name && matvar->name[0] != '\0') {
        r.globalTagList().set("NAME", matvar->name);
    }
    Mat_VarFree(matvar);
    return r;
}

bool FormatImportExportMAT::hasMore()
{
    return (_counter < arrayCount());
}

Error FormatImportExportMAT::writeArray(const ArrayContainer& array)
{
    enum matio_classes classType = MAT_C_INT8;
    enum matio_types dataType = MAT_T_INT8;
    switch (array.componentType()) {
    case int8:
        classType = MAT_C_INT8;
        dataType = MAT_T_INT8;
        break;
    case uint8:
        classType = MAT_C_UINT8;
        dataType = MAT_T_UINT8;
        break;
    case int16:
        classType = MAT_C_INT16;
        dataType = MAT_T_INT16;
        break;
    case uint16:
        classType = MAT_C_UINT16;
        dataType = MAT_T_UINT16;
        break;
    case int32:
        classType = MAT_C_INT32;
        dataType = MAT_T_INT32;
        break;
    case uint32:
        classType = MAT_C_UINT32;
        dataType = MAT_T_UINT32;
        break;
    case int64:
        classType = MAT_C_INT64;
        dataType = MAT_T_INT64;
        break;
    case uint64:
        classType = MAT_C_UINT64;
        dataType = MAT_T_UINT64;
        break;
    case float32:
        classType = MAT_C_SINGLE;
        dataType = MAT_T_SINGLE;
        break;
    case float64:
        classType = MAT_C_DOUBLE;
        dataType = MAT_T_DOUBLE;
        break;
    }
    std::string name = array.globalTagList().value("NAME");
    if (name.size() == 0)
        name = std::string("TAD") + std::to_string(_counter);
    _counter++;
    ArrayContainer dataArray = reorderMatlabOutputData(array);
    matvar_t* matvar = Mat_VarCreate(name.c_str(), classType, dataType,
            dataArray.dimensionCount(), const_cast<size_t*>(dataArray.dimensions().data()),
            dataArray.data(), MAT_F_DONT_COPY_DATA);
    if (!matvar || Mat_VarWrite(_mat, matvar, MAT_COMPRESSION_NONE) != 0) {
        return ErrorLibrary;
    }
    Mat_VarFree(matvar);
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_mat()
{
    return new FormatImportExportMAT();
}

}
