/*
 * Copyright (C) 2022 Computer Graphics Group, University of Siegen
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

#include "io-magick.hpp"
#include "io-utils.hpp"

#include <vector>
#include <limits>

#include <Magick++.h>


namespace TAD {

class TADMagick
{
public:
    std::vector<Magick::Image> imgs;

    TADMagick()
    {
    }
};

FormatImportExportMagick::FormatImportExportMagick() :
    _fileName(), _triedReading(false), _magick(new TADMagick), _lastArrayIndex(-1)
{
    Magick::InitializeMagick(0);
}

FormatImportExportMagick::~FormatImportExportMagick()
{
    delete _magick;
}

Error FormatImportExportMagick::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        return ErrorInvalidData;
    FILE* f = fopen(fileName.c_str(), "rb");
    if (!f)
        return ErrorSysErrno;
    fclose(f);
    _fileName = fileName;
    return ErrorNone;
}

Error FormatImportExportMagick::openForWriting(const std::string&, bool, const TagList&)
{
    return ErrorFeaturesUnsupported;
}

void FormatImportExportMagick::close()
{
    _triedReading = false;
    _lastArrayIndex = -1;
}

static bool readImagesOnce(const std::string& fileName, bool& triedReading, std::vector<Magick::Image>& imgs)
{
    if (!triedReading) {
        try {
            Magick::readImages(&imgs, fileName);
        }
        catch (...) {
            imgs.clear();
        }
        if (imgs.size() > std::numeric_limits<unsigned short>::max())
            imgs.clear();
        triedReading = true;
    }
    return (imgs.size() > 0);
}

int FormatImportExportMagick::arrayCount()
{
    readImagesOnce(_fileName, _triedReading, _magick->imgs);
    return _magick->imgs.size();
}

ArrayContainer FormatImportExportMagick::readArray(Error* error, int arrayIndex)
{
    readImagesOnce(_fileName, _triedReading, _magick->imgs);
    if (arrayIndex < 0)
        arrayIndex = _lastArrayIndex + 1;
    if (arrayCount() <= 0 || arrayIndex >= arrayCount()) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    TAD::ArrayContainer array;
    ImageOriginLocation originLocation = OriginTopLeft;
    try {
        Magick::Image& img = _magick->imgs[arrayIndex];
        size_t width = img.columns();
        size_t height = img.rows();
        Type type = (img.depth() <= 8 ? uint8
                : img.depth() <= 16 ? uint16
                : float32);
        bool hasAlpha = (img.matte());
        bool isGray = (img.colorspaceType() == Magick::GRAYColorspace);
        unsigned int channels = (isGray ? 1 : 3) + (hasAlpha ? 1 : 0);

        array = TAD::ArrayContainer({ width, height }, channels, type);
        if (isGray) {
            array.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
        } else {
            array.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            array.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            array.componentTagList(2).set("INTERPRETATION", "SRGB/B");
        }
        if (hasAlpha) {
            array.componentTagList(isGray ? 1 : 3).set("INTERPRETATION", "ALPHA");
        }
        switch (img.orientation()) {
        case Magick::UndefinedOrientation:
            break;
        case Magick::TopLeftOrientation:
            originLocation = OriginTopLeft;
            break;
        case Magick::TopRightOrientation:
            originLocation = OriginTopRight;
            break;
        case Magick::BottomRightOrientation:
            originLocation = OriginBottomRight;
            break;
        case Magick::BottomLeftOrientation:
            originLocation = OriginBottomLeft;
            break;
        case Magick::LeftTopOrientation:
            originLocation = OriginLeftTop;
            break;
        case Magick::RightTopOrientation:
            originLocation = OriginRightTop;
            break;
        case Magick::RightBottomOrientation:
            originLocation = OriginRightBottom;
            break;
        case Magick::LeftBottomOrientation:
            originLocation = OriginLeftBottom;
            break;
        }

        Magick::StorageType storageType = (type == uint8 ? Magick::CharPixel
                : type == uint16 ? Magick::ShortPixel : Magick::FloatPixel);
        std::string map = (isGray ? "I" : "RGB");
        if (hasAlpha)
            map += "A";
        img.write(0, 0, width, height, map.c_str(), storageType, array.data());
    }
    catch (...) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    fixImageOrientation(array, originLocation);
    _lastArrayIndex = arrayIndex;
    return array;
}

bool FormatImportExportMagick::hasMore()
{
    return _lastArrayIndex + 1 < arrayCount();
}

Error FormatImportExportMagick::writeArray(const ArrayContainer&)
{
    return ErrorFeaturesUnsupported;
}

extern "C" FormatImportExport* FormatImportExportFactory_magick()
{
    return new FormatImportExportMagick();
}

}
