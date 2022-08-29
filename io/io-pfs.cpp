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

#include <pfs/pfs.h>

#include "io-pfs.hpp"


namespace TGD {

FormatImportExportPFS::FormatImportExportPFS() :
    _f(nullptr),
    _arrayCount(-2)
{
}

FormatImportExportPFS::~FormatImportExportPFS()
{
    close();
}

Error FormatImportExportPFS::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        _f = stdin;
    else
        _f = fopen(fileName.c_str(), "rb");
    return _f ? ErrorNone : ErrorSysErrno;
}

Error FormatImportExportPFS::openForWriting(const std::string& fileName, bool append, const TagList&)
{
    if (fileName == "-")
        _f = stdout;
    else
        _f = fopen(fileName.c_str(), append ? "ab" : "wb");
    return _f ? ErrorNone : ErrorSysErrno;
}

void FormatImportExportPFS::close()
{
    if (_f) {
        if (_f != stdin && _f != stdout) {
            fclose(_f);
        }
        _f = nullptr;
    }
}

int FormatImportExportPFS::arrayCount()
{
    if (_arrayCount >= -1)
        return _arrayCount;

    if (!_f)
        return -1;

    // find offsets of all PFSs in the file
    off_t curPos = ftello(_f);
    if (curPos < 0) {
        _arrayCount = -1;
        return _arrayCount;
    }
    rewind(_f);
    pfs::DOMIO pfsio;
    while (hasMore()) {
        off_t arrayPos = ftello(_f);
        if (arrayPos < 0) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
        try {
            pfs::Frame *frame = pfsio.readFrame(_f);
            if (!frame) {
                _arrayOffsets.clear();
                _arrayCount = -1;
                return -1;
            }
            pfsio.freeFrame(frame);
        }
        catch (...) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
        _arrayOffsets.push_back(arrayPos);
        if (_arrayOffsets.size() == size_t(std::numeric_limits<int>::max()) && hasMore()) {
            _arrayOffsets.clear();
            _arrayCount = -1;
            return -1;
        }
    }
    if (fseeko(_f, curPos, SEEK_SET) < 0) {
        _arrayOffsets.clear();
        _arrayCount = -1;
        return -1;
    }
    _arrayCount = _arrayOffsets.size();
    return _arrayCount;
}

ArrayContainer FormatImportExportPFS::readArray(Error* error, int arrayIndex)
{
    // Seek if necessary
    if (arrayIndex >= 0) {
        if (arrayCount() < 0) {
            *error = ErrorSeekingNotSupported;
            return ArrayContainer();
        }
        if (arrayIndex >= arrayCount()) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        if (fseeko(_f, _arrayOffsets[arrayIndex], SEEK_SET) < 0) {
            *error = ErrorSysErrno;
            return ArrayContainer();
        }
    }

    // Read the PFS
    pfs::DOMIO pfsio;
    pfs::Frame* frame = nullptr;
    try {
        frame = pfsio.readFrame(_f);
        if (!frame) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
    }
    catch (...) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    }

    // Create the array and copy metadata
    size_t componentCount = 0;
    pfs::ChannelIteratorPtr cit0(frame->getChannelIterator());
    while (cit0->hasNext()) {
        cit0->getNext();
        componentCount++;
    }
    size_t width = frame->getWidth();
    size_t height = frame->getHeight();
    ArrayContainer r({ width, height }, componentCount, float32);
    pfs::TagIteratorPtr tit(frame->getTags()->getIterator());
    while (tit->hasNext()) {
        const char *tag_name = tit->getNext();
        r.globalTagList().set(tag_name, frame->getTags()->getString(tag_name));
    }
    pfs::ChannelIteratorPtr cit1(frame->getChannelIterator());
    size_t componentIndex = 0;
    std::vector<float *> channelData(componentCount);
    while (cit1->hasNext()) {
        pfs::Channel* channel = cit1->getNext();
        if (strcmp(channel->getName(), "X") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "XYZ/X");
        else if (strcmp(channel->getName(), "Y") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "XYZ/Y");
        else if (strcmp(channel->getName(), "Z") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "XYZ/Z");
        else if (strcmp(channel->getName(), "R") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "RED");
        else if (strcmp(channel->getName(), "G") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "GREEN");
        else if (strcmp(channel->getName(), "B") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "BLUE");
        else if (strcmp(channel->getName(), "SRGB/R") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "SRGB/R");
        else if (strcmp(channel->getName(), "SRGB/G") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "SRGB/G");
        else if (strcmp(channel->getName(), "SRGB/B") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "SRGB/B");
        else if (strcmp(channel->getName(), "ALPHA") == 0)
            r.componentTagList(componentIndex).set("INTERPRETATION", "ALPHA");
        else
            r.componentTagList(componentIndex).set("INTERPRETATION", channel->getName());
        channelData[componentIndex] = channel->getRawData();
        componentIndex++;
    }
    // Copy the data
    for (size_t y = 0; y < r.dimension(1); y++) {
        size_t pfs_y = r.dimension(1) - 1 - y;
        for (size_t x = 0; x < r.dimension(0); x++) {
            for (size_t c = 0; c < r.componentCount(); c++) {
                r.set<float>({ x, y }, c, channelData[c][pfs_y * r.dimension(0) + x]);
            }
        }
    }
    pfsio.freeFrame(frame);

    // Return the array
    return r;
}

bool FormatImportExportPFS::hasMore()
{
    int c = fgetc(_f);
    if (c == EOF) {
        return false;
    } else {
        ungetc(c, _f);
        return true;
    }
}

Error FormatImportExportPFS::writeArray(const ArrayContainer& array)
{
    if (array.dimensionCount() != 2
            || array.dimension(0) <= 0 || array.dimension(1) <= 0
            || array.dimension(0) > 65535 || array.dimension(1) > 65535
            || array.componentCount() <= 0 || array.componentCount() > 1024
            || array.componentType() != float32) {
        return ErrorFeaturesUnsupported;
    }

    pfs::DOMIO pfsio;
    pfs::Frame* frame = nullptr;
    std::vector<float*> channelData(array.componentCount());
    try {
        frame = pfsio.createFrame(array.dimension(0), array.dimension(1));
        for (auto gtit = array.globalTagList().cbegin(); gtit != array.globalTagList().cend(); gtit++)
            frame->getTags()->setString(gtit->first.c_str(), gtit->second.c_str());
        for (size_t componentIndex = 0; componentIndex < array.componentCount(); componentIndex++) {
            std::string interpretation = array.componentTagList(componentIndex).value("INTERPRETATION");
            std::string pfs_name = array.componentTagList(componentIndex).value("PFS/NAME");
            std::string channel_name;
            if (interpretation == "XYZ/X")
                channel_name = "X";
            else if (interpretation == "XYZ/Y")
                channel_name = "Y";
            else if (interpretation == "XYZ/Z")
                channel_name = "Z";
            else if (interpretation == "RED")
                channel_name = "R";
            else if (interpretation == "GREEN")
                channel_name = "G";
            else if (interpretation == "BLUE")
                channel_name = "B";
            else if (interpretation == "ALPHA")
                channel_name = "ALPHA";
            else if (pfs_name.length() > 0)
                channel_name = pfs_name;
            else if (interpretation.length() > 0)
                channel_name = interpretation;
            else
                channel_name = std::to_string(componentIndex);     
            pfs::Channel* channel = frame->createChannel(channel_name.c_str());
            channelData[componentIndex] = channel->getRawData();
        }
    }
    catch (...) {
        if (frame)
            pfsio.freeFrame(frame);
        return ErrorLibrary;
    }

    for (size_t y = 0; y < array.dimension(1); y++) {
        size_t pfs_y = array.dimension(1) - 1 - y;
        for (size_t x = 0; x < array.dimension(0); x++) {
            for (size_t c = 0; c < array.componentCount(); c++) {
                channelData[c][pfs_y * array.dimension(0) + x] = array.get<float>({ x, y }, c);
            }
        }
    }

    try {
        pfsio.writeFrame(frame, _f);
    }
    catch (...) {
        pfsio.freeFrame(frame);
        return ErrorLibrary;
    }
    pfsio.freeFrame(frame);
    if (fflush(_f) != 0) {
        return ErrorSysErrno;
    }
    return ErrorNone;
}

extern "C" FormatImportExport* FormatImportExportFactory_pfs()
{
    return new FormatImportExportPFS();
}

}
