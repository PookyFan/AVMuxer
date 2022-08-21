#pragma once

extern "C"
{
    #include <libavformat/avio.h>
}

namespace AVMuxer
{
using IoProcedurePtr = int (void*, uint8_t*, int);

class AVIOContextWrapper
{
    public:
        AVIOContextWrapper(void* applicationData, IoProcedurePtr readProc, IoProcedurePtr writeProc);
        ~AVIOContextWrapper();

        operator AVIOContext*() const
        {
            return context;
        }

        AVIOContext* operator->()
        {
            return context;
        }

        void reset();
    
    private:
        AVIOContext* context;

        void initialize(void* applicationData, IoProcedurePtr readProc, IoProcedurePtr writeProc);
        void deinitialize();
};
}
