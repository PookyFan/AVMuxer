#include <algorithm>
#include <stdexcept>

#include "MediaStreamContext.hpp"
#include "MuxerException.hpp"
#include "utils.hpp"

namespace AVMuxer
{
namespace
{
class EofException : public ::std::exception {};
}

int ioRead(void *opaque, uint8_t *buf, int bufsize)
{
    auto ctxt = reinterpret_cast<AVMuxer::MediaStreamContext*>(opaque);
    int sizeAvailable = ctxt->mediaDataBuffer.size() - ctxt->posInBuffer;
    if(sizeAvailable <= 0)
        throw EofException();
    
    auto readSize = std::min(sizeAvailable, bufsize);
    std::copy_n(ctxt->mediaDataBuffer.data() + ctxt->posInBuffer, readSize, buf);
    ctxt->posInBuffer += readSize;
    return readSize;
}

MediaStreamContext::MediaStreamContext()
    : formatCtxt(nullptr), stream(nullptr),
      ioCtxt(nullptr), posInBuffer(0), packetsCount(0)
{
}

MediaStreamContext::~MediaStreamContext()
{
    cleanUpComponents();
}

void MediaStreamContext::reset()
{
    cleanUpComponents();
    mediaDataBuffer.clear();
    stream = nullptr;
}

void MediaStreamContext::fillBuffer(const ByteVector& data)
{
    if(data.empty())
        return;
    
    if(auto currentSize = mediaDataBuffer.size(); posInBuffer > 0 && (data.size() + currentSize) > mediaDataBuffer.capacity())
    {
        std::move(mediaDataBuffer.begin() + posInBuffer, mediaDataBuffer.end(), mediaDataBuffer.begin());
        mediaDataBuffer.resize(currentSize - posInBuffer);
        posInBuffer = 0;
    }

    mediaDataBuffer.insert(mediaDataBuffer.end(), data.begin(), data.end());
}

AVPacket MediaStreamContext::getNextFrame()
{
    if(!*this && !initializeFormat())
        return {};
    
    AVPacket packet = { .data = nullptr, .size = 0 };
    av_init_packet(&packet);
    try { av_read_frame(formatCtxt, &packet); }
    catch(const EofException& e)
    {
        return invalidatePacket(packet);
    }
    
    packet.stream_index = stream->index;
    if(packet.pts == AV_NOPTS_VALUE)
    {
        auto inputTimeBase = (isTimeBaseValid(stream->r_frame_rate) ? stream->r_frame_rate : formatCtxt->streams[0]->time_base);
        auto duration = av_rescale_q(1, inputTimeBase, stream->time_base);
        packet.duration = duration;
        packet.pts = packet.dts = duration * packetsCount;
    }
    else
    {
        packet.pts      = av_rescale_q(packet.pts,      formatCtxt->streams[0]->time_base, stream->time_base);
        packet.dts      = av_rescale_q(packet.dts,      formatCtxt->streams[0]->time_base, stream->time_base);
        packet.duration = av_rescale_q(packet.duration, formatCtxt->streams[0]->time_base, stream->time_base);
    }

    ++packetsCount;
    return packet;
}

bool MediaStreamContext::initializeFormat()
{
    auto cleanAndReportFailure = [this](const std::string& errMsg, bool asWarning = false)
    {
        cleanUpComponents();
        log(errMsg, (asWarning ? LogLevel::WARNING : LogLevel::ERROR));
        return false;
    };

    try
    {
        if((ioCtxt = makeIoContext(this, ioRead, nullptr)) == nullptr)
        {
            log("MediaStreamContext::initializeFormat() - avio_alloc_context() failed", LogLevel::ERROR);
            return false;
        }
        
        if(formatCtxt = avformat_alloc_context();formatCtxt == nullptr)
            return cleanAndReportFailure("MediaStreamContext::initializeFormat() - avformat_alloc_context() failed");
        
        ioCtxt->seekable = 0;
        formatCtxt->pb = ioCtxt;
        if(auto result = avformat_open_input(&formatCtxt, nullptr, nullptr, nullptr); result < 0)
            return cleanAndReportFailure("MediaStreamContext::initializeFormat() - avformat_open_input() failed with error: " + getAvErrorString(result));
        
        log("MediaStreamContext::initializeFormat() - detected input streams: " + std::to_string(formatCtxt->nb_streams),
            LogLevel::DEBUG);
        if(formatCtxt->nb_streams == 0)
            return cleanAndReportFailure("No input streams detected (available media data may not be sufficient)", true);
        
        if(auto result = avformat_find_stream_info(formatCtxt, nullptr) < 0)
            return cleanAndReportFailure("MediaStreamContext::initializeFormat() - avformat_find_stream_info() failed with error: " + getAvErrorString(result));
    }
    catch(const EofException& e)
    {
        posInBuffer = 0;
        return cleanAndReportFailure("MediaStreamContext::initializeFormat() - not enough data buffered to identify input stream", true);
    }
    catch(...)
    {
        posInBuffer = 0;
        return cleanAndReportFailure("MediaStreamContext::initializeFormat() - an unknown error occurred");
    }

    avcodec_parameters_copy(stream->codecpar, formatCtxt->streams[0]->codecpar);
    stream->time_base = (isTimeBaseValid(stream->r_frame_rate)
        ? stream->r_frame_rate
        : formatCtxt->streams[0]->time_base);
    formatCtxt->opaque = nullptr;
    log("MediaStreamContext::initializeFormat() - successfully identified input stream", LogLevel::INFO);
    return true;
}

void MediaStreamContext::cleanUpComponents()
{
    if(formatCtxt != nullptr)
        avformat_close_input(&formatCtxt);
    if(ioCtxt != nullptr)
    {
        freeIoContextBuffer(ioCtxt);
        avio_context_free(&ioCtxt);
    }

    posInBuffer = 0;
}

}
