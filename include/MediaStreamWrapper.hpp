#pragma once

#include <memory>

#include "MediaStreamContext.hpp"

namespace AVMuxer
{
class MediaStreamWrapper
{
    friend class MediaContainerWrapper;

    MediaStreamWrapper(const MediaStreamWrapper&) = delete;
    MediaStreamWrapper(MediaStreamWrapper&&) = delete;
    public:
        virtual ~MediaStreamWrapper() = default;
        
        virtual void fillBuffer(const ByteVector& data) const
        {
            return streamCtxt->fillBuffer(data);
        }

        virtual AVPacket getNextFrame()
        {
            return streamCtxt->getNextFrame();
        }

        virtual bool hasQueuedData() const
        {
            return streamCtxt->hasQueuedData();
        }

        virtual size_t getBufferedDataSize() const
        {
            return streamCtxt->getBufferedDataSize();
        }

        virtual AVRational getTimeBase() const
        {
            return streamCtxt->getStream()->time_base;
        }

        virtual operator bool()
        {
            return (streamCtxt || streamCtxt->initializeFormat());
        }

    protected:
        MediaStreamWrapper(std::shared_ptr<MediaStreamContext> ctxt) : streamCtxt(ctxt)
        {
        }

    private:
        std::shared_ptr<MediaStreamContext> streamCtxt;
};
}
