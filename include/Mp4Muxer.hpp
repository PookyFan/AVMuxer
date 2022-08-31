#pragma once

#include <memory>

#include "MediaStreamWrapper.hpp"
#include "MediaContainerWrapper.hpp"

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
    
    protected:
        std::shared_ptr<MediaContainerWrapper> containerCtxt;
        WrappedMediaStreamSharedPtr  videoCtxt;
        WrappedMediaStreamSharedPtr  audioCtxt;

        int64_t audioAheadOfVideoInCommonTimebase;
        int64_t timeAheadInCommonTimebaseLimit;
        bool    isMuxedDataAvailable;

    private:    
        int muxAudioDataIntermediate(const ByteVector& inputData);
        int muxVideoDataIntermediate(const ByteVector& inputData);
        int muxMediaData(const ByteVector& inputData, MediaStreamWrapper& mediaCtxt, int timeUpdateSign);

};
}
