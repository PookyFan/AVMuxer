#pragma once

#include "DataStructures.hpp"
#include "MediaStreamWrapper.hpp"
#include "MediaContainerWrapper.hpp"

namespace AVMuxer
{
class BaseMuxer
{
    public:
        BaseMuxer(const char* formatName);
        BaseMuxer(const BaseMuxer&) = delete;
        BaseMuxer(BaseMuxer&&) = delete;
        virtual ~BaseMuxer() = default;

        ByteVector getMuxedData();

        bool hasMuxedData()
        {
            return isMuxedDataAvailable;
        }
    
    protected:
        virtual void updateStreamRelativeTimeAhead(MediaStreamWrapper& mediaCtxt, int64_t diff) = 0;
        virtual bool shouldStreamBeLimited(MediaStreamWrapper& mediaCtxt) = 0;
        int muxMediaData(MediaStreamWrapper& mediaCtxt, const ByteArray& inputData);

        std::shared_ptr<MediaContainerWrapper> containerCtxt;
        int64_t timeAheadInCommonTimebaseLimit;
    
    private:
        bool isMuxedDataAvailable;
        bool isContainerInitialized;
};
}
