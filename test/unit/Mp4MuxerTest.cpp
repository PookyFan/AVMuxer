#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MediaContainerWrapper.hpp"
#include "Mp4Muxer.hpp"

using namespace testing;

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

        MOCK_METHOD(void, fillBuffer, (const ByteVector& data), (const, override));
        MOCK_METHOD(AVPacket, getNextFrame, (), (override));
        MOCK_METHOD(bool, hasQueuedData, (), (const, override));
        MOCK_METHOD(size_t, getBufferedDataSize, (), (const, override));
        MOCK_METHOD(AVRational, getTimeBase, (), (const, override));
        MOCK_METHOD(bool, boolOp, (), (const));
};

class MediaContainerMock : public MediaContainerWrapper
{
     public:
        MediaContainerMock(const char* formatName) : MediaContainerWrapper(formatName)
        {}

        MOCK_METHOD(WrappedMediaStreamSharedPtr, createStream, (), (override));
        MOCK_METHOD(WrappedMediaStreamSharedPtr, createStream, (AVRational framerate), (override));
        MOCK_METHOD(bool, muxFramePacket, (AVPacket&& packet), (override));
        MOCK_METHOD(int64_t, getMaxInterleaveDelta, (), (const, override));
        MOCK_METHOD(ByteVector, getMuxedData, (), (override));
};

namespace
{
constexpr auto FPS = 24;
std::shared_ptr<MediaContainerWrapper>  containerCtxtMock = std::make_shared<StrictMock<MediaContainerMock>>("mp4");
WrappedMediaStreamSharedPtr firstStreamCtxtMock  = std::make_shared<StrictMock<MediaStreamMock>>();
WrappedMediaStreamSharedPtr secondStreamCtxtMock = std::make_shared<StrictMock<MediaStreamMock>>();
}

class Mp4MuxerTest : public Mp4Muxer
{
    public:
        Mp4MuxerTest(AVRational framerate)
            : Mp4Muxer(framerate)
        {
            containerCtxt = containerCtxtMock;
            audioCtxt = firstStreamCtxtMock;
            videoCtxt = secondStreamCtxtMock;
        }
};

TEST(Mp4MuxerF, Mp4MuxerShouldNotAcceptZeroOrNegativeFramerate)
{
    ASSERT_THROW(AVMuxer::Test::Mp4MuxerTest({0, 0}), std::invalid_argument);
    ASSERT_THROW(AVMuxer::Test::Mp4MuxerTest({1, 0}), std::invalid_argument);
    ASSERT_THROW(AVMuxer::Test::Mp4MuxerTest({0, 1}), std::invalid_argument);
    ASSERT_THROW(AVMuxer::Test::Mp4MuxerTest({1, -1}), std::invalid_argument);
    ASSERT_THROW(AVMuxer::Test::Mp4MuxerTest({-1, 1}), std::invalid_argument);
    ASSERT_THROW(AVMuxer::Test::Mp4MuxerTest({-1, -1}), std::invalid_argument);
}

}