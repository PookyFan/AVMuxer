#include <utility>

#include "Mp4Muxer.hpp"
#include "MuxerException.hpp"
#include "utils.hpp"

namespace AVMuxer
{
namespace
{
    constexpr AVRational TIME_AHEAD_LIMIT_RATIO = { .num = 8, .den = 10};
}

Mp4Muxer::Mp4Muxer(AVRational framerate)
    : containerCtxt(std::make_shared<MediaContainerWrapper>("mp4")),
      videoCtxt(containerCtxt->createStream(framerate)),
      audioCtxt(containerCtxt->createStream()),
      audioAheadOfVideoInCommonTimebase(0),
      timeAheadInCommonTimebaseLimit(0), isMuxedDataAvailable(false)
{
    log("Creating Mp4Muxer instance", LogLevel::DEBUG);
    if(framerate.num <= 0 || framerate.den <= 0)
        throw std::invalid_argument("Framerate can't be zero");
}

Mp4Muxer::~Mp4Muxer()
{
    log("Deleting Mp4Muxer instance", LogLevel::DEBUG);
}

bool Mp4Muxer::muxAudioData(const ByteVector& inputData)
{
    muxAudioDataIntermediate(inputData);
    return hasMuxedData();
}

bool Mp4Muxer::muxVideoData(const ByteVector& inputData)
{
    muxVideoDataIntermediate(inputData);
    return hasMuxedData();
}

bool Mp4Muxer::flush()
{
    ByteVector dummy;
    int muxedCnt;
    do
    {
        muxedCnt  = muxAudioDataIntermediate(dummy);
        muxedCnt += muxVideoDataIntermediate(dummy);
    } while(muxedCnt > 0);
    
    return hasMuxedData();
}

ByteVector Mp4Muxer::getMuxedData()
{
    isMuxedDataAvailable = false;
    return containerCtxt->getMuxedData();
}

inline int Mp4Muxer::muxAudioDataIntermediate(const ByteVector& inputData)
{
    return muxMediaData(inputData, *audioCtxt, +1);
}

inline int Mp4Muxer::muxVideoDataIntermediate(const ByteVector& inputData)
{
    return muxMediaData(inputData, *videoCtxt, -1);
}

int Mp4Muxer::muxMediaData(const ByteVector& inputData, MediaStreamWrapper& mediaCtxt, int timeUpdateSign)
{
    mediaCtxt.fillBuffer(inputData);
    if(!mediaCtxt || !*containerCtxt)
        return 0;
    
    if(timeAheadInCommonTimebaseLimit == 0)
        timeAheadInCommonTimebaseLimit = containerCtxt->getMaxInterleaveDelta() * TIME_AHEAD_LIMIT_RATIO.num / TIME_AHEAD_LIMIT_RATIO.den;
    
    auto checkLimit = (timeUpdateSign > 0
        ? [](int64_t timeAhead, int64_t limit) { return timeAhead < limit; }
        : [](int64_t timeAhead, int64_t limit) { return timeAhead > -limit; });
    
    if(!checkLimit(audioAheadOfVideoInCommonTimebase, timeAheadInCommonTimebaseLimit))
        return 0;
    
    int packetsMuxedCnt = 0;
    auto timebase = mediaCtxt.getTimeBase();
    for(auto packet = mediaCtxt.getNextFrame(); isPacketValid(packet); packet = mediaCtxt.getNextFrame())
    {
        audioAheadOfVideoInCommonTimebase += timeUpdateSign * av_rescale_q(packet.duration, timebase, AV_TIME_BASE_Q);
        isMuxedDataAvailable |= containerCtxt->muxFramePacket(std::move(packet));
        ++packetsMuxedCnt;
        if(!checkLimit(audioAheadOfVideoInCommonTimebase, timeAheadInCommonTimebaseLimit))
            break;
    }
    return packetsMuxedCnt;
}
}
