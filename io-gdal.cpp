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

#include "io-gdal.hpp"
#include "io-utils.hpp"

#include <cpl_conv.h>


namespace TAD {

FormatImportExportGDAL::FormatImportExportGDAL() : _dataset(nullptr)
{
    GDALAllRegister();
    close();
}

FormatImportExportGDAL::~FormatImportExportGDAL()
{
    close();
}

Error FormatImportExportGDAL::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        return ErrorInvalidData;

    if (!(_dataset = GDALOpen(fileName.c_str(), GA_ReadOnly))) {
        return ErrorInvalidData;
    }
    if (GDALGetRasterXSize(_dataset) < 1
            || GDALGetRasterYSize(_dataset) < 1
            || GDALGetRasterCount(_dataset) < 1) {
        close();
        return ErrorFeaturesUnsupported;
    }

    size_t width = GDALGetRasterXSize(_dataset);
    size_t height = GDALGetRasterYSize(_dataset);
    size_t compCount = GDALGetRasterCount(_dataset);
    Type type = uint8;
    for (size_t i = 0; i < compCount; i++) {
        GDALRasterBandH band = GDALGetRasterBand(_dataset, i + 1);
        Type bandType;
        GDALDataType gdalType = GDALGetRasterDataType(band);
        switch (gdalType) {
        case GDT_Byte:
            bandType = uint8;
            break;
        case GDT_UInt16:
            bandType = uint16;
            break;
        case GDT_Int16:
            bandType = int16;
            break;
        case GDT_UInt32:
            bandType = uint32;
            break;
        case GDT_Int32:
            bandType = int32;
            break;
        case GDT_Float32:
            bandType = float32;
            break;
        case GDT_Float64:
            bandType = float64;
            break;
        case GDT_CInt16:
        case GDT_CInt32:
        case GDT_CFloat32:
        case GDT_CFloat64:
        case GDT_Unknown:
        default:
            close();
            return ErrorFeaturesUnsupported;
            break;
        }
        if (i == 0) {
            type = bandType;
            _gdalType = gdalType;
        } else if (type != bandType) {
            close();
            return ErrorFeaturesUnsupported;
        }
    }
    _desc = ArrayDescription({ width, height }, compCount, type);

    std::string description = GDALGetDescription(_dataset);
    if (description.length() > 0 && description != fileName)
        _desc.globalTagList().set("DESCRIPTION", description);
    if (GDALGetProjectionRef(_dataset) && GDALGetProjectionRef(_dataset)[0])
        _desc.globalTagList().set("GDAL/PROJECTION", GDALGetProjectionRef(_dataset));
    double geoTransform[6];
    if (GDALGetGeoTransform(_dataset, geoTransform) == CE_None) {
        _desc.globalTagList().set("GDAL/GEO_TRANSFORM",
                std::to_string(geoTransform[0]) + " "
                + std::to_string(geoTransform[1]) + " "
                + std::to_string(geoTransform[2]) + " "
                + std::to_string(geoTransform[3]) + " "
                + std::to_string(geoTransform[4]) + " "
                + std::to_string(geoTransform[5]));
    }
    for (size_t i = 0; i < _desc.componentCount(); i++) {
        GDALRasterBandH band = GDALGetRasterBand(_dataset, i + 1);
        description = GDALGetDescription(band);
        if (description.length() > 0)
            _desc.componentTagList(i).set("DESCRIPTION", description);
        int success;
        double value;
        value = GDALGetRasterMinimum(band, &success);
        if (success)
            _desc.componentTagList(i).set("MINVAL", std::to_string(value));
        value = GDALGetRasterMaximum(band, &success);
        if (success)
            _desc.componentTagList(i).set("MAXVAL", std::to_string(value));
        value = GDALGetRasterOffset(band, &success);
        if (success && (value < 0.0 || value > 0.0))
            _desc.componentTagList(i).set("GDAL/OFFSET", std::to_string(value));
        value = GDALGetRasterScale(band, &success);
        if (success && (value < 1.0 || value > 1.0))
            _desc.componentTagList(i).set("GDAL/SCALE", std::to_string(value));
        value = GDALGetRasterNoDataValue(band, &success);
        if (success)
            _desc.componentTagList(i).set("NO_DATA_VALUE", std::to_string(value));
        const char *unit = GDALGetRasterUnitType(band);
        if (unit && unit[0])
            _desc.componentTagList(i).set("UNIT", unit);
        switch (GDALGetRasterColorInterpretation(band)) {
        case GCI_GrayIndex:
            _desc.componentTagList(i).set("INTERPRETATION", "SRGB/GRAY");
            break;
        case GCI_RedBand:
            _desc.componentTagList(i).set("INTERPRETATION", "SRGB/R");
            break;
        case GCI_GreenBand:
            _desc.componentTagList(i).set("INTERPRETATION", "SRGB/G");
            break;
        case GCI_BlueBand:
            _desc.componentTagList(i).set("INTERPRETATION", "SRGB/B");
            break;
        case GCI_AlphaBand:
            _desc.componentTagList(i).set("INTERPRETATION", "ALPHA");
            break;
        case GCI_HueBand:
            _desc.componentTagList(i).set("INTERPRETATION", "HSL/H");
            break;
        case GCI_SaturationBand:
            _desc.componentTagList(i).set("INTERPRETATION", "HSL/S");
            break;
        case GCI_LightnessBand:
            _desc.componentTagList(i).set("INTERPRETATION", "HSL/L");
            break;
        case GCI_CyanBand:
            _desc.componentTagList(i).set("INTERPRETATION", "CMYK/C");
            break;
        case GCI_MagentaBand:
            _desc.componentTagList(i).set("INTERPRETATION", "CMYK/M");
            break;
        case GCI_YellowBand:
            _desc.componentTagList(i).set("INTERPRETATION", "CMYK/Y");
            break;
        case GCI_BlackBand:
            _desc.componentTagList(i).set("INTERPRETATION", "CMYK/K");
            break;
        case GCI_YCbCr_YBand:
            _desc.componentTagList(i).set("INTERPRETATION", "YCBCR/Y");
            break;
        case GCI_YCbCr_CbBand:
            _desc.componentTagList(i).set("INTERPRETATION", "YCBCR/CB");
            break;
        case GCI_YCbCr_CrBand:
            _desc.componentTagList(i).set("INTERPRETATION", "YCBCR/CR");
            break;
        case GCI_PaletteIndex:
        default:
            // no tag fits
            break;
        }
    }
    return ErrorNone;
}

Error FormatImportExportGDAL::openForWriting(const std::string&, bool, const TagList&)
{
    return ErrorFeaturesUnsupported;
}

void FormatImportExportGDAL::close()
{
    if (_dataset) {
        GDALClose(_dataset);
        _dataset = nullptr;
    }
    _desc = ArrayDescription();
    _arrayWasRead = false;
}

int FormatImportExportGDAL::arrayCount()
{
    return 1;
}

ArrayContainer FormatImportExportGDAL::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    ArrayContainer r(_desc);
    CPLErr err = GDALDatasetRasterIO(_dataset, GF_Read,
            0, 0, r.dimension(0), r.dimension(1),
            r.data(), r.dimension(0), r.dimension(1),
            _gdalType, r.componentCount(), nullptr,
            r.elementSize(), r.elementSize() * r.dimension(0), r.componentSize());
    if (err != CE_None) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    reverseY(r);
    _arrayWasRead = true;
    return r;
}

bool FormatImportExportGDAL::hasMore()
{
    return !_arrayWasRead;
}

Error FormatImportExportGDAL::writeArray(const ArrayContainer&)
{
    return ErrorFeaturesUnsupported;
}

extern "C" FormatImportExport* FormatImportExportFactory_gdal()
{
    return new FormatImportExportGDAL();
}

}
