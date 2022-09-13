#pragma once

#include <cstdint>
#include <vector>

namespace AVMuxer
{
using ByteVector = ::std::vector<uint8_t>;

struct ByteArray
{
    ByteArray(const uint8_t* const ptr, size_t dataSize) : data(ptr), size(dataSize)
    {}

    const uint8_t* const begin() const
    {
        return data;
    }

    const uint8_t* const end() const
    {
        return data + size;
    }

    bool empty() const
    {
        return size == 0;
    }

    const uint8_t* const data;
    const size_t size;
};
}
