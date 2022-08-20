
#include "include/Mp4Muxer.hpp"
#include "include/MuxerException.hpp"
#include "include/utils.hpp"

namespace AVMuxer
{
int muxCallback(void* opaque, uint8_t* buf, int bufSize)
{
    auto muxer = static_cast<Mp4Muxer*>(opaque);
    ByteVector &outputData = muxer->muxedMediaData;
    outputData.insert(outputData.end(), buf, buf + bufSize);
    return bufSize;
}

Mp4Muxer::Mp4Muxer(AVRational framerate)
    : formatCtxt(nullptr), ioCtxt(nullptr), isHeaderWritten(false)
{
    if(auto buffer = new PageAlignedBuffer(); (ioCtxt = makeIoContext(this, nullptr, muxCallback)) == nullptr)
    {
        delete buffer;
        throw MuxerException("Couldn't initialize I/O context");
    }

    auto result = avformat_alloc_output_context2(&formatCtxt, nullptr, "mp4", nullptr);
    if(result < 0)
        throw MuxerException("Couldn't initialize format context; the error was: " + getAvErrorString(result));
    formatCtxt->pb = ioCtxt;

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
    muxMediaData(inputData, audioCtxt, videoCtxt.getStream()->time_base);
    return hasMuxedData();
}

bool Mp4Muxer::muxVideoData(const ByteVector& inputData)
{
    muxMediaData(inputData, videoCtxt, audioCtxt.getStream()->time_base);
    return hasMuxedData();
}

bool Mp4Muxer::flush()
{
    ByteVector dummy;
    muxAudioData(dummy);
    muxVideoData(dummy);
    return hasMuxedData();
}

ByteVector Mp4Muxer::getMuxedData()
{
    ByteVector output;
    output.swap(muxedMediaData);
    return output;
}

void Mp4Muxer::muxMediaData(const ByteVector& inputData, MediaStreamContext& mediaCtxt, const AVRational& otherStreamTimebase)
{
    mediaCtxt.fillBuffer(inputData);
    if(!mediaCtxt)
        mediaCtxt.initializeFormat();
    
    if(!isHeaderWritten && !writeHeader())
        return;
    
    for(auto packet = mediaCtxt.getNextFrame(); packet.size > 0; packet = mediaCtxt.getNextFrame())
    {
        if(auto result = av_interleaved_write_frame(formatCtxt, &packet); result < 0)
            throw MuxerException("Couldn't mux media data; the error was: " + getAvErrorString(result));
    }
}

bool Mp4Muxer::writeHeader()
{
    if(isHeaderWritten || !(audioCtxt && videoCtxt))
        return false;
    
    AVDictionary* options = nullptr;
    av_dict_set(&options, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
    auto result = avformat_write_header(formatCtxt, &options);
    if(result < 0)
        throw MuxerException("Couldn't write main header for MP4 container; the error was: " + getAvErrorString(result));
    
    return (isHeaderWritten = true);
}
}
