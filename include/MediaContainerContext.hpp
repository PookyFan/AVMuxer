#pragma once

#include <memory>

#include "AVIOContextWrapper.hpp"
#include "MediaStreamContext.hpp"

namespace AVMuxer
{
using MediaStreamContextSharedPtr = std::shared_ptr<MediaStreamContext>;
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

        MediaStreamContextSharedPtr createStream();
        MediaStreamContextSharedPtr createStream(AVRational framerate);

        bool       muxFramePacket(AVPacket&& packet);
        int64_t    getMaxInterleaveDelta() const;
        ByteVector getMuxedData();
        
    private:
        ByteVector muxedMediaData;
        std::vector<MediaStreamContextSharedPtr> streamCtxts;
        AVFormatContext* formatCtxt;
        AVIOContextWrapper ioCtxt;

        bool writeHeaderIfNeeded();
};
}
