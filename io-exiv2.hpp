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

#ifndef TAD_IO_EXIV2_HPP
#define TAD_IO_EXIV2_HPP

#ifdef TAD_WITH_EXIV2
# include <exiv2/exiv2.hpp>
#endif

#include "io-utils.hpp"

namespace TAD {

ImageOriginLocation getImageOriginLocation(const std::string& fileName)
{
    ImageOriginLocation originLocation = OriginTopLeft;
#if TAD_WITH_EXIV2
    if (fileName.size() != 0) {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fileName.c_str());
        if (image.get()) {
            image->readMetadata();
            Exiv2::ExifData& exifData = image->exifData();
            if (!exifData.empty()) {
                Exiv2::Exifdatum& orientationTag = exifData["Exif.Image.Orientation"];
                long orientation = orientationTag.toLong();
                if (orientation >= 1 && orientation <= 8)
                    originLocation = static_cast<ImageOriginLocation>(orientation);
            }
        }
    }
#endif
    return originLocation;
}

}

#endif
