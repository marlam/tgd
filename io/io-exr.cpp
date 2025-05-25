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

#include <cstdio>
#include <cstring>

#include <OpenEXR/openexr.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfStringAttribute.h>
#include <OpenEXR/ImfFrameBuffer.h>

#include "io-exr.hpp"
#include "io-utils.hpp"


namespace TGD {

FormatImportExportEXR::FormatImportExportEXR() : _arrayWasReadOrWritten(false)
{
}

FormatImportExportEXR::~FormatImportExportEXR()
{
}

Error FormatImportExportEXR::openForReading(const std::string& fileName, const TagList&)
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

Error FormatImportExportEXR::openForWriting(const std::string& fileName, bool append, const TagList&)
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

void FormatImportExportEXR::close()
{
    _arrayWasReadOrWritten = false;
}

int FormatImportExportEXR::arrayCount()
{
    return (_fileName.length() == 0 ? -1 : 1);
}

using namespace Imf;
using namespace Imath;

ArrayContainer FormatImportExportEXR::readArray(Error* error, int arrayIndex, const Allocator& alloc)
{
    if (arrayIndex > 0) {
        *error = ErrorSeekingNotSupported;
        return ArrayContainer();
    }

    try {
        InputFile file(_fileName.c_str());
        Box2i dw = file.header().dataWindow();
        int width = dw.max.x - dw.min.x + 1;
        int height = dw.max.y - dw.min.y + 1;
        if (width < 1 || height < 1) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        const ChannelList &channellist = file.header().channels();
        size_t channelCount = 0;
        for (ChannelList::ConstIterator iter = channellist.begin(); iter != channellist.end(); iter++) {
            channelCount++;
        }
        if (channelCount < 1) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }

        ArrayContainer r({ size_t(width), size_t(height) }, channelCount, float32, alloc);
        for (auto it = file.header().begin(); it != file.header().end(); it++) {
            if (std::string(it.attribute().typeName()) == std::string("string")) {
                r.globalTagList().set(it.name(),
                        file.header().typedAttribute<StringAttribute>(it.name()).value());
            }
        }
        char* charData = static_cast<char*>(r.data());
        FrameBuffer framebuffer;
        int channelIndex = 0;
        if (channellist.findChannel("Y")) {
            r.componentTagList(channelIndex).set("INTERPRETATION", "XYZ/Y");
            framebuffer.insert("Y", Slice(FLOAT, charData + channelIndex * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            channelIndex++;
        }
        if (channellist.findChannel("R")) {
            r.componentTagList(channelIndex).set("INTERPRETATION", "RED");
            framebuffer.insert("R", Slice(FLOAT, charData + channelIndex * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            channelIndex++;
        }
        if (channellist.findChannel("G")) {
            r.componentTagList(channelIndex).set("INTERPRETATION", "GREEN");
            framebuffer.insert("G", Slice(FLOAT, charData + channelIndex * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            channelIndex++;
        }
        if (channellist.findChannel("B")) {
            r.componentTagList(channelIndex).set("INTERPRETATION", "BLUE");
            framebuffer.insert("B", Slice(FLOAT, charData + channelIndex * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            channelIndex++;
        }
        if (channellist.findChannel("A")) {
            r.componentTagList(channelIndex).set("INTERPRETATION", "ALPHA");
            framebuffer.insert("A", Slice(FLOAT, charData + channelIndex * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            channelIndex++;
        }
        if (channellist.findChannel("Z")) {
            r.componentTagList(channelIndex).set("INTERPRETATION", "DEPTH");
            framebuffer.insert("Z", Slice(FLOAT, charData + channelIndex * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            channelIndex++;
        }
        for (ChannelList::ConstIterator iter = channellist.begin(); iter != channellist.end(); iter++) {
            if (std::strcmp(iter.name(), "Y") == 0
                    || std::strcmp(iter.name(), "R") == 0
                    || std::strcmp(iter.name(), "G") == 0
                    || std::strcmp(iter.name(), "B") == 0
                    || std::strcmp(iter.name(), "A") == 0
                    || std::strcmp(iter.name(), "Z") == 0) {
                continue;
            }
            r.componentTagList(channelIndex).set("INTERPRETATION", iter.name());
            framebuffer.insert(iter.name(), Slice(FLOAT, charData + channelIndex * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            channelIndex++;
        }
        file.setFrameBuffer(framebuffer);
        file.readPixels(dw.min.y, dw.max.y);

        reverseY(r);
        _arrayWasReadOrWritten = true;
        return r;
    }
    catch (...) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
}

bool FormatImportExportEXR::hasMore()
{
    return !_arrayWasReadOrWritten;
}

Error FormatImportExportEXR::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > 65535 || array.dimension(1) > 65535
            || array.componentCount() < 1
            || array.componentType() != float32
            || _arrayWasReadOrWritten) {
        return ErrorFeaturesUnsupported;
    }

    try {
        Header header(array.dimension(0), array.dimension(1), 1.0f, Imath::V2f(0, 0), 1.0f, INCREASING_Y, PIZ_COMPRESSION);
        for (auto it = array.globalTagList().cbegin(); it != array.globalTagList().cend(); it++) {
            header.insert(it->first.c_str(), StringAttribute(it->second.c_str()));
        }
        std::vector<std::string> channelNames(array.componentCount());
        for (size_t c = 0; c < channelNames.size(); c++) {
            std::string channelName;
            std::string interpretationValue = array.componentTagList(c).value("INTERPRETATION");
            if (interpretationValue == "RED")
                channelName = "R";
            else if (interpretationValue == "GREEN")
                channelName = "G";
            else if (interpretationValue == "BLUE")
                channelName = "B";
            else if (interpretationValue == "ALPHA")
                channelName = "A";
            else if (interpretationValue == "XYZ/Y")
                channelName = "Y";
            else if (interpretationValue == "DEPTH")
                channelName = "Z";
            else if (interpretationValue.size() > 0)
                channelName = interpretationValue;
            else
                channelName = std::string("U") + std::to_string(c);
            channelNames[c] = channelName;
            header.channels().insert(channelName.c_str(), Channel(FLOAT));
        }
        OutputFile file(_fileName.c_str(), header);
        FrameBuffer framebuffer;
        ArrayContainer reversedArray = array.deepCopy();
        reverseY(reversedArray);
        char* charData = static_cast<char*>(reversedArray.data());
        for (size_t c = 0; c < array.componentCount(); c++) {
            framebuffer.insert(channelNames[c].c_str(),
                    Slice(FLOAT, charData + c * sizeof(float),
                        array.componentCount() * sizeof(float),
                        array.componentCount() * array.dimension(0) * sizeof(float)));
        }
        file.setFrameBuffer(framebuffer);
        file.writePixels(array.dimension(1));
        _arrayWasReadOrWritten = true;
        return ErrorNone;
    }
    catch (...) {
        return ErrorLibrary;
    }
}

extern "C" FormatImportExport* FormatImportExportFactory_exr()
{
    return new FormatImportExportEXR();
}

}
