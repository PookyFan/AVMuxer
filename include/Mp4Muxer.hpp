#pragma once

#include "include/MediaStreamContext.hpp"

extern "C"
{
    #include <libavformat/avformat.h>
}

namespace AVMuxer
{
class Mp4Muxer
{
    friend int muxCallback(void*, uint8_t*, int);

    public:
        Mp4Muxer(AVRational framerate);
        ~Mp4Muxer();

        bool hasMuxedData()
        {
            return !muxedMediaData.empty();
        }

        bool muxAudioData(const ByteVector& inputData);
        bool muxVideoData(const ByteVector& inputData);
        bool flush();
        ByteVector getMuxedData();
    
    private:
        MediaStreamContext audioCtxt;
        MediaStreamContext videoCtxt;
        ByteVector muxedMediaData;
        AVFormatContext* formatCtxt;
        AVIOContext* ioCtxt;
        int64_t audioAheadOfVideoInCommonTimebase;
        int64_t timeAheadInCommonTimebaseLimit;
        bool isHeaderWritten;
        
        int muxAudioDataIntermediate(const ByteVector& inputData);
        int muxVideoDataIntermediate(const ByteVector& inputData);
        int muxMediaData(const ByteVector& inputData, MediaStreamContext& mediaCtxt, const AVRational& otherStreamTimebase, int timeUpdateSign);
        bool writeHeader();
};
}
