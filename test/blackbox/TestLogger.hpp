#pragma once

#include "utils.hpp"

class TestLogger : public AVMuxer::ILogger
{
    public:
        void logAVMuxerMessage(const ::std::string& msg, AVMuxer::LogLevel level) override
        {
            std::cout << msg << std::endl;
        }
};