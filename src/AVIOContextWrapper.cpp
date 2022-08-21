#include <memory>

#include "AVIOContextWrapper.hpp"
#include "MuxerException.hpp"
#include "utils.hpp"

namespace AVMuxer
{
AVIOContextWrapper::AVIOContextWrapper(void* applicationData, IoProcedurePtr readProc, IoProcedurePtr writeProc)
{
    log("Creating AVIOContextWrapper instance", LogLevel::DEBUG);
    initialize(applicationData, readProc, writeProc);
}

AVIOContextWrapper::~AVIOContextWrapper()
{
    log("Deleting AVIOContextWrapper instance", LogLevel::DEBUG);
    deinitialize();
}

void AVIOContextWrapper::reset()
{
    auto appData = context->opaque;
    auto readProc = context->read_packet;
    auto writeProc = context->write_packet;
    deinitialize();
    initialize(appData, readProc, writeProc);
}

void AVIOContextWrapper::initialize(void* applicationData, IoProcedurePtr readProc, IoProcedurePtr writeProc)
{
    if(auto buffer = std::make_unique<PageAlignedBuffer>();
       (context = avio_alloc_context(*buffer, buffer->size(), (writeProc == nullptr ? 0 : 1), applicationData, readProc, writeProc, nullptr)) == nullptr)
    {
        throw MuxerException("Could not initialize I/O context - avio_alloc_context() failed");
    }
    else
        buffer.release();
}

void AVIOContextWrapper::deinitialize()
{
    if(context->buffer != nullptr)
        delete reinterpret_cast<PageAlignedBuffer*>(context->buffer);
    avio_context_free(&context);
}
}
