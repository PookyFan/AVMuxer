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
    : containerCtxt("mp4"), videoCtxt(containerCtxt.createStream(framerate)),
      audioCtxt(containerCtxt.createStream()),
      audioAheadOfVideoInCommonTimebase(0),
      timeAheadInCommonTimebaseLimit(0), isMuxedDataAvailable(false)
{
    log("Creating Mp4Muxer instance", LogLevel::DEBUG);
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
    return containerCtxt.getMuxedData();
}

inline int Mp4Muxer::muxAudioDataIntermediate(const ByteVector& inputData)
{
    return muxMediaData(inputData, *audioCtxt, +1);
}

inline int Mp4Muxer::muxVideoDataIntermediate(const ByteVector& inputData)
{
    return muxMediaData(inputData, *videoCtxt, -1);
}

int Mp4Muxer::muxMediaData(const ByteVector& inputData, MediaStreamContext& mediaCtxt, int timeUpdateSign)
{
    mediaCtxt.fillBuffer(inputData);
    if(!mediaCtxt)
        mediaCtxt.initializeFormat();
    
    if(!containerCtxt)
        return 0;
    
    if(timeAheadInCommonTimebaseLimit == 0)
        timeAheadInCommonTimebaseLimit = containerCtxt.getMaxInterleaveDelta() * TIME_AHEAD_LIMIT_RATIO.num / TIME_AHEAD_LIMIT_RATIO.den;
    
    auto checkLimit = (timeUpdateSign > 0
        ? [](int64_t timeAhead, int64_t limit) { return timeAhead < limit; }
        : [](int64_t timeAhead, int64_t limit) { return timeAhead > -limit; });
    
    int packetsMuxedCnt = 0;
    auto stream = mediaCtxt.getStream();
    for(auto packet = mediaCtxt.getNextFrame();
        isPacketValid(packet) && checkLimit(audioAheadOfVideoInCommonTimebase, timeAheadInCommonTimebaseLimit);
        packet = mediaCtxt.getNextFrame(), ++packetsMuxedCnt)
    {
        audioAheadOfVideoInCommonTimebase += timeUpdateSign * av_rescale_q(packet.duration, stream->time_base, AV_TIME_BASE_Q);
        isMuxedDataAvailable |= containerCtxt.muxFramePacket(std::move(packet));
    }
    return packetsMuxedCnt;
}
}
