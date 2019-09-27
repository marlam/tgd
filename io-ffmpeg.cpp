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
#include <limits>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include "io-ffmpeg.hpp"
#include "io-utils.hpp"


namespace TAD {

class FFmpeg
{
public:
    AVFormatContext* formatCtx;
    AVCodecContext* codecCtx;
    AVPixelFormat pixFmt;
    int streamIndex;
    AVStream* stream;
    SwsContext* swsCtx;
    AVFrame* videoFrame;

    FFmpeg() :
        formatCtx(nullptr),
        codecCtx(nullptr),
        streamIndex(-1),
        stream(nullptr),
        swsCtx(nullptr),
        videoFrame(nullptr)
    {
    }
};

FormatImportExportFFMPEG::FormatImportExportFFMPEG()
{
    av_log_set_level(AV_LOG_ERROR);
    _ffmpeg = new FFmpeg;
    close();
}

FormatImportExportFFMPEG::~FormatImportExportFFMPEG()
{
    close();
    delete _ffmpeg;
}

Error FormatImportExportFFMPEG::openForReading(const std::string& fileName, const TagList&)
{
    if (fileName == "-")
        return ErrorInvalidData;

    if (avformat_open_input(&(_ffmpeg->formatCtx), fileName.c_str(), nullptr, nullptr) < 0
            || avformat_find_stream_info(_ffmpeg->formatCtx, nullptr) < 0
            || (_ffmpeg->streamIndex = av_find_best_stream(_ffmpeg->formatCtx,
                    AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0)) < 0) {
        return ErrorInvalidData;
    }
    for (int i = 0; i < int(_ffmpeg->formatCtx->nb_streams); i++) {
        // this helps with seeking:
        if (i != _ffmpeg->streamIndex)
            _ffmpeg->formatCtx->streams[i]->discard = AVDISCARD_ALL;
    }
    _ffmpeg->stream = _ffmpeg->formatCtx->streams[_ffmpeg->streamIndex];
    if (_ffmpeg->stream->nb_frames >= std::numeric_limits<int>::max()) {
        // we handle array indices as int
        return ErrorFeaturesUnsupported;
    }

    AVCodec *dec = avcodec_find_decoder(_ffmpeg->stream->codecpar->codec_id);
    if (!dec) {
        close();
        return ErrorLibrary;
    }
    _ffmpeg->codecCtx = avcodec_alloc_context3(dec);
    if (!_ffmpeg->codecCtx) {
        close();
        errno = ENOMEM;
        return ErrorSysErrno;
    }
    if (avcodec_parameters_to_context(_ffmpeg->codecCtx, _ffmpeg->stream->codecpar) < 0) {
        close();
        return ErrorLibrary;
    }
    _ffmpeg->codecCtx->thread_count = 0; // enable automatic multi-threaded decoding
    if (avcodec_open2(_ffmpeg->codecCtx, dec, nullptr) < 0) {
        close();
        return ErrorLibrary;
    }

    int w = _ffmpeg->codecCtx->width;
    int h = _ffmpeg->codecCtx->height;
    _ffmpeg->pixFmt = _ffmpeg->codecCtx->pix_fmt;
    const AVPixFmtDescriptor* pixFmtDesc = av_pix_fmt_desc_get(_ffmpeg->pixFmt);
    int componentCount = pixFmtDesc->nb_components;
    if (w < 1 || h < 1 || componentCount < 1 || componentCount > 4) {
        close();
        return ErrorInvalidData;
    }
    Type type = uint8;
    for (int i = 0; i < componentCount; i++) {
        if (pixFmtDesc->comp[i].depth > 8) {
            type = uint16;
            break;
        }        
    }
    _desc = ArrayDescription({ size_t(w), size_t(h) }, componentCount, type);

    AVPixelFormat dstPixFmt;
    if (type == uint8) {
        if (componentCount == 1) {
            dstPixFmt = AV_PIX_FMT_GRAY8;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
        } else if (componentCount == 2) {
            dstPixFmt = AV_PIX_FMT_YA8;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
            _desc.componentTagList(1).set("INTERPRETATION", "ALPHA");
        } else if (componentCount == 3) {
            dstPixFmt = AV_PIX_FMT_RGB24;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            _desc.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            _desc.componentTagList(2).set("INTERPRETATION", "SRGB/B");
        } else {
            dstPixFmt = AV_PIX_FMT_RGBA;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            _desc.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            _desc.componentTagList(2).set("INTERPRETATION", "SRGB/B");
            _desc.componentTagList(3).set("INTERPRETATION", "ALPHA");
        }
    } else {
        if (componentCount == 1) {
            dstPixFmt = AV_PIX_FMT_GRAY16;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
        } else if (componentCount == 2) {
            dstPixFmt = AV_PIX_FMT_YA16;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
            _desc.componentTagList(1).set("INTERPRETATION", "ALPHA");
        } else if (componentCount == 3) {
            dstPixFmt = AV_PIX_FMT_RGB48;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            _desc.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            _desc.componentTagList(2).set("INTERPRETATION", "SRGB/B");
        } else {
            dstPixFmt = AV_PIX_FMT_RGBA64;
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            _desc.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            _desc.componentTagList(2).set("INTERPRETATION", "SRGB/B");
            _desc.componentTagList(3).set("INTERPRETATION", "ALPHA");
        }
    }

    _ffmpeg->swsCtx = sws_getContext(w, h, _ffmpeg->pixFmt, w, h, dstPixFmt,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!_ffmpeg->swsCtx) {
        close();
        return ErrorLibrary;
    }

    _ffmpeg->videoFrame = av_frame_alloc();
    if (!_ffmpeg->videoFrame) {
        close();
        errno = ENOMEM;
        return ErrorSysErrno;
    }

    _fileName = fileName;
    return ErrorNone;
}

Error FormatImportExportFFMPEG::openForWriting(const std::string&, bool, const TagList&)
{
    return ErrorFeaturesUnsupported;
}

void FormatImportExportFFMPEG::close()
{
    if (_ffmpeg->formatCtx) {
        avformat_close_input(&(_ffmpeg->formatCtx));
        _ffmpeg->formatCtx = nullptr;
    }
    if (_ffmpeg->codecCtx) {
        avcodec_free_context(&(_ffmpeg->codecCtx));
        _ffmpeg->codecCtx = nullptr;
    }
    if (_ffmpeg->swsCtx) {
        sws_freeContext(_ffmpeg->swsCtx);
        _ffmpeg->swsCtx = nullptr;
    }
    if (_ffmpeg->videoFrame) {
        av_frame_free(&(_ffmpeg->videoFrame));
        _ffmpeg->videoFrame = nullptr;
    }
    _desc = ArrayDescription();
    _minDTS = 0; // bad guess, but INT64_MIN seems to cause problems when seeking to it
    _frameDTSs.clear();
    _keyFrames.clear();
    _fileEof = false;
    _sentEofPacket = false;
    _codecEof = false;
    _indexOfLastReadFrame = -1;
}

bool FormatImportExportFFMPEG::hardReset()
{
    // Hard reset by closing and reopening, because seeking to minDTS is not reliable
    int64_t bakMinDTS = _minDTS;
    std::vector<int64_t> bakFrameDTSs = _frameDTSs;
    std::vector<int> bakKeyFrames = _keyFrames;
    close();
    if (openForReading(_fileName, TagList()) != ErrorNone)
        return false;
    _minDTS = bakMinDTS;
    _frameDTSs = bakFrameDTSs;
    _keyFrames = bakKeyFrames;
    return true;
}

int FormatImportExportFFMPEG::arrayCount()
{
    if (_frameDTSs.size() == 0) {
        // We need to decode and scan the whole file, otherwise frame-precise seeking
        // will not be possible.
        if (_indexOfLastReadFrame >= 0) {
            // we already read a frame and therefore need to reset our state
            close();
            if (openForReading(_fileName, TagList()) != ErrorNone)
                return -1;
        }
        AVPacket pkt;
        bool fileEof = false;
        bool sentEofPacket = false;
        bool codecEof = false;
        do {
            int gotFrame = 0;
            do {
                if (!fileEof) {
                    int ret = av_read_frame(_ffmpeg->formatCtx, &pkt);
                    if (ret < 0 && ret != AVERROR_EOF) {
                        return -1;
                    }
                    if (ret == 0 && pkt.stream_index != _ffmpeg->streamIndex) {
                        av_packet_unref(&pkt);
                        continue;
                    }
                    fileEof = (ret == AVERROR_EOF);
                }
                if (fileEof) {
                    av_init_packet(&pkt);
                    pkt.data = nullptr;
                    pkt.size = 0;
                }
                if (pkt.data || !sentEofPacket) {
                    if (avcodec_send_packet(_ffmpeg->codecCtx, &pkt) < 0) {
                        return -1;
                    }
                }
                if (!pkt.data) {
                    sentEofPacket = true;
                }
                int ret = avcodec_receive_frame(_ffmpeg->codecCtx, _ffmpeg->videoFrame);
                if (ret == AVERROR(EAGAIN)) {
                    // we need another packet
                } else if (ret == AVERROR_EOF) {
                    codecEof = true;
                } else if (ret < 0) {
                    return -1;
                } else {
                    gotFrame = 1;
                }
                av_packet_unref(&pkt);
            } while (!codecEof && !gotFrame);

            if (!gotFrame) {
                break;
            }
            int64_t dts = _ffmpeg->videoFrame->pkt_dts;
            _frameDTSs.push_back(dts);
            if (_frameDTSs.size() == 1 || dts < _minDTS) {
                _minDTS = dts;
            }
            if (_ffmpeg->videoFrame->key_frame) {
                _keyFrames.push_back(_frameDTSs.size() - 1);
            }
        } while (!codecEof);
        if (!hardReset())
            return -1;
    }
    return _frameDTSs.size();
}

ArrayContainer FormatImportExportFFMPEG::readArray(Error* error, int arrayIndex)
{
    if (arrayIndex < 0 && _codecEof) {
        *error = ErrorInvalidData;
        return ArrayContainer();
    } else if (arrayIndex >= 0) {
        if (arrayIndex >= arrayCount()) { // this builds an index of DTS times if we did not do it yet
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        // find the key frame that has arrayIndex or precedes arrayIndex
        int precedingKeyFrameIndex = -1;
        for (auto it = _keyFrames.cbegin(); it != _keyFrames.cend(); ++it) {
            if (*it <= arrayIndex && *it > precedingKeyFrameIndex)
                precedingKeyFrameIndex = *it;
        }
        // check if we should seek or just skip frames until we're there
        if (_indexOfLastReadFrame == -1 && arrayIndex == 0) {
            // we read the first frame, no need to seek or skip
        } else if (arrayIndex > _indexOfLastReadFrame && precedingKeyFrameIndex <= _indexOfLastReadFrame) {
            // it is cheaper to skip frames until we arrive at the frame we want
            // so do nothing here
        } else {
            // find the DTS of that key frame
            int64_t precedingKeyFrameDTS;
            if (precedingKeyFrameIndex >= 0)
                precedingKeyFrameDTS = _frameDTSs[precedingKeyFrameIndex];
            else
                precedingKeyFrameDTS = _minDTS;
            // seek to this DTS
            avformat_seek_file(_ffmpeg->formatCtx, _ffmpeg->streamIndex,
                    std::numeric_limits<int64_t>::min(),
                    precedingKeyFrameDTS,
                    std::numeric_limits<int64_t>::max(),
                    0);
            // flush the decoder buffers
            avcodec_flush_buffers(_ffmpeg->codecCtx);
            // reset EOF flags
            _fileEof = false;
            _sentEofPacket = false;
            _codecEof = false;
        }
    }

    AVPacket pkt;
    int frameIndex = -1;
    do {
        int gotFrame = 0;
        do {
            if (!_fileEof) {
                int ret = av_read_frame(_ffmpeg->formatCtx, &pkt);
                if (ret < 0 && ret != AVERROR_EOF) {
                    close();
                    *error = ErrorInvalidData;
                    return ArrayContainer();
                }
                if (ret == 0 && pkt.stream_index != _ffmpeg->streamIndex) {
                    av_packet_unref(&pkt);
                    continue;
                }
                _fileEof = (ret == AVERROR_EOF);
            }
            if (_fileEof) {
                av_init_packet(&pkt);
                pkt.data = nullptr;
                pkt.size = 0;
            }
            if (pkt.data || !_sentEofPacket) {
                if (avcodec_send_packet(_ffmpeg->codecCtx, &pkt) < 0) {
                    close();
                    *error = ErrorInvalidData;
                    return ArrayContainer();
                }
            }
            if (!pkt.data) {
                _sentEofPacket = true;
            }
            int ret = avcodec_receive_frame(_ffmpeg->codecCtx, _ffmpeg->videoFrame);
            if (ret == AVERROR(EAGAIN)) {
                // we need another packet
            } else if (ret == AVERROR_EOF) {
                _codecEof = true;
            } else if (ret < 0) {
                close();
                *error = ErrorInvalidData;
                return ArrayContainer();
            } else {
                gotFrame = 1;
            }
            av_packet_unref(&pkt);
        } while (!_codecEof && !gotFrame);

        if (!gotFrame && arrayIndex < 0) {
            close();
            *error = ErrorInvalidData;
            return ArrayContainer();
        }

        if (gotFrame) {
            if (_ffmpeg->videoFrame->width != int(_desc.dimension(0))
                    || _ffmpeg->videoFrame->height != int(_desc.dimension(1))
                    || _ffmpeg->videoFrame->format != _ffmpeg->pixFmt) {
                // we don't support changing specs
                close();
                *error = ErrorInvalidData;
                return ArrayContainer();
            }

            // Get frame index from its DTS
            int64_t dts = _ffmpeg->videoFrame->pkt_dts;
            if (arrayIndex < 0) {
                frameIndex = _indexOfLastReadFrame + 1;
            } else {
                for (size_t i = 0; i < _frameDTSs.size(); i++) {
                    if (_frameDTSs[i] == dts) {
                        frameIndex = i;
                        break;
                    }
                }
                if (frameIndex == -1) {
                    close();
                    *error = ErrorInvalidData;
                    return ArrayContainer();
                }
            }
        }

        // Check if seeking worked
        if (!gotFrame || (arrayIndex >= 0 && frameIndex > arrayIndex)) {
            // We structured our seeking so that this should never ever happen,
            // but seeking in FFmpeg is often ... let's say surprising.
            // So we have a terrible fallback here: hard reset
            if (!hardReset()) {
                close();
                *error = ErrorInvalidData;
                return ArrayContainer();
            }
            frameIndex = -1;
        }
    } while (frameIndex < arrayIndex);

    ArrayContainer r(_desc);
    uint8_t* dst[4] = { static_cast<uint8_t*>(r.data()), nullptr, nullptr, nullptr };
    int dstStride[4] = { int(r.dimension(0) * r.elementSize()), 0, 0, 0 };
    sws_scale(_ffmpeg->swsCtx, _ffmpeg->videoFrame->data, _ffmpeg->videoFrame->linesize, 0, _ffmpeg->videoFrame->height, dst, dstStride);
    reverseY(r);
    if (arrayIndex < 0)
        _indexOfLastReadFrame++;
    else
        _indexOfLastReadFrame = arrayIndex;
    return r;
}

bool FormatImportExportFFMPEG::hasMore()
{
    int frameCount = _ffmpeg->stream->nb_frames;
    if (frameCount <= 0)
        frameCount = arrayCount();
    return (_indexOfLastReadFrame < frameCount - 1);
}

Error FormatImportExportFFMPEG::writeArray(const ArrayContainer&)
{
    return ErrorFeaturesUnsupported;
}

extern "C" FormatImportExport* FormatImportExportFactory_ffmpeg()
{
    return new FormatImportExportFFMPEG();
}

}
