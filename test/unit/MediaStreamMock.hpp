#pragma once

#include <gmock/gmock.h>

namespace AVMuxer::Test
{
class MediaStreamMock : public MediaStreamWrapper
{
    friend class MediaContainerWrapper;
    public:
        MediaStreamMock()
            : MediaStreamWrapper({})
        {}

        operator bool() { return boolOp(); }

        MOCK_METHOD(void, fillBuffer, (const ByteArray& data), (const, override));
        MOCK_METHOD(AVPacket, getNextFrame, (), (override));
        MOCK_METHOD(bool, hasQueuedData, (), (const, override));
        MOCK_METHOD(size_t, getBufferedDataSize, (), (const, override));
        MOCK_METHOD(AVRational, getTimeBase, (), (const, override));
        MOCK_METHOD(bool, boolOp, (), (const));
};
}
