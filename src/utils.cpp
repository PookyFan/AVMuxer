#include <utility>

#include "include/utils.hpp"

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/error.h>
}

namespace AVMuxer
{
namespace
{
constexpr auto AV_ERR_MSG_SIZE = 128;
constexpr bool IS_DEBUG_MODE = 
                                #ifdef DEBUG_MODE
                                true;
                                #else
                                false;
                                #endif

std::unique_ptr<ILogger> logger;
}

void log(const std::string& msg, LogLevel level)
{
    if(level == LogLevel::DEBUG)
    {
        if constexpr(!IS_DEBUG_MODE)
            return;
    }

    if(logger)
        logger->logAVMuxerMessage(msg, level);
}

void setLogger(std::unique_ptr<ILogger>&& newLogger)
{
    logger = std::move(newLogger);
}

std::string getAvErrorString(int errNr)
{
    char errMsg[AV_ERR_MSG_SIZE];
    av_strerror(errNr, errMsg, AV_ERR_MSG_SIZE);
    return std::string(errMsg);
}

AVIOContext* makeIoContext(void* applicationData, IoProcedurePtr readProc, IoProcedurePtr writeProc)
{
    AVIOContext* ioCtxt;
    if(auto buffer = new PageAlignedBuffer();
       (ioCtxt = avio_alloc_context(*buffer, buffer->size(), (writeProc == nullptr ? 0 : 1), applicationData, readProc, writeProc, nullptr)) == nullptr)
    {
        log("Could not initialize I/O context - avio_alloc_context() failed", LogLevel::ERROR);
        delete buffer;
    }

    return ioCtxt;
}
}