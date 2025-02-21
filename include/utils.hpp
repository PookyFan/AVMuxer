#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Logger.hpp"

class AVIOContext;

namespace AVMuxer
{
template<int Size>
struct alignas(Size) AlignedBuffer
{
    uint8_t buf[Size];
    constexpr auto size() { return Size; }
    operator uint8_t*() { return buf; }
};

constexpr auto PAGE_SIZE = 4096;

using PageAlignedBuffer = AlignedBuffer<PAGE_SIZE>;
using IoProcedurePtr = int (void*, uint8_t*, int);

void log(const std::string& msg, LogLevel level);

void setLogger(std::unique_ptr<ILogger>&& newLogger);

std::string getAvErrorString(int errNr);

AVIOContext* makeIoContext(void* applicationData, IoProcedurePtr readProc, IoProcedurePtr writeProc);

template <class Packet>
bool isPacketValid(const Packet& p)
{
    return p.size > 0;
}

template <class Packet>
Packet& invalidatePacket(Packet& p)
{
    if(isPacketValid(p))
        av_packet_unref(&p);
    return p;
}

template <class TimeBase>
bool isTimeBaseValid(TimeBase t)
{
    return t.den > 0;
}
}
