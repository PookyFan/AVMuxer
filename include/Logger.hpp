#pragma once

#include <string>

namespace AVMuxer
{
    enum LogLevel { DEBUG, INFO, WARNING, ERROR };

    class ILogger
    {
        public:
            virtual void logAVMuxerMessage(const ::std::string& msg, AVMuxer::LogLevel level) = 0;
            virtual ~ILogger() = default;
    };
}
