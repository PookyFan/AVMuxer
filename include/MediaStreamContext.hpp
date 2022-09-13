#pragma once

#include <cstdint>
#include <vector>

#include "AVIOContextWrapper.hpp"
#include "DataStructures.hpp"

extern "C"
{
    #include <libavformat/avformat.h>
}

namespace AVMuxer
{
//Aligned either to entire cache line (64 bit systems) or half the cache line (32 bit systems)
class alignas(8*sizeof(void*)) MediaStreamContext
{
    friend int ioRead(void *opaque, uint8_t *buf, int bufsize);
    
    public:
        MediaStreamContext(AVStream* newStream);
        MediaStreamContext(const MediaStreamContext&) = delete;
        MediaStreamContext(MediaStreamContext&&) = default;
        ~MediaStreamContext();

        void fillBuffer(const ByteArray& data) const;
        AVPacket getNextFrame();

        bool hasQueuedData() const
        {
            return !mediaDataBuffer.empty();
        }

        size_t getBufferedDataSize() const
        {
            return mediaDataBuffer.size();
        }

        AVStream* getStream() const
        {
            return stream;
        }

        operator bool() const
        {
            return formatCtxt != nullptr;
        }

        bool initializeFormat();
    
    private:
        mutable ByteVector mediaDataBuffer;
        AVFormatContext*   formatCtxt;
        AVStream*          stream;
        AVIOContextWrapper ioCtxt;
        mutable int        posInBuffer;
        unsigned int       packetsCount;

        void reset();
};

static_assert(sizeof(MediaStreamContext) == 32 || sizeof(MediaStreamContext) == 64);
}
