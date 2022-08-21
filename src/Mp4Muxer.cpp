#include "Mp4Muxer.hpp"
#include "MuxerException.hpp"
#include "utils.hpp"

namespace AVMuxer
{
namespace
{
    constexpr AVRational TIME_AHEAD_LIMIT_RATIO = { .num = 8, .den = 10};
}

int muxCallback(void* opaque, uint8_t* buf, int bufSize)
{
    auto muxer = reinterpret_cast<Mp4Muxer*>(opaque);
    ByteVector &outputData = muxer->muxedMediaData;
    outputData.insert(outputData.end(), buf, buf + bufSize);
    return bufSize;
}

Mp4Muxer::Mp4Muxer(AVRational framerate)
    : formatCtxt(nullptr), ioCtxt(nullptr),
      audioAheadOfVideoInCommonTimebase(0),
      timeAheadInCommonTimebaseLimit(0),
      isHeaderWritten(false)
{
    if((ioCtxt = makeIoContext(this, nullptr, muxCallback)) == nullptr)
        throw MuxerException("Couldn't initialize I/O context");

    auto result = avformat_alloc_output_context2(&formatCtxt, nullptr, "mp4", nullptr);
    if(result < 0)
        throw MuxerException("Couldn't initialize format context; the error was: " + getAvErrorString(result));
    formatCtxt->pb = ioCtxt;
    formatCtxt->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    auto videoStream = avformat_new_stream(formatCtxt, nullptr);
    if(videoStream == nullptr)
        throw MuxerException("Couldn't initialize video stream");
    videoStream->r_frame_rate = framerate;
    videoCtxt.setStream(videoStream);

    auto audioStream = avformat_new_stream(formatCtxt, nullptr);
    if(audioStream == nullptr)
        throw MuxerException("Couldn't initialize audio stream");
    audioCtxt.setStream(audioStream);
}

Mp4Muxer::~Mp4Muxer()
{
    log("Deleting Mp4Muxer instance", LogLevel::DEBUG);
    freeIoContextBuffer(ioCtxt);
    if(formatCtxt != nullptr)
        avformat_free_context(formatCtxt);
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
    ByteVector output;
    output.swap(muxedMediaData);
    return output;
}

inline int Mp4Muxer::muxAudioDataIntermediate(const ByteVector& inputData)
{
    return muxMediaData(inputData, audioCtxt, videoCtxt.getStream()->time_base, +1);
}

inline int Mp4Muxer::muxVideoDataIntermediate(const ByteVector& inputData)
{
    return muxMediaData(inputData, videoCtxt, audioCtxt.getStream()->time_base, -1);
}

int Mp4Muxer::muxMediaData(const ByteVector& inputData, MediaStreamContext& mediaCtxt, const AVRational& otherStreamTimebase, int timeUpdateSign)
{
    mediaCtxt.fillBuffer(inputData);
    if(!mediaCtxt)
        mediaCtxt.initializeFormat();
    
    if(!isHeaderWritten && !writeHeader())
        return 0;
    
    auto checkLimit = (timeUpdateSign > 0
        ? [](int64_t timeAhead, int64_t limit) { return timeAhead < limit; }
        : [](int64_t timeAhead, int64_t limit) { return timeAhead > -limit; });
    
    int packetsMuxedCnt = 0;
    auto stream = mediaCtxt.getStream();
    for(auto packet = mediaCtxt.getNextFrame();
        packet.size > 0 && checkLimit(audioAheadOfVideoInCommonTimebase, timeAheadInCommonTimebaseLimit);
        packet = mediaCtxt.getNextFrame(), ++packetsMuxedCnt)
    {
        audioAheadOfVideoInCommonTimebase += timeUpdateSign * av_rescale_q(packet.duration, stream->time_base, AV_TIME_BASE_Q);
        if(auto result = av_interleaved_write_frame(formatCtxt, &packet); result < 0)
            throw MuxerException("Couldn't mux media data; the error was: " + getAvErrorString(result));
    }

    return packetsMuxedCnt;
}

bool Mp4Muxer::writeHeader()
{
    if(isHeaderWritten || !(audioCtxt && videoCtxt))
        return false;
    
    AVDictionary* options = nullptr;
    av_dict_set(&options, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
    formatCtxt->max_interleave_delta = 10000000LL * 10LL; //FIXME: seems to help with glitchy audio, but there must be a better solution
    auto result = avformat_write_header(formatCtxt, &options);
    if(result < 0)
        throw MuxerException("Couldn't write main header for MP4 container; the error was: " + getAvErrorString(result));
    
    timeAheadInCommonTimebaseLimit = formatCtxt->max_interleave_delta * TIME_AHEAD_LIMIT_RATIO.num / TIME_AHEAD_LIMIT_RATIO.den;
    return (isHeaderWritten = true);
}
}
