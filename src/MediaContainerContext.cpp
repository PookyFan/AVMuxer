#include <algorithm>

#include "MediaContainerContext.hpp"
#include "MuxerException.hpp"
#include "utils.hpp"

namespace AVMuxer
{
int muxCallback(void* opaque, uint8_t* buf, int bufSize)
{
    auto muxer = reinterpret_cast<MediaContainerContext*>(opaque);
    ByteVector &outputData = muxer->muxedMediaData;
    outputData.insert(outputData.end(), buf, buf + bufSize); //todo: some data allocation optimization would be cool
    return bufSize;
}

MediaContainerContext::MediaContainerContext(const char* formatName)
    : ioCtxt(this, nullptr, muxCallback)
{
    log("Creating MediaStreamContext instance", LogLevel::DEBUG);

    auto result = avformat_alloc_output_context2(&formatCtxt, nullptr, formatName, nullptr);
    if(result < 0)
        throw MuxerException("Couldn't initialize format context; the error was: " + getAvErrorString(result));
    formatCtxt->pb = ioCtxt;
    formatCtxt->opaque = nullptr;
    formatCtxt->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
}

MediaContainerContext::~MediaContainerContext()
{
    log("Deleting MediaStreamContext instance", LogLevel::DEBUG);
    if(formatCtxt != nullptr)
        avformat_free_context(formatCtxt);
}

MediaStreamSharedPtr MediaContainerContext::createStream(AVRational framerate)
{
    auto stream = avformat_new_stream(formatCtxt, nullptr);
    if(stream == nullptr)
    {
        log("Failed at creating media stream", LogLevel::ERROR);
        throw MuxerException("Couldn't initialize media stream");
    }

    stream->r_frame_rate = framerate;
    return streamCtxts.emplace_back(std::make_shared<MediaStreamContext>(stream));
}

bool MediaContainerContext::muxFramePacket(AVPacket&& packet)
{
    if(auto result = av_interleaved_write_frame(formatCtxt, &packet); result < 0)
        throw MuxerException("Couldn't mux media data; the error was: " + getAvErrorString(result));
    
    return !muxedMediaData.empty();
}

ByteVector MediaContainerContext::getMuxedData()
{
    ByteVector result;
    result.swap(muxedMediaData);
    return result;
}

bool MediaContainerContext::writeHeaderIfNeeded()
{
    bool* opaqueAsBool = reinterpret_cast<bool*>(&formatCtxt->opaque);
    bool &isHeaderWritten = *opaqueAsBool;
    if(isHeaderWritten || std::any_of(streamCtxts.begin(), streamCtxts.end(), [] (const auto& stream) { return !*stream; }))
        return isHeaderWritten;
    
    AVDictionary* options = nullptr;
    av_dict_set(&options, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
    auto result = avformat_write_header(formatCtxt, &options);
    if(result < 0)
        throw MuxerException("Couldn't write main header for MP4 container; the error was: " + getAvErrorString(result));
    
    return (isHeaderWritten = true);
}
}
