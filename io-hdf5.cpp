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

#include "io-hdf5.hpp"
#include "io-utils.hpp"


namespace TAD {

FormatImportExportHDF5::FormatImportExportHDF5() : _counter(0)
{
    H5::Exception::dontPrint();
}

FormatImportExportHDF5::~FormatImportExportHDF5()
{
    close();
}

Error FormatImportExportHDF5::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-") {
        return ErrorInvalidData;
    } else {
        FILE* f = fopen(fileName.c_str(), "rb");
        if (!f) {
            return ErrorSysErrno;
        }
        fclose(f);
        try {
            _f.openFile(fileName.c_str(), H5F_ACC_RDONLY);
        }
        catch (H5::Exception& error) {
            return ErrorLibrary;
        }
        return ErrorNone;
    }
}

Error FormatImportExportHDF5::openForWriting(const std::string& fileName, bool append, const TagList&)
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
        try {
            _f = H5::H5File(fileName.c_str(), H5F_ACC_TRUNC);
        }
        catch (H5::Exception& error) {
            fprintf(stderr, "%s: %s\n", fileName.c_str(), error.getCDetailMsg());
            return ErrorLibrary;
        }
        return ErrorNone;
    }
}

void FormatImportExportHDF5::close()
{
    try {
        _f.close();
    }
    catch (H5::Exception& error) {
    }
}

static herr_t datasetVisitor(hid_t /* loc_id */, const char* name, const H5L_info_t* /* linfo */, void* opdata)
{
    std::vector<std::string>* datasetNames = reinterpret_cast<std::vector<std::string>*>(opdata);
    datasetNames->push_back(name);
    return 0;
}

int FormatImportExportHDF5::arrayCount()
{
    if (_datasetNames.size() == 0) {
        H5Literate(_f.getId(), H5_INDEX_NAME, H5_ITER_INC, NULL, datasetVisitor, &_datasetNames);
    }
    return _datasetNames.size();
}

ArrayContainer FormatImportExportHDF5::readArray(Error* error, int arrayIndex)
{
    int datasetIndex;
    if (arrayIndex >= 0) {
        if (arrayIndex >= arrayCount()) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        datasetIndex = arrayIndex;
    } else {
        datasetIndex = _counter++;
        if (datasetIndex >= arrayCount()) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
    }
    const std::string& datasetName = _datasetNames[datasetIndex];
    H5::DataSet dataset;
    H5::DataType datatype;
    H5T_class_t typeclass;
    try {
        dataset = _f.openDataSet(datasetName.c_str());
        datatype = dataset.getDataType();
        typeclass = dataset.getTypeClass();
    }
    catch (H5::Exception& e) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    H5::DataType type;
    Type rType;
    if (typeclass == H5T_INTEGER) {
        H5::IntType inttype = dataset.getIntType();
        if (inttype.getSign() == H5T_SGN_NONE && datatype.getSize() == 1) {
            type = H5::PredType::NATIVE_UINT8;
            rType = uint8;
        } else if (inttype.getSign() == H5T_SGN_NONE && datatype.getSize() == 2) {
            type = H5::PredType::NATIVE_UINT16;
            rType = uint16;
        } else if (inttype.getSign() == H5T_SGN_NONE && datatype.getSize() == 4) {
            type = H5::PredType::NATIVE_UINT32;
            rType = uint32;
        } else if (inttype.getSign() == H5T_SGN_NONE && datatype.getSize() == 8) {
            type = H5::PredType::NATIVE_UINT64;
            rType = uint64;
        } else if (inttype.getSign() != H5T_SGN_NONE && datatype.getSize() == 1) {
            type = H5::PredType::NATIVE_INT8;
            rType = int8;
        } else if (inttype.getSign() != H5T_SGN_NONE && datatype.getSize() == 2) {
            type = H5::PredType::NATIVE_INT16;
            rType = int16;
        } else if (inttype.getSign() != H5T_SGN_NONE && datatype.getSize() == 4) {
            type = H5::PredType::NATIVE_INT32;
            rType = int32;
        } else if (inttype.getSign() != H5T_SGN_NONE && datatype.getSize() == 8) {
            type = H5::PredType::NATIVE_INT64;
            rType = int64;
        } else {
            *error = ErrorFeaturesUnsupported;
            return ArrayContainer();
        }
    } else if (typeclass == H5T_FLOAT) {
        if (datatype.getSize() == 4) {
            type = H5::PredType::NATIVE_FLOAT;
            rType = float32;
        } else if (datatype.getSize() == 8) {
            type = H5::PredType::NATIVE_DOUBLE;
            rType = float64;
        } else {
            *error = ErrorFeaturesUnsupported;
            return ArrayContainer();
        }
    } else {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }
    H5::DataSpace dataspace = dataset.getSpace();
    int dimCount = dataspace.getSimpleExtentNdims();
    if (dimCount < 1) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }
    std::vector<hsize_t> hdims(dimCount);
    dataspace.getSimpleExtentDims(hdims.data(), nullptr);
    std::vector<size_t> dims(dimCount);
    for (size_t i = 0; i < dims.size(); i++)
        dims[i] = hdims[dims.size() - 1 - i];
    ArrayContainer dataArray(dims, 1, rType);
    try {
        dataset.read(dataArray.data(), type, dataspace, dataspace);
    }
    catch (H5::Exception& e) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    ArrayContainer r = reorderMatlabInputData(dims, rType, dataArray.data());
    return r;
}

bool FormatImportExportHDF5::hasMore()
{
    return (_counter < arrayCount());
}

Error FormatImportExportHDF5::writeArray(const ArrayContainer& array)
{
    std::string counterString = std::to_string(_counter);
    size_t leadingZeros = 0;
    if (_counter < 10)
        leadingZeros++;
    if (_counter < 100)
        leadingZeros++;
    if (_counter < 1000)
        leadingZeros++;
    counterString.insert(0, leadingZeros, '0');
    _counter++;
    std::string datasetname = std::string("ARRAY_") + counterString;
    H5::DataType type;
    switch (array.componentType()) {
    case TAD::int8:
        type = H5::IntType(H5::PredType::NATIVE_INT8);
        break;
    case TAD::uint8:
        type = H5::IntType(H5::PredType::NATIVE_UINT8);
        break;
    case TAD::int16:
        type = H5::IntType(H5::PredType::NATIVE_INT16);
        break;
    case TAD::uint16:
        type = H5::IntType(H5::PredType::NATIVE_UINT16);
        break;
    case TAD::int32:
        type = H5::IntType(H5::PredType::NATIVE_INT32);
        break;
    case TAD::uint32:
        type = H5::IntType(H5::PredType::NATIVE_UINT32);
        break;
    case TAD::int64:
        type = H5::IntType(H5::PredType::NATIVE_INT64);
        break;
    case TAD::uint64:
        type = H5::IntType(H5::PredType::NATIVE_UINT64);
        break;
    case TAD::float32:
        type = H5::FloatType(H5::PredType::NATIVE_FLOAT);
        break;
    case TAD::float64:
        type = H5::FloatType(H5::PredType::NATIVE_DOUBLE);
        break;
    }
    ArrayContainer dataArray = reorderMatlabOutputData(array);
    std::vector<hsize_t> dims(dataArray.dimensionCount());
    dims[0] = dataArray.dimension(dataArray.dimensionCount() - 1);
    for (size_t i = 1; i < dataArray.dimensionCount(); i++) {
        dims[i] = array.dimension(i - 1);
    }
    H5::DataSpace dataspace(dims.size(), dims.data());
    try {
        H5::DataSet dataset = _f.createDataSet(datasetname.c_str(), type, dataspace);
        dataset.write(dataArray.data(), type);
    }
    catch (H5::Exception& error) {
        fprintf(stderr, "%s\n", error.getCDetailMsg());
        return ErrorLibrary;
    }
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_hdf5()
{
    return new FormatImportExportHDF5();
}

}
