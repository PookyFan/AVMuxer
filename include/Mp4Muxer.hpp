#pragma once

#include "Muxer.hpp"

namespace AVMuxer
{
class AudioVideoMp4Muxer : public Muxer<2>
{
    public:
        AudioVideoMp4Muxer(AVRational framerate) : Muxer<2>("mp4", framerate)
        {}

        bool muxVideoData(const ByteVector& inputData)
        {
            return muxMediaData<0>(inputData);
        }

        bool muxAudioData(const ByteVector& inputData)
        {
            return muxMediaData<1>(inputData);
        }
};

class VideoOnlyMp4Muxer : public Muxer<1>
{
    public:
        VideoOnlyMp4Muxer(AVRational framerate) : Muxer<1>("mp4", framerate)
        {}

        bool muxVideoData(const ByteVector& inputData)
        {
            return muxMediaData<0>(inputData);
        }
};
}
