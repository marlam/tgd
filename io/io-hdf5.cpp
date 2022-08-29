/*
 * Copyright (C) 2019, 2020, 2021, 2022
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

#include <cstdio>

#include "io-hdf5.hpp"
#include "io-utils.hpp"

#include <H5Cpp.h>


namespace TGD {

FormatImportExportHDF5::FormatImportExportHDF5() : _f(nullptr), _counter(0)
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
        _f = new H5::H5File;
        try {
            _f->openFile(fileName.c_str(), H5F_ACC_RDONLY);
        }
        catch (H5::Exception& error) {
            delete _f;
            _f = nullptr;
            return ErrorInvalidData;
        }
        return ErrorNone;
    }
}

Error FormatImportExportHDF5::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (fileName == "-") {
        return ErrorInvalidData;
    } else {
        FILE* f = fopen(fileName.c_str(), append ? "rb+" : "wb");
        if (!f) {
            return ErrorSysErrno;
        }
        fclose(f);
        if (!append) {
            remove(fileName.c_str());
        }
        _f = new H5::H5File;
        try {
            *_f = H5::H5File(fileName.c_str(), append ? H5F_ACC_RDWR : H5F_ACC_TRUNC);
        }
        catch (H5::Exception& error) {
            fprintf(stderr, "%s: %s\n", fileName.c_str(), error.getCDetailMsg());
            delete _f;
            _f = nullptr;
            return ErrorLibrary;
        }
        _counter = arrayCount();
        return ErrorNone;
    }
}

void FormatImportExportHDF5::close()
{
    if (_f) {
        try {
            _f->close();
        }
        catch (H5::Exception& error) {
        }
        delete _f;
        _f = nullptr;
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
        H5Literate(_f->getId(), H5_INDEX_NAME, H5_ITER_INC, NULL, datasetVisitor, &_datasetNames);
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
        dataset = _f->openDataSet(datasetName.c_str());
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
    // read attributes
    H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
    for (int i = 0; i < dataset.getNumAttrs(); i++) {
        H5::Attribute a = dataset.openAttribute(i);
        std::string name = a.getName();
        std::string value;
        a.read(strType, value);
        bool tagConsumed = false;
        if (name.substr(0, 7) == "TGD/DIM") {
            size_t i = 0;
            size_t j = std::stoul(name.substr(7), &i);
            if (j < r.dimensionCount() && name[7 + i] == '/' && name.size() > 7 + i + 1) {
                r.dimensionTagList(j).set(name.substr(7 + i + 1), value);
                tagConsumed = true;
            }
        }
        if (!tagConsumed && name.substr(0, 8) == "TGD/COMP") {
            size_t i = 0;
            size_t j = std::stoul(name.substr(8), &i);
            if (j < r.componentCount() && name[8 + i] == '/' && name.size() > 8 + i + 1) {
                r.componentTagList(j).set(name.substr(8 + i + 1), value);
                tagConsumed = true;
            }
        }
        if (!tagConsumed) {
            r.globalTagList().set(name, value);
        }
    }
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
    case TGD::int8:
        type = H5::IntType(H5::PredType::NATIVE_INT8);
        break;
    case TGD::uint8:
        type = H5::IntType(H5::PredType::NATIVE_UINT8);
        break;
    case TGD::int16:
        type = H5::IntType(H5::PredType::NATIVE_INT16);
        break;
    case TGD::uint16:
        type = H5::IntType(H5::PredType::NATIVE_UINT16);
        break;
    case TGD::int32:
        type = H5::IntType(H5::PredType::NATIVE_INT32);
        break;
    case TGD::uint32:
        type = H5::IntType(H5::PredType::NATIVE_UINT32);
        break;
    case TGD::int64:
        type = H5::IntType(H5::PredType::NATIVE_INT64);
        break;
    case TGD::uint64:
        type = H5::IntType(H5::PredType::NATIVE_UINT64);
        break;
    case TGD::float32:
        type = H5::FloatType(H5::PredType::NATIVE_FLOAT);
        break;
    case TGD::float64:
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
        H5::DataSet dataset = _f->createDataSet(datasetname.c_str(), type, dataspace);
        dataset.write(dataArray.data(), type);
        // write attributes
        H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
        H5::DataSpace attSpace(H5S_SCALAR);
        for (auto it = array.globalTagList().cbegin(); it != array.globalTagList().cend(); it++) {
            H5::Attribute att = dataset.createAttribute(it->first.c_str(), strType, attSpace);
            att.write(strType, it->second);
        }
        for (size_t i = 0; i < array.dimensionCount(); i++) {
            for (auto it = array.dimensionTagList(i).cbegin(); it != array.dimensionTagList(i).cend(); it++) {
                std::string name = std::string("TGD/DIM") + std::to_string(i) + '/' + it->first;
                H5::Attribute att = dataset.createAttribute(name.c_str(), strType, attSpace);
                att.write(strType, it->second);
            }
        }
        for (size_t i = 0; i < array.componentCount(); i++) {
            for (auto it = array.componentTagList(i).cbegin(); it != array.componentTagList(i).cend(); it++) {
                std::string name = std::string("TGD/COMP") + std::to_string(i) + '/' + it->first;
                H5::Attribute att = dataset.createAttribute(name.c_str(), strType, attSpace);
                att.write(strType, it->second);
            }
        }
        // TODO: if this is an image, set the appropriate IMAGE attributes
        // See https://support.hdfgroup.org/HDF5/doc/ADGuide/ImageSpec.html
        // But that is an absolutely crappy specification... I currently see no point
        // in supporting that
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
