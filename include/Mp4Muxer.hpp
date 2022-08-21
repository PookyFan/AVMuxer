#pragma once

#include "MediaStreamContext.hpp"
#include "MediaContainerContext.hpp"

namespace AVMuxer
{
class Mp4Muxer
{
    public:
        Mp4Muxer(AVRational framerate);
        ~Mp4Muxer();

        bool hasMuxedData()
        {
            return isMuxedDataAvailable;
        }

        bool muxAudioData(const ByteVector& inputData);
        bool muxVideoData(const ByteVector& inputData);
        bool flush();
        ByteVector getMuxedData();
    
    private:
        MediaContainerContext       containerCtxt;
        MediaStreamContextSharedPtr videoCtxt;
        MediaStreamContextSharedPtr audioCtxt;
        int64_t                     audioAheadOfVideoInCommonTimebase;
        int64_t                     timeAheadInCommonTimebaseLimit;
        bool                        isMuxedDataAvailable;
        
        int muxAudioDataIntermediate(const ByteVector& inputData);
        int muxVideoDataIntermediate(const ByteVector& inputData);
        int muxMediaData(const ByteVector& inputData, MediaStreamContext& mediaCtxt, int timeUpdateSign);

};
}
