/*
 * Copyright (C) 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 * Copyright (C) 2023, 2024, 2025
 * Martin Lambers <marlam@marlam.de>
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

#include "io-dcmtk.hpp"
#include "io-utils.hpp"

#include <dcmtk/oflog/oflog.h>
#include <dcmtk/dcmimage/diregist.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcrledrg.h>
#include <dcmtk/dcmjpeg/djdecode.h>


namespace TGD {

FormatImportExportDCMTK::FormatImportExportDCMTK() :
    _dff(nullptr), _di(nullptr)
{
    OFLog::configure(OFLogger::OFF_LOG_LEVEL);
    DcmRLEDecoderRegistration::registerCodecs(OFFalse, OFFalse);
    DJDecoderRegistration::registerCodecs(EDC_photometricInterpretation, EUC_default, EPC_default, OFFalse);
    close();
}

FormatImportExportDCMTK::~FormatImportExportDCMTK()
{
    close();
    DcmRLEDecoderRegistration::cleanup();
    DJDecoderRegistration::cleanup();
}

Error FormatImportExportDCMTK::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        return ErrorInvalidData;

    _dff = new DcmFileFormat();
    OFCondition cond = _dff->loadFile(fileName.c_str(), EXS_Unknown, EGL_withoutGL, DCM_MaxReadLength, ERM_autoDetect);
    if (cond.bad()) {
        close();
        return ErrorLibrary;
    }
    E_TransferSyntax xfer = _dff->getDataset()->getOriginalXfer();
    _di = new DicomImage(_dff, xfer, CIF_MayDetachPixelData | CIF_TakeOverExternalDataset);
    if (!_di) {
        close();
        errno = ENOMEM;
        return ErrorSysErrno;
    }
    if (_di->getStatus() != EIS_Normal) {
        close();
        return ErrorLibrary;
    }
    _di->hideAllOverlays();

    Type type;
    if (_di->getDepth() <= 8) {
        type = uint8;
    } else if (_di->getDepth() <= 16) {
        type = uint16;
    } else if (_di->getDepth() <= 32) {
        type = uint32;
    } else if (_di->getDepth() <= 64) {
        type = uint64;
    } else {
        close();
        return ErrorFeaturesUnsupported;
    }

    size_t compCount = _di->isMonochrome() ? 1 : 3;
    _desc = ArrayDescription({ _di->getWidth(), _di->getHeight() }, compCount, type);
    _desc.globalTagList().set("DICOM/TRANSFER_SYNTAX", DcmXfer(xfer).getXferName());
    if (_di->getPhotometricInterpretation() == EPI_RGB) {
        _desc.componentTagList(0).set("INTERPRETATION", "SRGB/R");
        _desc.componentTagList(1).set("INTERPRETATION", "SRGB/G");
        _desc.componentTagList(2).set("INTERPRETATION", "SRGB/B");
    }
    _desc.globalTagList().set("DICOM/PIXEL_ASPECT_RATIO", std::to_string(_di->getHeightWidthRatio()).c_str());
    _desc.globalTagList().set("DICOM/BITS_PER_SAMPLE", std::to_string(_di->getDepth()).c_str());
    return ErrorNone;
}

Error FormatImportExportDCMTK::openForWriting(const std::string&, bool, const TagList&)
{
    return ErrorFeaturesUnsupported;
}

void FormatImportExportDCMTK::close()
{
    if (_dff) {
        delete _dff;
        _dff = nullptr;
    }
    if (_di) {
        // delete _di; // Automatically deleted!?
        _di = nullptr;
    }
    _desc = ArrayDescription();
    _indexOfLastReadFrame = -1;
}

int FormatImportExportDCMTK::arrayCount()
{
    return _di->getFrameCount();
}

ArrayContainer FormatImportExportDCMTK::readArray(Error* error, int arrayIndex, const Allocator& alloc)
{
    if (arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    int index = _indexOfLastReadFrame + 1;
    if (arrayIndex >= 0)
        index = arrayIndex;
    ArrayContainer r(_desc, alloc);
    if (!_di->getOutputData(r.data(), r.dataSize(), r.componentSize() * 8, index, 0)) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    _indexOfLastReadFrame = index;
    reverseY(r);
    return r;
}

bool FormatImportExportDCMTK::hasMore()
{
    return (_indexOfLastReadFrame < arrayCount() - 1);
}

Error FormatImportExportDCMTK::writeArray(const ArrayContainer&)
{
    return ErrorFeaturesUnsupported;
}

extern "C" FormatImportExport* FormatImportExportFactory_dcmtk()
{
    return new FormatImportExportDCMTK();
}

}
