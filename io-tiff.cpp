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
#include <cstring>

#include <tiffio.h>

#include "io-tiff.hpp"
#include "io-utils.hpp"


namespace TAD {

FormatImportExportTIFF::FormatImportExportTIFF() :
    _tiff(nullptr), _dirCount(-1), _readCount(0)
{
    TIFFSetErrorHandler(0);
    TIFFSetWarningHandler(0);
}

FormatImportExportTIFF::~FormatImportExportTIFF()
{
    close();
}

Error FormatImportExportTIFF::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        return ErrorInvalidData;
    FILE* f = fopen(fileName.c_str(), "rb");
    if (!f)
        return ErrorSysErrno;
    fclose(f);
    _tiff = TIFFOpen(fileName.c_str(), "r");
    if (!_tiff)
        return ErrorInvalidData;
    return ErrorNone;
}

Error FormatImportExportTIFF::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (append)
        return ErrorFeaturesUnsupported;
    if (fileName == "-")
        return ErrorInvalidData;
    FILE* f = fopen(fileName.c_str(), "wb");
    if (!f)
        return ErrorSysErrno;
    fclose(f);
    _tiff = TIFFOpen(fileName.c_str(), "w");
    if (!_tiff)
        return ErrorLibrary;
    return ErrorNone;
}

void FormatImportExportTIFF::close()
{
    if (_tiff) {
        TIFFClose(_tiff);
        _tiff = nullptr;
        _dirCount = -1;
        _readCount = 0;
    }
}

int FormatImportExportTIFF::arrayCount()
{
    if (_dirCount < 0) {
        _dirCount = 0;
        do {
            _dirCount++;
        } while (TIFFReadDirectory(_tiff));
        TIFFSetDirectory(_tiff, 0);
    }
    return _dirCount;
}

ArrayContainer FormatImportExportTIFF::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    } else if (arrayIndex < 0) {
        if (_readCount > 0) {
            if (!TIFFSetDirectory(_tiff, _readCount)) {
                *error = ErrorLibrary;
                return ArrayContainer();
            }
        }
    } else {
        if (!TIFFSetDirectory(_tiff, arrayIndex)) {
            *error = ErrorLibrary;
            return ArrayContainer();
        }        
    }

    uint32_t width = 0, height = 0;
    TIFFGetField(_tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(_tiff, TIFFTAG_IMAGELENGTH, &height);
    if (width == 0 || height == 0) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    uint32_t tileWidth = 0, tileHeight = 0;
    TIFFGetField(_tiff, TIFFTAG_TILEWIDTH, &tileWidth);
    TIFFGetField(_tiff, TIFFTAG_TILELENGTH, &tileHeight);

    uint16_t config;
    if (!TIFFGetField(_tiff, TIFFTAG_PLANARCONFIG, &config)) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    if (config != PLANARCONFIG_CONTIG && config != PLANARCONFIG_SEPARATE) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }

    uint16_t sampleFormat;
    if (!TIFFGetFieldDefaulted(_tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat)) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    if (sampleFormat != SAMPLEFORMAT_UINT && sampleFormat != SAMPLEFORMAT_INT && sampleFormat != SAMPLEFORMAT_IEEEFP) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }

    uint16_t bps;
    if (!TIFFGetField(_tiff, TIFFTAG_BITSPERSAMPLE, &bps)) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    if (bps != 8 && bps != 16 && bps != 32 && bps != 64) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }

    uint16_t nSamples;
    if (!TIFFGetField(_tiff, TIFFTAG_SAMPLESPERPIXEL, &nSamples)) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }
    if (nSamples < 1) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }

    uint16_t orientation;
    if (!TIFFGetFieldDefaulted(_tiff, TIFFTAG_ORIENTATION, &orientation)) {
        orientation = ORIENTATION_TOPLEFT;
    }
    if (orientation != ORIENTATION_TOPLEFT && orientation != ORIENTATION_BOTLEFT) {
        *error = ErrorFeaturesUnsupported;
        return ArrayContainer();
    }

    uint16_t comp;
    if (!TIFFGetField(_tiff, TIFFTAG_COMPRESSION, &comp))
        comp = COMPRESSION_NONE;

    uint16_t phot;
    bool havePhot = TIFFGetFieldDefaulted(_tiff, TIFFTAG_PHOTOMETRIC, &phot);

    Type type = float32;
    if (havePhot && phot == PHOTOMETRIC_LOGLUV && (comp == COMPRESSION_SGILOG || comp == COMPRESSION_SGILOG24)) {
        TIFFSetField(_tiff, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
        type = float32;
    } else if (bps == 8) {
        if (sampleFormat == SAMPLEFORMAT_UINT) {
            type = uint8;
        } else if (sampleFormat == SAMPLEFORMAT_INT) {
            type = int8;
        } else {
            *error = ErrorFeaturesUnsupported;
            return ArrayContainer();
        }
    } else if (bps == 16) {
        if (sampleFormat == SAMPLEFORMAT_UINT) {
            type = uint16;
        } else if (sampleFormat == SAMPLEFORMAT_INT) {
            type = int16;
        } else {
            *error = ErrorFeaturesUnsupported;
            return ArrayContainer();
        }
    } else if (bps == 32) {
        if (sampleFormat == SAMPLEFORMAT_UINT) {
            type = uint32;
        } else if (sampleFormat == SAMPLEFORMAT_INT) {
            type = int32;
        } else {
            type = float32;
        }
    } else /* bps == 64 */ {
        if (sampleFormat == SAMPLEFORMAT_UINT) {
            type = uint64;
        } else if (sampleFormat == SAMPLEFORMAT_INT) {
            type = int64;
        } else {
            type = float64;
        }
    }

    ArrayContainer r({ width, height }, nSamples, type);
    if (r.dimension(0) * r.elementSize() != size_t(TIFFScanlineSize(_tiff))) {
        *error = ErrorLibrary;
        return ArrayContainer();
    }

    if (havePhot && phot == PHOTOMETRIC_LOGLUV && (comp == COMPRESSION_SGILOG || comp == COMPRESSION_SGILOG24)) {
        if (r.componentCount() == 3 || r.componentCount() == 4) {
            r.componentTagList(0).set("INTERPRETATION", "XYZ/X");
            r.componentTagList(1).set("INTERPRETATION", "XYZ/Y");
            r.componentTagList(2).set("INTERPRETATION", "XYZ/Z");
            if (r.componentCount() == 4)
                r.componentTagList(3).set("INTERPRETATION", "ALPHA");
        }
    } else if (havePhot && phot == PHOTOMETRIC_RGB) {
        if (r.componentCount() == 3 || r.componentCount() == 4) {
            if (type == uint8) {
                r.componentTagList(0).set("INTERPRETATION", "SRGB/R");
                r.componentTagList(1).set("INTERPRETATION", "SRGB/G");
                r.componentTagList(2).set("INTERPRETATION", "SRGB/B");
            } else {
                r.componentTagList(0).set("INTERPRETATION", "RED");
                r.componentTagList(1).set("INTERPRETATION", "GREEN");
                r.componentTagList(2).set("INTERPRETATION", "BLUE");
            }
            if (r.componentCount() == 4)
                r.componentTagList(3).set("INTERPRETATION", "ALPHA");
        }
    } else if (havePhot && phot == PHOTOMETRIC_MINISBLACK) {
        if (r.componentCount() == 1 || r.componentCount() == 2) {
            if (type == uint8)
                r.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
            else
                r.componentTagList(0).set("INTERPRETATION", "GRAY");
            if (r.componentCount() == 2)
                r.componentTagList(1).set("INTERPRETATION", "ALPHA");
        }
    }

    if (tileWidth == 0 && tileHeight == 0) {
        if (config == PLANARCONFIG_CONTIG) {
            for (uint32_t row = 0; row < height; row++) {
                if (!TIFFReadScanline(_tiff, r.get({ 0, row }), row)) {
                    *error = ErrorLibrary;
                    return ArrayContainer();
                }
            }
        } else {
            for (uint16_t s = 0; s < nSamples; s++) {
                for (uint32_t row = 0; row < height; row++) {
                    if (!TIFFReadScanline(_tiff, r.get({ 0, row }), row, s)) {
                        *error = ErrorLibrary;
                        return ArrayContainer();
                    }
                }
            }
        }
    } else {
        std::vector<unsigned char> tileData(TIFFTileSize(_tiff));
        size_t tileLineSize = tileWidth * r.elementSize();
        for (uint32_t y = 0; y < height; y += tileHeight) {
            for (uint32_t x = 0; x < width; x+= tileWidth) {
                for (uint16_t s = 0; s < nSamples; s++) {
                    TIFFReadTile(_tiff, tileData.data(), x, y, 0, s);
                    uint32_t remainingTileHeight = tileHeight;
                    if (y + remainingTileHeight > r.dimension(1))
                        remainingTileHeight = r.dimension(1) - y;
                    uint32_t remainingTileWidth = tileWidth;
                    if (x + remainingTileWidth > r.dimension(0))
                        remainingTileWidth = r.dimension(0) - x;
                    for (uint32_t ty = 0; ty < remainingTileHeight; ty++) {
                        uint32_t ry = y + ty;
                        unsigned char* rdata = static_cast<unsigned char*>(r.get({ x, ry }));
                        const unsigned char* tdata = tileData.data() + ty * tileLineSize;
                        for (uint32_t tx = 0; tx < remainingTileWidth; tx++) {
                            std::memcpy(
                                    rdata + tx * r.elementSize() + s * r.componentSize(),
                                    tdata + tx * r.componentSize(),
                                    r.componentSize());
                        }
                    }
                }
            }
        }
    }

    if (orientation >= 1 && orientation <= 8) {
        fixImageOrientation(r, static_cast<ImageOriginLocation>(orientation));
    }

    _readCount++;
    return r;
}

bool FormatImportExportTIFF::hasMore()
{
    return _readCount < arrayCount();
}

Error FormatImportExportTIFF::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > std::numeric_limits<uint32_t>::max()
            || array.dimension(1) > std::numeric_limits<uint32_t>::max()
            || array.componentCount() <= 0
            || array.componentCount() > std::numeric_limits<uint16_t>::max()) {
        return ErrorFeaturesUnsupported;
    }

    TIFFSetField(_tiff, TIFFTAG_IMAGEWIDTH, uint32_t(array.dimension(0)));
    TIFFSetField(_tiff, TIFFTAG_IMAGELENGTH, uint32_t(array.dimension(1)));
    TIFFSetField(_tiff, TIFFTAG_SAMPLESPERPIXEL, uint16_t(array.componentCount()));
    uint16_t sampleFormat = SAMPLEFORMAT_UINT;
    uint16_t bps = 8;
    switch (array.componentType()) {
    case int8:
        sampleFormat = SAMPLEFORMAT_INT;
        bps = 8;
        break;
    case uint8:
        sampleFormat = SAMPLEFORMAT_UINT;
        bps = 8;
        break;
    case int16:
        sampleFormat = SAMPLEFORMAT_INT;
        bps = 16;
        break;
    case uint16:
        sampleFormat = SAMPLEFORMAT_UINT;
        bps = 16;
        break;
    case int32:
        sampleFormat = SAMPLEFORMAT_INT;
        bps = 32;
        break;
    case uint32:
        sampleFormat = SAMPLEFORMAT_UINT;
        bps = 32;
        break;
    case int64:
        sampleFormat = SAMPLEFORMAT_INT;
        bps = 64;
        break;
    case uint64:
        sampleFormat = SAMPLEFORMAT_UINT;
        bps = 64;
        break;
    case float32:
        sampleFormat = SAMPLEFORMAT_IEEEFP;
        bps = 32;
        break;
    case float64:
        sampleFormat = SAMPLEFORMAT_IEEEFP;
        bps = 64;
        break;
    }
    TIFFSetField(_tiff, TIFFTAG_SAMPLEFORMAT, sampleFormat);
    TIFFSetField(_tiff, TIFFTAG_BITSPERSAMPLE, bps);

    TIFFSetField(_tiff, TIFFTAG_COMPRESSION, uint16_t(COMPRESSION_NONE));
    TIFFSetField(_tiff, TIFFTAG_PLANARCONFIG, uint16_t(PLANARCONFIG_CONTIG));
    TIFFSetField(_tiff, TIFFTAG_ORIENTATION, uint16_t(ORIENTATION_TOPLEFT));

    if (array.componentCount() >= 3
            && array.componentTagList(0).value("INTERPRETATION") == "RED"
            && array.componentTagList(1).value("INTERPRETATION") == "GREEN"
            && array.componentTagList(2).value("INTERPRETATION") == "BLUE") {
        TIFFSetField(_tiff, TIFFTAG_PHOTOMETRIC, uint16_t(PHOTOMETRIC_RGB));
    } else if (array.componentCount() >= 3
            && array.componentTagList(0).value("INTERPRETATION") == "SRGB/R"
            && array.componentTagList(1).value("INTERPRETATION") == "SRGB/G"
            && array.componentTagList(2).value("INTERPRETATION") == "SRGB/B") {
        TIFFSetField(_tiff, TIFFTAG_PHOTOMETRIC, uint16_t(PHOTOMETRIC_RGB));
    } else if (array.componentCount() >= 1
            && array.componentTagList(0).value("INTERPRETATION") == "GRAY") {
        TIFFSetField(_tiff, TIFFTAG_PHOTOMETRIC, uint16_t(PHOTOMETRIC_MINISBLACK));
    } else if (array.componentCount() >= 1
            && array.componentTagList(0).value("INTERPRETATION") == "SRGB/GRAY") {
        TIFFSetField(_tiff, TIFFTAG_PHOTOMETRIC, uint16_t(PHOTOMETRIC_MINISBLACK));
    }

    for (size_t y = 0; y < array.dimension(1); y++) {
        TIFFWriteScanline(_tiff, const_cast<void*>(array.get({ 0, array.dimension(1) - 1 - y })), y);
    }
    return (TIFFFlush(_tiff) ? ErrorNone : ErrorLibrary);
}

extern "C" FormatImportExport* FormatImportExportFactory_tiff()
{
    return new FormatImportExportTIFF();
}

}
