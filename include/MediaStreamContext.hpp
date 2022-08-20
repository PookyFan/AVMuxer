#pragma once

#include <cstdint>
#include <vector>

extern "C"
{
    #include <libavformat/avformat.h>
}

namespace AVMuxer
{
using ByteVector = ::std::vector<uint8_t>;

//Aligned either to entire cache line (64 bit systems) or half the cache line (32 bit systems)
class alignas(8*sizeof(void*)) MediaStreamContext
{
    friend int ioRead(void *opaque, uint8_t *buf, int bufsize);
    
    public:
        MediaStreamContext();
        ~MediaStreamContext();

        void reset();
        void fillBuffer(const ByteVector& data);
        AVPacket getNextFrame();

        bool hasQueuedData()
        {
            return !mediaDataBuffer.empty();
        }

        auto getBufferedDataSize()
        {
            return mediaDataBuffer.size();
        }

        void setStream(AVStream* newStream)
        {
            stream = newStream;
        }

        auto getStream()
        {
            return stream;
        }

        operator bool()
        {
            return formatCtxt != nullptr;
        }

        bool initializeFormat();
    
    private:
        ByteVector       mediaDataBuffer;
        AVFormatContext* formatCtxt;
        AVStream*        stream;
        AVIOContext*     ioCtxt;
        int              posInBuffer;
        unsigned int     packetsCount;
        
        void cleanUpComponents();
};

static_assert(sizeof(MediaStreamContext) == 32 || sizeof(MediaStreamContext) == 64);
}
