#pragma once

#include <utility>

#include "MediaContainerContext.hpp"
#include "MediaStreamWrapper.hpp"

namespace AVMuxer
{
using WrappedMediaStreamSharedPtr = std::shared_ptr<MediaStreamWrapper>;
class MediaContainerWrapper
{
    public:
        MediaContainerWrapper(const char* format) : containerCtxt(format)
        {
        }

        virtual ~MediaContainerWrapper() = default;

        virtual operator bool()
        {
            return containerCtxt;
        }

        virtual WrappedMediaStreamSharedPtr createStream()
        {
            return createStream({0, 0});
        }

        virtual WrappedMediaStreamSharedPtr createStream(AVRational framerate)
        {
            auto ptr = new MediaStreamWrapper(containerCtxt.createStream(framerate));
            return std::shared_ptr<MediaStreamWrapper>(ptr);
        }

        virtual bool muxFramePacket(AVPacket&& packet)
        {
            return containerCtxt.muxFramePacket(std::move(packet));
        }

        virtual int64_t getMaxInterleaveDelta() const
        {
            return containerCtxt.getFormatContext()->max_interleave_delta;
        }

        virtual ByteVector getMuxedData()
        {
            return containerCtxt.getMuxedData();
        }

    private:
        MediaContainerContext containerCtxt;
};
}
