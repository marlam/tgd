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

#ifndef TGD_IO_EXIV2_HPP
#define TGD_IO_EXIV2_HPP

#ifdef TGD_WITH_EXIV2
# include <exiv2/exiv2.hpp>
#endif

#include "io-utils.hpp"

namespace TGD {

inline ImageOriginLocation getImageOriginLocation(const std::string& fileName)
{
    ImageOriginLocation originLocation = OriginTopLeft;
#if TGD_WITH_EXIV2
    if (fileName.size() != 0) {
        auto image = Exiv2::ImageFactory::open(fileName.c_str());
        if (image.get()) {
            image->readMetadata();
            Exiv2::ExifData& exifData = image->exifData();
            if (!exifData.empty()) {
                Exiv2::Exifdatum& orientationTag = exifData["Exif.Image.Orientation"];
                int64_t orientation = 0;
#if EXIV2_TEST_VERSION(0,28,0)
                orientation = orientationTag.toInt64();
#else
                orientation = orientationTag.toLong();
#endif
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
