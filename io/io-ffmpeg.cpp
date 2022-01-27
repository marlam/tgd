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
    AVPacket* pkt;
    bool fileEof;

    FFmpeg() :
        formatCtx(nullptr),
        codecCtx(nullptr),
        streamIndex(-1),
        stream(nullptr),
        swsCtx(nullptr),
        videoFrame(nullptr),
        pkt(nullptr),
        fileEof(false)
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

    const AVCodec *dec = avcodec_find_decoder(_ffmpeg->stream->codecpar->codec_id);
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
    _ffmpeg->pkt = av_packet_alloc();
    if (!_ffmpeg->videoFrame || !_ffmpeg->pkt) {
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
    if (_ffmpeg->pkt) {
        av_packet_free(&(_ffmpeg->pkt));
        _ffmpeg->pkt = nullptr;
    }
    _ffmpeg->fileEof = false;
    _desc = ArrayDescription();
    _minDTS = 0; // bad guess, but INT64_MIN seems to cause problems when seeking to it
    _unreliableTimeStamps = false;
    _frameDTSs.clear();
    _framePTSs.clear();
    _keyFrames.clear();
    _indexOfLastReadFrame = -1;
}

bool FormatImportExportFFMPEG::hardReset()
{
    // Hard reset by closing and reopening, because seeking to minDTS is not reliable
    int64_t bakMinDTS = _minDTS;
    bool bakUnreliableTimeStamps = _unreliableTimeStamps;
    std::vector<int64_t> bakFrameDTSs = _frameDTSs;
    std::vector<int64_t> bakFramePTSs = _framePTSs;
    std::vector<int> bakKeyFrames = _keyFrames;
    close();
    if (openForReading(_fileName, TagList()) != ErrorNone)
        return false;
    _minDTS = bakMinDTS;
    _unreliableTimeStamps = bakUnreliableTimeStamps;
    _frameDTSs = bakFrameDTSs;
    _framePTSs = bakFramePTSs;
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
        int ret;
        for (;;) {
            ret = avcodec_receive_frame(_ffmpeg->codecCtx, _ffmpeg->videoFrame);
            if (ret == AVERROR_EOF) {
                break;
            } else if (ret == AVERROR(EAGAIN)) {
                for (;;) {
                    ret = av_read_frame(_ffmpeg->formatCtx, _ffmpeg->pkt);
                    if (ret == AVERROR_EOF) {
                        if (avcodec_send_packet(_ffmpeg->codecCtx, NULL) < 0) {
                            return -1;
                        }
                        break;
                    } else if (ret < 0) {
                        return -1;
                    } else if (_ffmpeg->pkt->stream_index == _ffmpeg->streamIndex) {
                        if (avcodec_send_packet(_ffmpeg->codecCtx, _ffmpeg->pkt) < 0) {
                            return -1;
                        }
                        av_packet_unref(_ffmpeg->pkt);
                        break;
                    } else {
                        av_packet_unref(_ffmpeg->pkt);
                    }
                }
            } else if (ret < 0) {
                return -1;
            } else {
                int64_t dts = _ffmpeg->videoFrame->pkt_dts;
                int64_t pts = _ffmpeg->videoFrame->pts;
                if (dts == AV_NOPTS_VALUE && pts == AV_NOPTS_VALUE) {
                    _unreliableTimeStamps = true;
                }
                _frameDTSs.push_back(dts);
                _framePTSs.push_back(pts);
                //fprintf(stderr, "frame %zu: dts=%ld pts=%ld\n", _frameDTSs.size(), dts, pts);
                if (_frameDTSs.size() == 1 || dts < _minDTS) {
                    _minDTS = dts;
                }
                if (_ffmpeg->videoFrame->key_frame) {
                    _keyFrames.push_back(_frameDTSs.size() - 1);
                }
            }
        }
        if (!hardReset())
            return -1;
    }
    return _frameDTSs.size();
}

ArrayContainer FormatImportExportFFMPEG::readArray(Error* error, int arrayIndex)
{
    bool seeked = false;
    if (arrayIndex >= 0) {
        // reset file EOF flag
        _ffmpeg->fileEof = false;
        // build an index of DTS times if we did not do it yet
        if (arrayIndex >= arrayCount()) {
            *error = ErrorInvalidData;
            return ArrayContainer();
        }
        // only perform proper seeking if we have reliable time stamps
        if (!_unreliableTimeStamps) {
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
                        precedingKeyFrameDTS,
                        0);
                // flush the decoder buffers
                avcodec_flush_buffers(_ffmpeg->codecCtx);
                // set a flag that tells the code below that we did an actual seek
                seeked = true;
            }
        } else {
            // no reliable time stamps: we cannot call avformat_seek_file()
            // and therefore have to skip frames until we arrive at the requested one.
            if (arrayIndex > _indexOfLastReadFrame) {
                // ok, no need to do anything: the requested array is still to come
            } else {
                // we are past the requested array and need to rewind HARD to start
                // over with the first frame
                if (!hardReset()) {
                    close();
                    *error = ErrorInvalidData;
                    return ArrayContainer();
                }
            }
        }
    }

    bool triedHardReset = false;
    int ret;
    for (;;) {
        ret = avcodec_receive_frame(_ffmpeg->codecCtx, _ffmpeg->videoFrame);
        if (ret == AVERROR_EOF) {
            if (_ffmpeg->fileEof) {
                break;
            } else {
                close();
                *error = ErrorInvalidData;
                return ArrayContainer();
            }
        } else if (ret == AVERROR(EAGAIN)) {
            for (;;) {
                ret = av_read_frame(_ffmpeg->formatCtx, _ffmpeg->pkt);
                if (ret == AVERROR_EOF) {
                    _ffmpeg->fileEof = true;
                    if (avcodec_send_packet(_ffmpeg->codecCtx, NULL) < 0) {
                        close();
                        *error = ErrorInvalidData;
                        return ArrayContainer();
                    }
                    break;
                } else if (ret < 0) {
                    close();
                    *error = ErrorInvalidData;
                    return ArrayContainer();
                } else if (_ffmpeg->pkt->stream_index == _ffmpeg->streamIndex) {
                    if (avcodec_send_packet(_ffmpeg->codecCtx, _ffmpeg->pkt) < 0) {
                        close();
                        *error = ErrorInvalidData;
                        return ArrayContainer();
                    }
                    av_packet_unref(_ffmpeg->pkt);
                    break;
                } else {
                    av_packet_unref(_ffmpeg->pkt);
                }
            }
        } else if (ret < 0) {
            close();
            *error = ErrorInvalidData;
            return ArrayContainer();
        } else {
            if (_ffmpeg->videoFrame->width != int(_desc.dimension(0))
                    || _ffmpeg->videoFrame->height != int(_desc.dimension(1))
                    || _ffmpeg->videoFrame->format != _ffmpeg->pixFmt) {
                // we don't support changing specs
                close();
                *error = ErrorInvalidData;
                return ArrayContainer();
            }
            if (arrayIndex < 0) {
                // We are ok once we read one frame
                _indexOfLastReadFrame++;
                break;
            } else {
                // Get frame index from its DTS and PTS pair (the DTS is not necessarily unique)
                int frameIndex = -1;
                if (!seeked) {
                    frameIndex = _indexOfLastReadFrame + 1;
                } else {
                    int64_t dts = _ffmpeg->videoFrame->pkt_dts;
                    int64_t pts = _ffmpeg->videoFrame->pts;
                    for (size_t i = 0; i < _frameDTSs.size(); i++) {
                        if (_frameDTSs[i] == dts && _framePTSs[i] == pts) {
                            frameIndex = i;
                            break;
                        }
                    }
                }
                if (frameIndex == -1) {
                    close();
                    *error = ErrorInvalidData;
                    return ArrayContainer();
                }
                _indexOfLastReadFrame = frameIndex;
                if (frameIndex == arrayIndex) {
                    // We are ok once we read the frame with the index we seek;
                    // otherwise we read more frames until we arrive there
                    break;
                } else if (frameIndex > arrayIndex) {
                    // We structured our seeking so that this should never ever happen,
                    // but seeking in FFmpeg is often ... let's say surprising.
                    // So we have a terrible fallback here: hard reset
                    if (triedHardReset || !hardReset()) {
                        close();
                        *error = ErrorInvalidData;
                        return ArrayContainer();
                    }
                    triedHardReset = true;
                }
            }
        }
    }

    ArrayContainer r(_desc);
    uint8_t* dst[4] = { static_cast<uint8_t*>(r.data()), nullptr, nullptr, nullptr };
    int dstStride[4] = { int(r.dimension(0) * r.elementSize()), 0, 0, 0 };
    sws_scale(_ffmpeg->swsCtx, _ffmpeg->videoFrame->data, _ffmpeg->videoFrame->linesize, 0, _ffmpeg->videoFrame->height, dst, dstStride);
    reverseY(r);
    return r;
}

bool FormatImportExportFFMPEG::hasMore()
{
    if (_ffmpeg->fileEof) {
        /* this flag overrides nb_frames because the latter is unreliable, e.g. with .wmv files */
        return false;
    }
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
