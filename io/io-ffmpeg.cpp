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

#include <cerrno>
#include <limits>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/cpu.h>
#include <libswscale/swscale.h>
}

#include "io-ffmpeg.hpp"
#include "io-utils.hpp"


namespace TGD {

class FFmpeg
{
public:
    AVFormatContext* formatCtx;
    AVCodecContext* codecCtx;
    int streamIndex;
    AVStream* stream;
    SwsContext* swsCtx;
    AVFrame* videoFrame;
    AVPacket* pkt;
    bool havePkt;
    int havePktRet;
    bool fileEof;
    bool haveFrame;
    int haveFrameRet;
    bool codecEof;

    // for hardware-accelerated decoding:
    AVHWDeviceType hwDeviceType; // may be AV_HWDEVICE_TYPE_NONE if no hw-accel is available
    AVPixelFormat hwPixelFormat;
    AVBufferRef* hwDeviceCtx;
    AVFrame* videoFrameFromHW;

    FFmpeg() :
        formatCtx(nullptr),
        codecCtx(nullptr),
        streamIndex(-1),
        stream(nullptr),
        swsCtx(nullptr),
        videoFrame(nullptr),
        pkt(nullptr),
        havePkt(false),
        havePktRet(0),
        fileEof(false),
        haveFrame(false),
        haveFrameRet(0),
        codecEof(false),
        hwDeviceType(AV_HWDEVICE_TYPE_NONE),
        hwPixelFormat(AV_PIX_FMT_NONE),
        hwDeviceCtx(nullptr),
        videoFrameFromHW(nullptr)
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

// FFmpeg callback for hw-accel
static enum AVPixelFormat getHwFormat(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
    FFmpeg* ffmpeg = reinterpret_cast<FFmpeg*>(ctx->opaque);
    AVPixelFormat ret = AV_PIX_FMT_NONE;
    for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        //const AVPixFmtDescriptor* pixFmtDescP = av_pix_fmt_desc_get(*p);
        //const AVPixFmtDescriptor* pixFmtDescHW = av_pix_fmt_desc_get(ffmpeg->hwPixelFormat);
        //fprintf(stderr, "pix fmt %d (%s) vs %d (%s)\n", *p, pixFmtDescP->name, ffmpeg->hwPixelFormat, pixFmtDescHW->name);
        if (*p == ffmpeg->hwPixelFormat) {
            ret = *p;
            break;
        }
    }
    if (ret == AV_PIX_FMT_NONE) {
        // signal that negotiation failed - this may happen late (when decoding the first frame)
        ffmpeg->hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    }
    return ret;
}

Error FormatImportExportFFMPEG::openForReading(const std::string& fileName, const TagList& hints)
{
    if (fileName == "-")
        return ErrorInvalidData;

    _fileName = fileName;
    _hints = hints;
    int enableHWAccel = _hints.value("HWACCEL", 1);
    std::string logLevel = _hints.value("LOGLEVEL", "error");
    if (logLevel == "quiet")
        av_log_set_level(AV_LOG_QUIET);
    else if (logLevel == "panic")
        av_log_set_level(AV_LOG_PANIC);
    else if (logLevel == "fatal")
        av_log_set_level(AV_LOG_FATAL);
    else if (logLevel == "error")
        av_log_set_level(AV_LOG_ERROR);
    else if (logLevel == "warning")
        av_log_set_level(AV_LOG_WARNING);
    else if (logLevel == "info")
        av_log_set_level(AV_LOG_INFO);
    else if (logLevel == "verbose")
        av_log_set_level(AV_LOG_VERBOSE);
    else if (logLevel == "debug")
        av_log_set_level(AV_LOG_DEBUG);
    else if (logLevel == "trace")
        av_log_set_level(AV_LOG_TRACE);
    else
        av_log_set_level(AV_LOG_ERROR);

    if (avformat_open_input(&(_ffmpeg->formatCtx), _fileName.c_str(), nullptr, nullptr) < 0
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
    _ffmpeg->hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    if (enableHWAccel) {
        enum AVHWDeviceType deviceTypePreferenceList[6] = {
            AV_HWDEVICE_TYPE_VAAPI,         // Linux standard
            AV_HWDEVICE_TYPE_VDPAU,         // Linux NVIDIA
            AV_HWDEVICE_TYPE_MEDIACODEC,    // Android
            AV_HWDEVICE_TYPE_VIDEOTOOLBOX,  // Mac
            AV_HWDEVICE_TYPE_DXVA2,         // W32 D3D9
            AV_HWDEVICE_TYPE_D3D11VA        // W32 D3D11
        };
        for (int typeIndex = 0; typeIndex < 6; typeIndex++) {
            for (int i = 0; ; i++) {
                const AVCodecHWConfig* config = avcodec_get_hw_config(dec, i);
                if (!config)
                    break;
                if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX
                        && config->device_type == deviceTypePreferenceList[typeIndex]) {
                    _ffmpeg->hwDeviceType = deviceTypePreferenceList[typeIndex];
                    _ffmpeg->hwPixelFormat = config->pix_fmt;
                    break;
                }
            }
            if (_ffmpeg->hwDeviceType != AV_HWDEVICE_TYPE_NONE)
                break;
        }
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
    if (_ffmpeg->hwDeviceType != AV_HWDEVICE_TYPE_NONE) {
        _ffmpeg->codecCtx->opaque = _ffmpeg;
        _ffmpeg->codecCtx->get_format = getHwFormat;
        if (av_hwdevice_ctx_create(&(_ffmpeg->hwDeviceCtx), _ffmpeg->hwDeviceType, nullptr, nullptr, 0) < 0) {
            _ffmpeg->hwDeviceType = AV_HWDEVICE_TYPE_NONE;
            _ffmpeg->codecCtx->opaque = nullptr;
            _ffmpeg->codecCtx->get_format = nullptr;
            /* The above is actually not enough to clean up everything that is
             * related to hardware acceleration. Something happened to codecCtx
             * that causes a null pointer dereference if we continue. So we just
             * retry from the start but without hardware acceleration. */
            close();
            _hints.set("HWACCEL", "0");
            return openForReading(_fileName, _hints);
        } else {
            _ffmpeg->codecCtx->hw_device_ctx = av_buffer_ref(_ffmpeg->hwDeviceCtx);
        }
    }
    _ffmpeg->codecCtx->thread_count = std::min(av_cpu_count(), 16); // enable multi-threaded decoding
    if (avcodec_open2(_ffmpeg->codecCtx, dec, nullptr) < 0) {
        close();
        return ErrorLibrary;
    }

    _ffmpeg->videoFrame = av_frame_alloc();
    _ffmpeg->videoFrameFromHW = av_frame_alloc();
    _ffmpeg->pkt = av_packet_alloc();
    if (!_ffmpeg->videoFrame || !_ffmpeg->videoFrameFromHW || !_ffmpeg->pkt) {
        close();
        errno = ENOMEM;
        return ErrorSysErrno;
    }

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
    if (_ffmpeg->videoFrameFromHW) {
        av_frame_free(&(_ffmpeg->videoFrameFromHW));
        _ffmpeg->videoFrameFromHW = nullptr;
    }
    if (_ffmpeg->havePkt) {
        av_packet_unref(_ffmpeg->pkt);
    }
    if (_ffmpeg->pkt) {
        av_packet_free(&(_ffmpeg->pkt));
        _ffmpeg->pkt = nullptr;
    }
    _ffmpeg->havePkt = false;
    _ffmpeg->havePktRet = 0;
    _ffmpeg->fileEof = false;
    _ffmpeg->haveFrame = false;
    _ffmpeg->haveFrameRet = 0;
    _ffmpeg->codecEof = false;
    _ffmpeg->hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    _ffmpeg->hwPixelFormat = AV_PIX_FMT_NONE;
    if (_ffmpeg->hwDeviceCtx) {
        av_buffer_unref(&(_ffmpeg->hwDeviceCtx));
        _ffmpeg->hwDeviceCtx = nullptr;
    }
    _desc = ArrayDescription();
    _minDTS = 0; // bad guess, but INT64_MIN seems to cause problems when seeking to it
    _unreliableTimeStamps = false;
    _frameDTSs.clear();
    _framePTSs.clear();
    _keyFrames.clear();
    _indexOfLastReadFrame = -1;
}

bool FormatImportExportFFMPEG::hardReset(bool disableHWAccel = false)
{
    // Hard reset by closing and reopening, because seeking to minDTS is not reliable
    // or because hardware acceleration failed
    int64_t bakMinDTS = _minDTS;
    bool bakUnreliableTimeStamps = _unreliableTimeStamps;
    std::vector<int64_t> bakFrameDTSs = _frameDTSs;
    std::vector<int64_t> bakFramePTSs = _framePTSs;
    std::vector<int> bakKeyFrames = _keyFrames;
    close();
    if (disableHWAccel)
        _hints.set("HWACCEL", "0");
    if (openForReading(_fileName, _hints) != ErrorNone)
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
    /* Lesson learned: we cannot know the exact number of frames unless we decode the whole
     * stream.
     *
     * 1. According to the documentation, nb_frames should only be > 0 if the number of frames
     * is known, but often it is only a guess, e.g. based on the number of packets. This is not
     * good enough.
     *
     * 2. Counting the packets in the stream does not help because there is not always a 1:1
     * correspondence between packets and frames.
     *
     * 3. Decoding the whole stream is obviously far too slow.
     */
    return -1;
}

ArrayContainer FormatImportExportFFMPEG::readArray(Error* error, int arrayIndex, const Allocator& alloc)
{
    bool seeked = false;
    if (arrayIndex >= 0) {
        // only perform proper seeking if we have reliable time stamps
        if (!_unreliableTimeStamps) {
            // find the key frame that has arrayIndex or precedes arrayIndex
            int precedingKeyFrameIndex = -1;
            for (auto it = _keyFrames.cbegin(); it != _keyFrames.cend(); ++it) {
                if (*it <= arrayIndex && *it > precedingKeyFrameIndex)
                    precedingKeyFrameIndex = *it;
            }
            // check if we should seek or just skip frames until we're there
            if (arrayIndex == _indexOfLastReadFrame + 1) {
                // we read the next frame, no need to seek or skip
            } else if (arrayIndex > _indexOfLastReadFrame && precedingKeyFrameIndex <= _indexOfLastReadFrame) {
                // it is cheaper to skip frames until we arrive at the frame we want
                // so do nothing here
            } else if (size_t(arrayIndex) >= _frameDTSs.size()) {
                // we did not record the timestamps of at least one frame on the way to the
                // requested frame, so read and skip frames until we arrive at the frame
                // we want
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
                // get rid of packet read by hasMore(), if any
                if (_ffmpeg->havePkt) {
                    av_packet_unref(_ffmpeg->pkt);
                    _ffmpeg->havePkt = false;
                }
                // flush the decoder buffers
                avcodec_flush_buffers(_ffmpeg->codecCtx);
                _ffmpeg->haveFrame = false;
                // reset EOF flags
                _ffmpeg->fileEof = false;
                _ffmpeg->codecEof = false;
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
        if (_ffmpeg->haveFrame) {
            /* We already have a frame from hasMore(). Consume it now. */
            ret = _ffmpeg->haveFrameRet;
            _ffmpeg->haveFrame = false;
        } else {
            ret = avcodec_receive_frame(_ffmpeg->codecCtx, _ffmpeg->videoFrame);
        }
        if (ret == AVERROR_EOF && _ffmpeg->fileEof) {
            _ffmpeg->codecEof = true;
            break;
        } else if (ret == AVERROR(EAGAIN)) {
            for (;;) {
                if (_ffmpeg->havePkt) {
                    /* We already have a packet from hasMore(). Consume it now. */
                    ret = _ffmpeg->havePktRet;
                    _ffmpeg->havePkt = false;
                } else {
                    ret = av_read_frame(_ffmpeg->formatCtx, _ffmpeg->pkt);
                }
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
                    bool haveHWAccel = (_ffmpeg->hwDeviceType != AV_HWDEVICE_TYPE_NONE);
                    if (avcodec_send_packet(_ffmpeg->codecCtx, _ffmpeg->pkt) < 0) {
                        close();
                        *error = ErrorInvalidData;
                        return ArrayContainer();
                    }
                    av_packet_unref(_ffmpeg->pkt);
                    if (haveHWAccel && (_ffmpeg->hwDeviceType == AV_HWDEVICE_TYPE_NONE)) {
                        /* hardware acceleration failed late, signalled by getHwFormat()
                         * because FFmpeg won't let us know otherwise */
                        if (hardReset(true)) {
                            return readArray(error, arrayIndex, alloc);
                        } else {
                            close();
                            *error = ErrorInvalidData;
                            return ArrayContainer();
                        }
                    }
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
            /* Record the timestamps of this frame if it is the next one that needs to be recorded. */
            if (!seeked && _frameDTSs.size() == size_t(_indexOfLastReadFrame + 1)) {
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
                if (_ffmpeg->videoFrame->flags & AV_FRAME_FLAG_KEY) {
                    _keyFrames.push_back(_frameDTSs.size() - 1);
                }
            }
            /* Find out if we read the requested frame or if we need to skip until we
             * arrive at the requested frame. */
            if (arrayIndex < 0) {
                // We are ok once we read one frame
                _indexOfLastReadFrame++;
                break;
            } else if (!seeked) {
                int frameIndex = _indexOfLastReadFrame + 1;
                _indexOfLastReadFrame = frameIndex;
                // We are ok once we read the frame with the index we seek;
                // otherwise we read more frames until we arrive there
                if (frameIndex == arrayIndex)
                    break;
            } else {
                // Get frame index from its DTS and PTS pair (the DTS is not necessarily unique)
                int frameIndex = -1;
                int64_t dts = _ffmpeg->videoFrame->pkt_dts;
                int64_t pts = _ffmpeg->videoFrame->pts;
                for (size_t i = 0; i < _frameDTSs.size(); i++) {
                    if (_frameDTSs[i] == dts && _framePTSs[i] == pts) {
                        frameIndex = i;
                        break;
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

    /* Make videoFramePtr point to the video frame in main memory */
    AVFrame* videoFramePtr;
    if (_ffmpeg->hwDeviceType == AV_HWDEVICE_TYPE_NONE) {
        videoFramePtr = _ffmpeg->videoFrame;
    } else {
        /* transfer data to main memory */
        if (av_hwframe_transfer_data(_ffmpeg->videoFrameFromHW, _ffmpeg->videoFrame, 0) < 0) {
            close();
            *error = ErrorLibrary;
            return ArrayContainer();
        }
        videoFramePtr = _ffmpeg->videoFrameFromHW;
    }

    /* Lazily initialize _desc */
    int w = _ffmpeg->codecCtx->width;
    int h = _ffmpeg->codecCtx->height;
    const AVPixFmtDescriptor* pixFmtDesc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(videoFramePtr->format));
    if (w < 1 || h < 1 || !pixFmtDesc || pixFmtDesc->nb_components < 1 || pixFmtDesc->nb_components > 4) {
        close();
        *error = ErrorInvalidData;
        return ArrayContainer();
    }
    int componentCount = pixFmtDesc->nb_components;
    Type type = uint8;
    for (int i = 0; i < componentCount; i++) {
        if (pixFmtDesc->comp[i].depth > 8) {
            type = uint16;
            break;
        }
    }
    if (_desc.dimensionCount() != 2
            || _desc.dimension(0) != size_t(w)
            || _desc.dimension(1) != size_t(h)
            || _desc.componentCount() != size_t(componentCount)
            || _desc.componentType() != type) {
        _desc = ArrayDescription({ size_t(w), size_t(h) }, componentCount, type);
        if (componentCount <= 2) {
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/GRAY");
            if (componentCount == 2)
                _desc.componentTagList(1).set("INTERPRETATION", "ALPHA");
        } else {
            _desc.componentTagList(0).set("INTERPRETATION", "SRGB/R");
            _desc.componentTagList(1).set("INTERPRETATION", "SRGB/G");
            _desc.componentTagList(2).set("INTERPRETATION", "SRGB/B");
            if (componentCount == 4)
                _desc.componentTagList(3).set("INTERPRETATION", "ALPHA");
        }
    }

    /* Lazily initialize swsCtx. FFmpeg takes care of reusing an existing context if possible. */
    AVPixelFormat dstPixFmt;
    if (type == uint8) {
        if (componentCount == 1)
            dstPixFmt = AV_PIX_FMT_GRAY8;
        else if (componentCount == 2)
            dstPixFmt = AV_PIX_FMT_YA8;
        else if (componentCount == 3)
            dstPixFmt = AV_PIX_FMT_RGB24;
        else
            dstPixFmt = AV_PIX_FMT_RGBA;
    } else {
        if (componentCount == 1)
            dstPixFmt = AV_PIX_FMT_GRAY16;
        else if (componentCount == 2)
            dstPixFmt = AV_PIX_FMT_YA16;
        else if (componentCount == 3)
            dstPixFmt = AV_PIX_FMT_RGB48;
        else
            dstPixFmt = AV_PIX_FMT_RGBA64;
    }
    _ffmpeg->swsCtx = sws_getCachedContext(_ffmpeg->swsCtx,
            w, h, static_cast<AVPixelFormat>(videoFramePtr->format),
            w, h, dstPixFmt,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!_ffmpeg->swsCtx) {
        close();
        *error = ErrorLibrary;
        return ArrayContainer();
    }

    ArrayContainer r(_desc, alloc);
    uint8_t* dst[4] = { static_cast<uint8_t*>(r.data()), nullptr, nullptr, nullptr };
    int dstStride[4] = { int(r.dimension(0) * r.elementSize()), 0, 0, 0 };
    sws_scale(_ffmpeg->swsCtx, videoFramePtr->data, videoFramePtr->linesize, 0, videoFramePtr->height, dst, dstStride);
    reverseY(r);

    return r;
}

bool FormatImportExportFFMPEG::hasMore()
{
    /* We have no knowledge about the number of frames in this video, see also the
     * comments in arrayCount().
     * So we try to read a packet from the correct stream, and if that succeeds,
     * we know that we have more frames. Since we cannot "unread" the packet,
     * we need to consume it in readArray().
     * Similarly, in drain mode (when there are no more packets in the file but the
     * codec might still have frames buffered) we try to get the next frame, which
     * needs to be consumed in readArray(). */

    if (_ffmpeg->havePkt) {
        /* There's still a packet in the pipeline */
        return true;
    } else if (_ffmpeg->haveFrame) {
        /* There's still a frame in the pipeline */
        return true;
    } else {
        /* No packet and no frame in the pipeline */
        if (!_ffmpeg->fileEof) {
            /* Read another packet */
            bool ret;
            for (;;) {
                _ffmpeg->havePktRet = av_read_frame(_ffmpeg->formatCtx, _ffmpeg->pkt);
                if (_ffmpeg->havePktRet == AVERROR_EOF) {
                    _ffmpeg->fileEof = true;
                    _ffmpeg->havePkt = true;
                    ret = true;
                    break;
                } else if (_ffmpeg->havePktRet < 0) {
                    ret = false;
                    break;
                } else if (_ffmpeg->pkt->stream_index == _ffmpeg->streamIndex) {
                    _ffmpeg->havePkt = true;
                    ret = true;
                    break;
                } else {
                    av_packet_unref(_ffmpeg->pkt);
                }
            }
            return ret;
        } else {
            /* No more packets to read */
            if (_ffmpeg->codecEof) {
                /* No more frames to be drained */
                return false;
            } else {
                /* Try to receive another frame */
                _ffmpeg->haveFrameRet = avcodec_receive_frame(_ffmpeg->codecCtx, _ffmpeg->videoFrame);
                if (_ffmpeg->haveFrameRet == AVERROR_EOF) {
                    /* Nothing more to drain */
                    _ffmpeg->codecEof = true;
                    return false;
                } else if (_ffmpeg->haveFrameRet < 0) {
                    /* Some decoding error - give up */
                    return false;
                } else {
                    /* We got a frame */
                    _ffmpeg->haveFrame = true;
                    return true;
                }
            }
        }
    }
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
