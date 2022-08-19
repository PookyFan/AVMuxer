#pragma once

#include <stdexcept>

namespace AVMuxer
{
class MuxerException : public ::std::runtime_error
{
    public:
        using ::std::runtime_error::runtime_error;
};
}