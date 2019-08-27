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

#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>

#include "io-exr.hpp"


namespace TAD {

FormatImportExportEXR::FormatImportExportEXR() : _arrayWasRead(false)
{
}

FormatImportExportEXR::~FormatImportExportEXR()
{
}

Error FormatImportExportEXR::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-") {
        errno = EINVAL;
        return ErrorSysErrno;
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
        errno = EINVAL;
        return ErrorSysErrno;
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

Error FormatImportExportEXR::close()
{
    return ErrorNone;
}

int FormatImportExportEXR::arrayCount()
{
    return (_fileName.length() == 0 ? -1 : 1);
}

static void reverseY(size_t height, size_t line_size, unsigned char* data)
{
    std::vector<unsigned char> tmp_line(line_size);
    for (size_t y = 0; y < height / 2; y++) {
        size_t ty = height - 1 - y;
        std::memcpy(&(tmp_line[0]), &(data[ty * line_size]), line_size);
        std::memcpy(&(data[ty * line_size]), &(data[y * line_size]), line_size);
        std::memcpy(&(data[y * line_size]), &(tmp_line[0]), line_size);
    }
}

static void reverseY(ArrayContainer& array)
{
    reverseY(array.dimension(1),
            array.dimension(0) * array.elementSize(),
            static_cast<unsigned char*>(array.data()));
}

using namespace Imf;
using namespace Imath;

ArrayContainer FormatImportExportEXR::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex > 0) {
        *error = ErrorSeekingNotSupported;
        return ArrayContainer();
    }

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
    for (ChannelList::ConstIterator iter = channellist.begin(); iter != channellist.end(); iter++)
        channelCount++;
    if (channelCount < 1 || channelCount > 4) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    size_t w = width;
    size_t h = height;
    ArrayContainer r({ w, h }, channelCount, float32);
    char* charData = static_cast<char*>(r.data());
    FrameBuffer framebuffer;
    if ((channelCount == 3 || channelCount == 4)
            && channellist.findChannel("R") && channellist.findChannel("G") && channellist.findChannel("B")
            && (channelCount == 3 || channellist.findChannel("A"))) {
        r.componentTagList(0).set("INTERPRETATION", "RED");
        framebuffer.insert("R", Slice(FLOAT, charData + 0 * sizeof(float),
                    channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
        r.componentTagList(1).set("INTERPRETATION", "GREEN");
        framebuffer.insert("G", Slice(FLOAT, charData + 1 * sizeof(float),
                    channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
        r.componentTagList(2).set("INTERPRETATION", "BLUE");
        framebuffer.insert("B", Slice(FLOAT, charData + 2 * sizeof(float),
                    channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
        if (channelCount == 4) {
            r.componentTagList(3).set("INTERPRETATION", "ALPHA");
            framebuffer.insert("A", Slice(FLOAT, charData + 3 * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
        }
    } else {
        int c = 0;
        for (ChannelList::ConstIterator iter = channellist.begin(); iter != channellist.end(); iter++) {
            framebuffer.insert(iter.name(), Slice(FLOAT, charData + c * sizeof(float),
                        channelCount * sizeof(float), channelCount * width * sizeof(float), 1, 1, 0.0f));
            c++;
        }
    }
    file.setFrameBuffer(framebuffer);
    file.readPixels(dw.min.y, dw.max.y);

    reverseY(r);
    _arrayWasRead = true;
    return r;
}

bool FormatImportExportEXR::hasMore()
{
    return !_arrayWasRead;
}

Error FormatImportExportEXR::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > 65535 || array.dimension(1) > 65535
            || array.componentCount() < 1 || array.componentCount() > 4
            || array.componentType() != float32) {
        return ErrorFeaturesUnsupported;
    }

    Header header(array.dimension(0), array.dimension(1), 1.0f, Imath::V2f(0, 0), 1.0f, INCREASING_Y, PIZ_COMPRESSION);
    std::string channel_names[4] = { "U0", "U1", "U2", "U3" };
    if (array.componentCount() == 1) {
        channel_names[0] = "Y";
    } else if (array.componentCount() == 2) {
        channel_names[0] = "Y";
        channel_names[1] = "A";
    } else if (array.componentCount() == 3) {
        channel_names[0] = "R";
        channel_names[1] = "G";
        channel_names[2] = "B";
    } else {
        channel_names[0] = "R";
        channel_names[1] = "G";
        channel_names[2] = "B";
        channel_names[3] = "A";
    }
    for (size_t c = 0; c < array.componentCount(); c++) {
        header.channels().insert(channel_names[c].c_str(), Channel(FLOAT));
    }
    OutputFile file(_fileName.c_str(), header);
    FrameBuffer framebuffer;
    ArrayContainer reversedArray = array.deepCopy();
    reverseY(reversedArray);
    char* charData = static_cast<char*>(reversedArray.data());
    for (size_t c = 0; c < array.componentCount(); c++) {
        framebuffer.insert(channel_names[c].c_str(),
                Slice(FLOAT, charData + c * sizeof(float),
                    array.componentCount() * sizeof(float),
                    array.componentCount() * array.dimension(0) * sizeof(float)));
    }
    file.setFrameBuffer(framebuffer);
    file.writePixels(array.dimension(1));
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_exr()
{
    return new FormatImportExportEXR();
}

}
