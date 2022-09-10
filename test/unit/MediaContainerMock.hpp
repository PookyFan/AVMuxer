#pragma once

#include <gmock/gmock.h>

namespace AVMuxer::Test
{
class MediaContainerMock : public MediaContainerWrapper
{
     public:
        MediaContainerMock(const char* formatName) : MediaContainerWrapper(formatName)
        {}

        operator bool() { return boolOp(); }

        MOCK_METHOD(WrappedMediaStreamSharedPtr, createStream, (), (override));
        MOCK_METHOD(WrappedMediaStreamSharedPtr, createStream, (AVRational framerate), (override));
        MOCK_METHOD(bool, muxFramePacket, (AVPacket&& packet), (override));
        MOCK_METHOD(int64_t, getMaxInterleaveDelta, (), (const, override));
        MOCK_METHOD(ByteVector, getMuxedData, (), (override));
        MOCK_METHOD(bool, boolOp, (), (const));
};
}
