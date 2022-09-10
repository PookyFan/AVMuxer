#include <array>
#include <gtest/gtest.h>
#include "Muxer.hpp"
#include "MediaStreamMock.hpp"
#include "MediaContainerMock.hpp"

using namespace testing;

namespace AVMuxer::Test
{
namespace
{
constexpr auto FPS                        = 24;
constexpr auto DURATION                   = 50 * FPS;
constexpr auto SOME_FRAMES_CNT_PER_STREAM = 2;
constexpr auto MORE_FRAMES_CNT_PER_STREAM = 4;
constexpr auto IS_FINAL                   = true; 
}

template <unsigned StreamsCount>
class MuxerTest : public Muxer<StreamsCount>
{
    using Base = Muxer<StreamsCount>;
    public:
        MuxerTest(AVRational framerate,
                  std::shared_ptr<MediaContainerWrapper> containerCtxtMock,
                  const std::array<WrappedMediaStreamSharedPtr, StreamsCount>& streamCtxtsMocks)
            : MuxerTest(std::array {framerate}, containerCtxtMock, streamCtxtsMocks)
        {}

        template <long unsigned VideoStreamsCount>
        MuxerTest(std::array<AVRational, VideoStreamsCount> framerates,
                  std::shared_ptr<MediaContainerWrapper> containerCtxtMock,
                  const std::array<WrappedMediaStreamSharedPtr, StreamsCount>& streamCtxtsMocks)
            : Base("mp4", framerates)
        {
            static_assert(VideoStreamsCount <= StreamsCount);
            this->containerCtxt = containerCtxtMock;

            auto currentMock = streamCtxtsMocks.begin();
            for(auto& currentStream : this->streams)
                currentStream = *(currentMock++);
        }
};

template <typename T>
class MuxerTestFixture : public Test
{
    public:
        static constexpr unsigned STREAMS_COUNT = T::STREAMS_COUNT;

        MuxerTestFixture() : containerCtxtMock(std::make_shared<StrictMock<MediaContainerMock>>("mp4"))
        {
            for(auto& mock : streamCtxtMocks)
                mock = std::make_shared<StrictMock<MediaStreamMock>>();
        }

    protected:
        auto& onContainerCtxtMock()
        {
            return *reinterpret_cast<StrictMock<MediaContainerMock>*>(&*containerCtxtMock);
        }

        template <unsigned StreamIndex>
        auto& onStreamCtxtMock()
        {
            static_assert(StreamIndex < STREAMS_COUNT);
            return *reinterpret_cast<StrictMock<MediaStreamMock>*>(&*streamCtxtMocks[StreamIndex]);
        }

        auto createMuxer(AVRational framerate)
        {
            return MuxerTest<STREAMS_COUNT>(framerate, containerCtxtMock, streamCtxtMocks);
        }

        template <unsigned Num>
        auto createMuxer(std::array<AVRational, Num> framerates)
        {
            return MuxerTest<STREAMS_COUNT>(framerates, containerCtxtMock, streamCtxtMocks);
        }

        auto createMuxerForFramerateTest(AVRational framerate)
        {
            constexpr auto frameratesCount = (STREAMS_COUNT + 1) / 2;
            std::array<AVRational, frameratesCount> framerates;
            for(auto& fps : framerates)
                fps = {1, 1};
            
            framerates.back() = framerate;
            return createMuxer<frameratesCount>(framerates);
        }

        auto createMuxer()
        {
            return createMuxer(AVRational{1, 24});
        }

        void expectCountlessFillBufferForAllStreams()
        {
            expectCountlessFillBufferForAllStreams(std::make_index_sequence<STREAMS_COUNT>());
        }

        template <std::size_t... StreamsIndices>
        void expectCountlessFillBufferForAllStreams(std::index_sequence<StreamsIndices...>)
        {
            (EXPECT_CALL(onStreamCtxtMock<StreamsIndices>(), fillBuffer(_)).Times(AnyNumber()), ...);
        }

        void expectCountlessBooleanCastForAllStreamsReturning(bool returnVal)
        {
            expectCountlessBooleanCastForAllStreamsReturning(returnVal, std::make_index_sequence<STREAMS_COUNT>());
        }

        template <std::size_t... StreamsIndices>
        void expectCountlessBooleanCastForAllStreamsReturning(bool returnVal, std::index_sequence<StreamsIndices...>)
        {
            (EXPECT_CALL(onStreamCtxtMock<StreamsIndices>(), boolOp()).WillRepeatedly(Return(returnVal)), ...);
        }

        void expectCountlessGetTimeBaseReturningFps()
        {
            expectCountlessGetTimeBaseReturningFps(std::make_index_sequence<STREAMS_COUNT>());
        }

        template <std::size_t... StreamsIndices>
        void expectCountlessGetTimeBaseReturningFps(std::index_sequence<StreamsIndices...>)
        {
            (EXPECT_CALL(onStreamCtxtMock<StreamsIndices>(), getTimeBase()).WillRepeatedly(Return(AVRational {1, FPS})), ...);
        }

        void expectAllStreamsToReturnNumberOfFramesAndThenNothing(unsigned framesToReturnCnt, bool isFinal, AVPacket packet = {.size = 1, .duration = 1})
        {
            expectAllStreamsToReturnNumberOfFramesAndThenNothing(framesToReturnCnt, isFinal, packet, std::make_index_sequence<STREAMS_COUNT>());
        }

        template <std::size_t... StreamsIndices>
        void expectAllStreamsToReturnNumberOfFramesAndThenNothing(unsigned framesToReturnCnt, bool isFinal, AVPacket packet, std::index_sequence<StreamsIndices...>)
        {
            if(isFinal)
                (EXPECT_CALL(onStreamCtxtMock<StreamsIndices>(), getNextFrame()).WillRepeatedly(Return(AVPacket {.size = 0})), ...);
            else
                (EXPECT_CALL(onStreamCtxtMock<StreamsIndices>(), getNextFrame()).WillOnce(Return(AVPacket {.size = 0})).RetiresOnSaturation(), ...);
            
            (EXPECT_CALL(onStreamCtxtMock<StreamsIndices>(), getNextFrame()).Times(framesToReturnCnt)
                                                                            .WillRepeatedly(Return(packet))
                                                                            .RetiresOnSaturation(), ...);
        }

        template <unsigned StreamsCount>
        bool muxDataToAllStreams(MuxerTest<StreamsCount>& muxer)
        {
            return muxDataToAllStreams(muxer, std::make_index_sequence<StreamsCount>());
        }

        template <unsigned StreamsCount, std::size_t... StreamsIndices>
        bool muxDataToAllStreams(MuxerTest<StreamsCount>& muxer, std::index_sequence<StreamsIndices...>)
        {
            return (muxer.template muxMediaData<StreamsIndices>(inputData) | ...);
        }

        std::shared_ptr<MediaContainerWrapper>  containerCtxtMock = std::make_shared<StrictMock<MediaContainerMock>>("mp4");
        std::array<WrappedMediaStreamSharedPtr,
                   STREAMS_COUNT>            streamCtxtMocks;

        const ByteVector inputData = {0, 1, 2, 3, 4, 5, 6, 7};
        const ByteVector outputData = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
};

using MuxerVariants = Types<Muxer<1>, Muxer<2>, Muxer<4>>;
TYPED_TEST_SUITE(MuxerTestFixture, MuxerVariants);

TYPED_TEST(MuxerTestFixture, MuxerShouldNotAcceptZeroOrNegativeFramerate)
{
    ASSERT_THROW(this->createMuxerForFramerateTest({0, 0}),   std::invalid_argument);
    ASSERT_THROW(this->createMuxerForFramerateTest({1, 0}),   std::invalid_argument);
    ASSERT_THROW(this->createMuxerForFramerateTest({0, 1}),   std::invalid_argument);
    ASSERT_THROW(this->createMuxerForFramerateTest({1, -1}),  std::invalid_argument);
    ASSERT_THROW(this->createMuxerForFramerateTest({-1, 1}),  std::invalid_argument);
    ASSERT_THROW(this->createMuxerForFramerateTest({-1, -1}), std::invalid_argument);
}

TYPED_TEST(MuxerTestFixture, MuxerShouldNotReturnMuxedDataWhileNoInputDataIsBeingProvided)
{
    this->expectCountlessFillBufferForAllStreams();

    this->expectCountlessBooleanCastForAllStreamsReturning(false);
    
    EXPECT_CALL(this->onContainerCtxtMock(), getMuxedData()).WillRepeatedly(Return(ByteVector{}));

    auto muxer = this->createMuxer();
    auto data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());

    muxer.flush();
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}

TYPED_TEST(MuxerTestFixture, MuxerShouldNotReturnMuxedDataWhileAvcodecInternalsAreStillInitializing)
{
    this->expectCountlessFillBufferForAllStreams();

    this->expectCountlessBooleanCastForAllStreamsReturning(false);

    EXPECT_CALL(this->onContainerCtxtMock(), getMuxedData()).WillRepeatedly(Return(ByteVector{}));

    auto muxer = this->createMuxer();
    this->muxDataToAllStreams(muxer);
    auto data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());

    muxer.flush();
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}

TYPED_TEST(MuxerTestFixture, MuxerShouldReturnMuxedDataOnlyWhenItIsSuccessfullyMuxed)
{
    
    this->expectCountlessFillBufferForAllStreams();

    this->expectCountlessBooleanCastForAllStreamsReturning(true);
    
    this->expectAllStreamsToReturnNumberOfFramesAndThenNothing(SOME_FRAMES_CNT_PER_STREAM, IS_FINAL);
    this->expectAllStreamsToReturnNumberOfFramesAndThenNothing(SOME_FRAMES_CNT_PER_STREAM, not IS_FINAL);
    
    this->expectCountlessGetTimeBaseReturningFps();

    EXPECT_CALL(this->onContainerCtxtMock(), boolOp()).WillRepeatedly(Return(true));

    EXPECT_CALL(this->onContainerCtxtMock(), muxFramePacket(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(this->onContainerCtxtMock(), muxFramePacket(_)).Times(this->STREAMS_COUNT).WillRepeatedly(Return(true)).RetiresOnSaturation();
    EXPECT_CALL(this->onContainerCtxtMock(), muxFramePacket(_)).Times(SOME_FRAMES_CNT_PER_STREAM * this->STREAMS_COUNT).WillRepeatedly(Return(false)).RetiresOnSaturation();

    EXPECT_CALL(this->onContainerCtxtMock(), getMuxedData()).WillOnce(Return(ByteVector{})).WillOnce(Return(this->outputData)).WillOnce(Return(ByteVector{}));

    EXPECT_CALL(this->onContainerCtxtMock(), getMaxInterleaveDelta()).WillRepeatedly(Return(900000));

    auto muxer = this->createMuxer();
    ASSERT_FALSE(this->muxDataToAllStreams(muxer));
    auto data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());

    ASSERT_TRUE(this->muxDataToAllStreams(muxer));
    data = muxer.getMuxedData();
    ASSERT_EQ(data, this->outputData);

    ASSERT_FALSE(this->muxDataToAllStreams(muxer));
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}

TYPED_TEST(MuxerTestFixture, MuxerShouldReturnMuxedDataWhileFlushingOnlyWhenDataIsQueuedForMuxingAndCanBeFlushed)
{
    if(this->STREAMS_COUNT < 2)
        return;
    
    this->expectCountlessFillBufferForAllStreams();

    this->expectCountlessBooleanCastForAllStreamsReturning(true);

    this->expectAllStreamsToReturnNumberOfFramesAndThenNothing(MORE_FRAMES_CNT_PER_STREAM, IS_FINAL, {.size = 1, .duration = DURATION});

    this->expectCountlessGetTimeBaseReturningFps();

    EXPECT_CALL(this->onContainerCtxtMock(), boolOp()).WillRepeatedly(Return(true));

    EXPECT_CALL(this->onContainerCtxtMock(), muxFramePacket(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(this->onContainerCtxtMock(), getMuxedData()).WillOnce(Return(ByteVector{}));
    EXPECT_CALL(this->onContainerCtxtMock(), getMuxedData()).Times(2).WillRepeatedly(Return(this->outputData)).RetiresOnSaturation();

    EXPECT_CALL(this->onContainerCtxtMock(), getMaxInterleaveDelta()).WillRepeatedly(Return(100ULL*AV_TIME_BASE));

    auto muxer = this->createMuxer();
    this->muxDataToAllStreams(muxer);
    auto data = muxer.getMuxedData();
    ASSERT_EQ(data, this->outputData);

    ASSERT_TRUE(muxer.flush());
    data = muxer.getMuxedData();
    ASSERT_EQ(data, this->outputData);

    ASSERT_FALSE(muxer.flush());
    data = muxer.getMuxedData();
    ASSERT_TRUE(data.empty());
}
}
