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

        operator bool() { return boolOp(); }

        MOCK_METHOD(WrappedMediaStreamSharedPtr, createStream, (), (override));
        MOCK_METHOD(WrappedMediaStreamSharedPtr, createStream, (AVRational framerate), (override));
        MOCK_METHOD(bool, muxFramePacket, (AVPacket&& packet), (override));
        MOCK_METHOD(int64_t, getMaxInterleaveDelta, (), (const, override));
        MOCK_METHOD(ByteVector, getMuxedData, (), (override));
        MOCK_METHOD(bool, boolOp, (), (const));
};

class Mp4MuxerTest : public Mp4Muxer
{
    public:
        Mp4MuxerTest(AVRational framerate,
                     std::shared_ptr<MediaContainerWrapper> containerCtxtMock,
                     WrappedMediaStreamSharedPtr firstStreamCtxtMock,
                     WrappedMediaStreamSharedPtr secondStreamCtxtMock)
            : Mp4Muxer(framerate)
        {
            containerCtxt = containerCtxtMock;
            audioCtxt = firstStreamCtxtMock;
            videoCtxt = secondStreamCtxtMock;
        }
};

class Mp4MuxerF : public ::testing::Test
{
    protected:
        const int FPS = 24;
        const int64_t DURATION = 50 * FPS;
        const ByteVector inputData = {0, 1, 2, 3, 4, 5, 6, 7};
        const ByteVector outputData = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::shared_ptr<MediaContainerWrapper>  containerCtxtMock = std::make_shared<StrictMock<MediaContainerMock>>("mp4");
        WrappedMediaStreamSharedPtr firstStreamCtxtMock  = std::make_shared<StrictMock<MediaStreamMock>>();
        WrappedMediaStreamSharedPtr secondStreamCtxtMock = std::make_shared<StrictMock<MediaStreamMock>>();

        auto& onContainerCtxtMock()
        {
            return *reinterpret_cast<StrictMock<MediaContainerMock>*>(&*containerCtxtMock);
        }

        auto& onFirstStreamCtxtMock()
        {
            return *reinterpret_cast<StrictMock<MediaStreamMock>*>(&*firstStreamCtxtMock);
        }

        auto& onSecondStreamCtxtMock()
        {
            return *reinterpret_cast<StrictMock<MediaStreamMock>*>(&*secondStreamCtxtMock);
        }

        auto createMuxer(AVRational fps)
        {
            return Mp4MuxerTest(fps, containerCtxtMock, firstStreamCtxtMock, secondStreamCtxtMock);
        }

        auto createMuxer()
        {
            return createMuxer({1, FPS});
        }
};

TEST_F(Mp4MuxerF, Mp4MuxerShouldNotAcceptZeroOrNegativeFramerate)
{
    ASSERT_THROW(createMuxer({0, 0}), std::invalid_argument);
    ASSERT_THROW(createMuxer({1, 0}), std::invalid_argument);
    ASSERT_THROW(createMuxer({0, 1}), std::invalid_argument);
    ASSERT_THROW(createMuxer({1, -1}), std::invalid_argument);
    ASSERT_THROW(createMuxer({-1, 1}), std::invalid_argument);
    ASSERT_THROW(createMuxer({-1, -1}), std::invalid_argument);
}

TEST_F(Mp4MuxerF, Mp4MuxerShouldNotReturnMuxedDataWhileNoInputDataIsBeingProvided)
{
    EXPECT_CALL(onFirstStreamCtxtMock(), fillBuffer(_)).Times(AnyNumber());
    EXPECT_CALL(onSecondStreamCtxtMock(), fillBuffer(_)).Times(AnyNumber());

    EXPECT_CALL(onFirstStreamCtxtMock(), boolOp()).WillRepeatedly(Return(false));
    EXPECT_CALL(onSecondStreamCtxtMock(), boolOp()).WillRepeatedly(Return(false));
    
    EXPECT_CALL(onContainerCtxtMock(), getMuxedData()).WillRepeatedly(Return(ByteVector{}));

    auto muxer = createMuxer();
    auto data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());

    muxer.flush();
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}

TEST_F(Mp4MuxerF, Mp4MuxerShouldNotReturnMuxedDataWhileAvcodecInternalsAreStillInitializing)
{
    EXPECT_CALL(onFirstStreamCtxtMock(), fillBuffer(inputData)).Times(AnyNumber());
    EXPECT_CALL(onSecondStreamCtxtMock(), fillBuffer(inputData)).Times(AnyNumber());

    EXPECT_CALL(onFirstStreamCtxtMock(), fillBuffer(_)).Times(AnyNumber());
    EXPECT_CALL(onSecondStreamCtxtMock(), fillBuffer(_)).Times(AnyNumber());

    EXPECT_CALL(onFirstStreamCtxtMock(), boolOp()).WillRepeatedly(Return(false));
    EXPECT_CALL(onSecondStreamCtxtMock(), boolOp()).WillRepeatedly(Return(false));

    EXPECT_CALL(onContainerCtxtMock(), getMuxedData()).WillRepeatedly(Return(ByteVector{}));

    auto muxer = createMuxer();
    muxer.muxVideoData(inputData);
    muxer.muxAudioData(inputData);
    auto data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());

    muxer.flush();
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}

TEST_F(Mp4MuxerF, Mp4MuxerShouldReturnMuxedDataOnlyWhenItIsSuccessfullyMuxed)
{
    EXPECT_CALL(onFirstStreamCtxtMock(),  fillBuffer(inputData)).Times(AnyNumber());
    EXPECT_CALL(onSecondStreamCtxtMock(), fillBuffer(inputData)).Times(AnyNumber());

    EXPECT_CALL(onFirstStreamCtxtMock(),  boolOp()).WillRepeatedly(Return(true));
    EXPECT_CALL(onSecondStreamCtxtMock(), boolOp()).WillRepeatedly(Return(true));
    EXPECT_CALL(onContainerCtxtMock(),    boolOp()).WillRepeatedly(Return(true));

    EXPECT_CALL(onFirstStreamCtxtMock(), getNextFrame()).WillRepeatedly(Return(AVPacket {.size = 0}));
    EXPECT_CALL(onFirstStreamCtxtMock(), getNextFrame()).Times(2).WillRepeatedly(Return(AVPacket {.size = 1, .duration = 1})).RetiresOnSaturation();
    
    EXPECT_CALL(onSecondStreamCtxtMock(), getNextFrame()).WillRepeatedly(Return(AVPacket {.size = 0}));
    EXPECT_CALL(onSecondStreamCtxtMock(), getNextFrame()).WillOnce(Return(AVPacket {.size = 1, .duration = 1})).RetiresOnSaturation();
    EXPECT_CALL(onSecondStreamCtxtMock(), getNextFrame()).WillOnce(Return(AVPacket {.size = 0})).RetiresOnSaturation();
    EXPECT_CALL(onSecondStreamCtxtMock(), getNextFrame()).Times(2).WillRepeatedly(Return(AVPacket {.size = 1})).RetiresOnSaturation();
    
    EXPECT_CALL(onFirstStreamCtxtMock(),  getTimeBase()).WillRepeatedly(Return(AVRational {1, FPS}));
    EXPECT_CALL(onSecondStreamCtxtMock(), getTimeBase()).WillRepeatedly(Return(AVRational {1, FPS}));

    EXPECT_CALL(onContainerCtxtMock(), muxFramePacket(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(onContainerCtxtMock(), muxFramePacket(_)).Times(2).WillRepeatedly(Return(true)).RetiresOnSaturation();
    EXPECT_CALL(onContainerCtxtMock(), muxFramePacket(_)).Times(2).WillRepeatedly(Return(false)).RetiresOnSaturation();

    EXPECT_CALL(onContainerCtxtMock(), getMuxedData()).WillOnce(Return(ByteVector{})).WillOnce(Return(outputData)).WillOnce(Return(ByteVector{}));

    EXPECT_CALL(onContainerCtxtMock(), getMaxInterleaveDelta()).WillRepeatedly(Return(900000));

    auto muxer = createMuxer();
    ASSERT_FALSE(muxer.muxVideoData(inputData));
    auto data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());

    ASSERT_TRUE(muxer.muxAudioData(inputData));
    data = muxer.getMuxedData();
    ASSERT_EQ(data, outputData);

    ASSERT_FALSE(muxer.muxVideoData(inputData));
    ASSERT_FALSE(muxer.muxAudioData(inputData));
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}

TEST_F(Mp4MuxerF, Mp4MuxerShouldReturnMuxedDataWhileFlushingOnlyWhenDataIsQueuedForMuxingAndCanBeFlushed)
{
    EXPECT_CALL(onFirstStreamCtxtMock(),  fillBuffer(_)).Times(AnyNumber());
    EXPECT_CALL(onSecondStreamCtxtMock(), fillBuffer(_)).Times(AnyNumber());

    EXPECT_CALL(onFirstStreamCtxtMock(),  boolOp()).WillRepeatedly(Return(true));
    EXPECT_CALL(onSecondStreamCtxtMock(), boolOp()).WillRepeatedly(Return(true));
    EXPECT_CALL(onContainerCtxtMock(),    boolOp()).WillRepeatedly(Return(true));

    EXPECT_CALL(onFirstStreamCtxtMock(), getNextFrame()).WillRepeatedly(Return(AVPacket {.size = 0}));
    EXPECT_CALL(onFirstStreamCtxtMock(), getNextFrame()).Times(2).WillRepeatedly(Return(AVPacket {.size = 1, .duration = DURATION})).RetiresOnSaturation();

    EXPECT_CALL(onSecondStreamCtxtMock(), getNextFrame()).Times(4).WillRepeatedly(Return(AVPacket {.size = 1, .duration = DURATION})).RetiresOnSaturation();

    EXPECT_CALL(onFirstStreamCtxtMock(),  getTimeBase()).WillRepeatedly(Return(AVRational {1, FPS}));
    EXPECT_CALL(onSecondStreamCtxtMock(), getTimeBase()).WillRepeatedly(Return(AVRational {1, FPS}));

    EXPECT_CALL(onContainerCtxtMock(), muxFramePacket(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(onContainerCtxtMock(), getMuxedData()).WillOnce(Return(ByteVector{}));
    EXPECT_CALL(onContainerCtxtMock(), getMuxedData()).Times(2).WillRepeatedly(Return(outputData)).RetiresOnSaturation();

    EXPECT_CALL(onContainerCtxtMock(), getMaxInterleaveDelta()).WillRepeatedly(Return(100ULL*AV_TIME_BASE));

    auto muxer = createMuxer();
    muxer.muxVideoData(inputData);
    muxer.muxAudioData(inputData);
    auto data = muxer.getMuxedData();
    ASSERT_EQ(data, outputData);

    ASSERT_TRUE(muxer.flush());
    data = muxer.getMuxedData();
    ASSERT_EQ(data, outputData);

    ASSERT_FALSE(muxer.flush());
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}

}