#pragma once

#include <memory>

#include "AVIOContextWrapper.hpp"
#include "MediaStreamContext.hpp"

namespace AVMuxer
{
using MediaStreamSharedPtr = std::shared_ptr<MediaStreamContext>;

class MediaContainerContext
{
    friend int muxCallback(void*, uint8_t*, int);

    public:
        MediaContainerContext(const char* formatName);
        ~MediaContainerContext();
        
        operator bool()
        {
            return writeHeaderIfNeeded();
        }

        MediaStreamSharedPtr createStream(AVRational framerate);

        bool       muxFramePacket(AVPacket&& packet);
        ByteVector getMuxedData();

        AVFormatContext* getFormatContext() const
        {
            return formatCtxt;
        }
        
    private:
        ByteVector muxedMediaData;
        std::vector<MediaStreamSharedPtr> streamCtxts;
        AVFormatContext* formatCtxt;
        AVIOContextWrapper ioCtxt;

        bool writeHeaderIfNeeded();
};
}
