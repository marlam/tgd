/*
 * Copyright (C) 2022
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
#include <cstring>

#include "io-tinyexr.hpp"
#include "io-utils.hpp"

#define TINYEXR_USE_MINIZ (0)
#define TINYEXR_USE_STB_ZLIB (1)
#define TINYEXR_IMPLEMENTATION
#include "../ext/tinyexr.h"

namespace TGD {

FormatImportExportTinyEXR::FormatImportExportTinyEXR() : _arrayWasReadOrWritten(false)
{
}

FormatImportExportTinyEXR::~FormatImportExportTinyEXR()
{
}

Error FormatImportExportTinyEXR::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-") {
        return ErrorInvalidData;
    } else {
        FILE* f = fopen(fileName.c_str(), "rb");
        if (f) {
            fclose(f);
            _fileName = fileName;
            return ErrorNone;
        } else {
            return ErrorSysErrno;
        }
    }
}

Error FormatImportExportTinyEXR::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (append)
        return ErrorFeaturesUnsupported;
    if (fileName == "-") {
        return ErrorInvalidData;
    } else {
        FILE* f = fopen(fileName.c_str(), "wb");
        if (f) {
            fclose(f);
            _fileName = fileName;
            return ErrorNone;
        } else {
            return ErrorSysErrno;
        }
    }
}

void FormatImportExportTinyEXR::close()
{
    _arrayWasReadOrWritten = false;
}

int FormatImportExportTinyEXR::arrayCount()
{
    return (_fileName.length() == 0 ? -1 : 1);
}

ArrayContainer FormatImportExportTinyEXR::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex > 0) {
        *error = ErrorSeekingNotSupported;
        return ArrayContainer();
    }
    if (_arrayWasReadOrWritten) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    _arrayWasReadOrWritten = true;

    EXRVersion exr_version;
    int ret = ParseEXRVersionFromFile(&exr_version, _fileName.c_str());
    if (ret != 0) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    if (exr_version.multipart) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }

    EXRHeader exr_header;
    InitEXRHeader(&exr_header);
    const char* err = nullptr;
    ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, _fileName.c_str(), &err);
    if (ret != 0) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    for (int i = 0; i < exr_header.num_channels; i++) {
        if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_UINT) {
            *error = ErrorFeaturesUnsupported;
            FreeEXRErrorMessage(err);
            FreeEXRHeader(&exr_header);
            return ArrayContainer();
        }
        if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
            exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        }
    }

    EXRImage exr_image;
    InitEXRImage(&exr_image);
    ret = LoadEXRImageFromFile(&exr_image, &exr_header, _fileName.c_str(), &err);
    if (ret != 0) {
        *error = ErrorInvalidData;
        FreeEXRErrorMessage(err);
        FreeEXRHeader(&exr_header);
        return ArrayContainer();
    }

    if (!exr_image.images) {
        // tiled format; TODO: implement reconstruction from tiles
        *error = ErrorFeaturesUnsupported;
        FreeEXRImage(&exr_image);
        FreeEXRHeader(&exr_header);
        return ArrayContainer();
    }
    if (exr_image.width < 0 || exr_image.height < 0 || exr_image.num_channels < 0
            || exr_image.num_channels >= std::numeric_limits<int>::max()) {
        *error = ErrorInvalidData;
        FreeEXRImage(&exr_image);
        FreeEXRHeader(&exr_header);
        return ArrayContainer();
    }

    size_t w = exr_image.width;
    size_t h = exr_image.height;
    size_t nc = exr_image.num_channels;

    /* Try to find the typical color channels and put them into the right position */
    int indexR = -1;
    int indexG = -1;
    int indexB = -1;
    int indexA = -1;
    for (size_t c = 0; c < nc; c++) {
        if (std::string(exr_header.channels[c].name) == "R")
            indexR = c;
        if (std::string(exr_header.channels[c].name) == "G")
            indexG = c;
        if (std::string(exr_header.channels[c].name) == "B")
            indexB = c;
        if (std::string(exr_header.channels[c].name) == "A")
            indexA = c;
    }
    std::vector<size_t> channelPermutation;
    channelPermutation.reserve(nc);
    if (indexR >= 0)
        channelPermutation.push_back(indexR);
    if (indexG >= 0)
        channelPermutation.push_back(indexG);
    if (indexB >= 0)
        channelPermutation.push_back(indexB);
    if (indexA >= 0)
        channelPermutation.push_back(indexA);
    for (int c = 0; c < int(nc); c++) {
        if (c != indexR && c != indexG && c != indexB && c != indexA)
            channelPermutation.push_back(c);
    }

    TGD::Array<float> r({ w, h }, nc);
    for (size_t c = 0; c < nc; c++) {
        std::string interpretation = exr_header.channels[channelPermutation[c]].name;
        if (interpretation == "R")
            interpretation = "RED";
        else if (interpretation == "G")
            interpretation = "GREEN";
        else if (interpretation == "B")
            interpretation = "BLUE";
        else if (interpretation == "A")
            interpretation = "ALPHA";
        else if (interpretation == "Y")
            interpretation = "XYZ/Y";
        else if (interpretation == "Z")
            interpretation = "DEPTH";
        r.componentTagList(c).set("INTERPRETATION", interpretation);
    }
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            for (size_t c = 0; c < nc; c++) {
                size_t realY = (exr_header.line_order == 0 ? h - 1 - y : y);
                float v = reinterpret_cast<float*>(exr_image.images[channelPermutation[c]])[realY * w + x];
                r[{x, y}][c] = v;
            }
        }
    }

    FreeEXRImage(&exr_image);
    FreeEXRHeader(&exr_header);

    return r;
}

bool FormatImportExportTinyEXR::hasMore()
{
    return !_arrayWasReadOrWritten;
}

Error FormatImportExportTinyEXR::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) < 1 || array.dimension(1) < 1
            || array.dimension(0) > 65535 || array.dimension(1) > 65535
            || array.componentCount() < 1 || array.componentCount() > 65535
            || array.componentType() != float32
            || _arrayWasReadOrWritten) {
        return ErrorFeaturesUnsupported;
    }

    EXRHeader header;
    InitEXRHeader(&header);
    header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;

    EXRImage image;
    InitEXRImage(&image);

    image.width = array.dimension(0);
    image.height = array.dimension(1);
    image.num_channels = array.componentCount();
    header.num_channels = array.componentCount();
    std::vector<std::vector<float>> images(array.componentCount());
    for (size_t c = 0; c < array.componentCount(); c++)
        images[c].resize(array.elementCount());
    for (size_t y = 0; y < array.dimension(1); y++) {
        for (size_t x = 0; x < array.dimension(0); x++) {
            size_t srcE = (array.dimension(1) - 1 - y) * array.dimension(0) + x;
            size_t dstE = y * array.dimension(0) + x;
            for (size_t c = 0; c < array.componentCount(); c++) {
                images[c][dstE] = array.get<float>(srcE)[c];
            }
        }
    }
    // find RGBA channels and put them in order ABGR
    int indexR = -1, indexG = -1, indexB = -1, indexA = -1;
    for (size_t c = 0; c < array.componentCount(); c++) {
        std::string s = array.componentTagList(c).value("INTERPRETATION");
        if (s == "RED")
            indexR = c;
        else if (s == "GREEN")
            indexG = c;
        else if (s == "BLUE")
            indexB = c;
        else if (s == "ALPHA")
            indexA = c;
    }
    std::vector<float*> imagePtr;
    imagePtr.reserve(array.componentCount());
    std::vector<EXRChannelInfo> channelInfos;
    channelInfos.reserve(array.componentCount());
    EXRChannelInfo chi;
    if (indexA >= 0) {
        imagePtr.push_back(images[indexA].data());
        chi.name[0] = 'A'; chi.name[1] = '\0';
        channelInfos.push_back(chi);
    }
    if (indexB >= 0) {
        imagePtr.push_back(images[indexB].data());
        chi.name[0] = 'B'; chi.name[1] = '\0';
        channelInfos.push_back(chi);
    }
    if (indexG >= 0) {
        imagePtr.push_back(images[indexG].data());
        chi.name[0] = 'G'; chi.name[1] = '\0';
        channelInfos.push_back(chi);
    }
    if (indexR >= 0) {
        imagePtr.push_back(images[indexR].data());
        chi.name[0] = 'R'; chi.name[1] = '\0';
        channelInfos.push_back(chi);
    }
    for (size_t c = 0; c < array.componentCount(); c++) {
        if (int(c) != indexR && int(c) != indexG && int(c) != indexB && int(c) != indexA) {
            imagePtr.push_back(images[c].data());
            std::string s = array.componentTagList(c).value("INTERPRETATION", std::string("VALUE") + std::to_string(c));
            if (s == "XYZ/Y")
                s = "Y";
            else if (s == "DEPTH")
                s = "Z";
            strncpy(chi.name, s.c_str(), 255);
            chi.name[255] = '\0';
            channelInfos.push_back(chi);
        }
    }
    image.images = reinterpret_cast<unsigned char**>(imagePtr.data());
    header.channels = channelInfos.data();
    std::vector<int> pixelTypes(array.componentCount(), TINYEXR_PIXELTYPE_FLOAT);
    header.pixel_types = pixelTypes.data();
    header.requested_pixel_types = pixelTypes.data();

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, _fileName.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        FreeEXRErrorMessage(err);
        return ErrorLibrary;
    }

    _arrayWasReadOrWritten = true;
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_tinyexr()
{
    return new FormatImportExportTinyEXR();
}

}
