
#include "BaseMuxer.hpp"
#include "utils.hpp"

namespace AVMuxer
{
namespace
{
    constexpr AVRational TIME_AHEAD_LIMIT_RATIO = { .num = 8, .den = 10};
}

BaseMuxer::BaseMuxer(const char* formatName)
    : containerCtxt(std::make_shared<MediaContainerWrapper>(formatName)),
      timeAheadInCommonTimebaseLimit(0),
      isMuxedDataAvailable(false), isContainerInitialized(false)
{}

ByteVector BaseMuxer::getMuxedData()
{
    isMuxedDataAvailable = false;
    return containerCtxt->getMuxedData();
}

int BaseMuxer::muxMediaData(MediaStreamWrapper& mediaCtxt, const ByteVector& inputData)
{
    mediaCtxt.fillBuffer(inputData);
    if(!isContainerInitialized)
    {
        if(!(isContainerInitialized = mediaCtxt && *containerCtxt))
            return 0;
        
        timeAheadInCommonTimebaseLimit = containerCtxt->getMaxInterleaveDelta() * TIME_AHEAD_LIMIT_RATIO.num / TIME_AHEAD_LIMIT_RATIO.den;
    }

    if(shouldStreamBeLimited(mediaCtxt))
        return 0;
    
    int packetsMuxedCnt = 0;
    auto timebase = mediaCtxt.getTimeBase();
    for(auto packet = mediaCtxt.getNextFrame(); isPacketValid(packet); packet = mediaCtxt.getNextFrame())
    {
        auto diffInCommonTimebase = av_rescale_q(packet.duration, timebase, AV_TIME_BASE_Q);
        isMuxedDataAvailable |= containerCtxt->muxFramePacket(std::move(packet));
        ++packetsMuxedCnt;
        updateStreamRelativeTimeAhead(mediaCtxt, diffInCommonTimebase);
        if(shouldStreamBeLimited(mediaCtxt))
            break;
    }
    return packetsMuxedCnt;
}
}
