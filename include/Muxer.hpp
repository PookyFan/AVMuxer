#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

#include "BaseMuxer.hpp"
#include "MediaStreamWrapper.hpp"

namespace AVMuxer
{
template <unsigned StreamsCount>
class Muxer : public BaseMuxer
{
    static_assert(StreamsCount > 0);

    public:
        Muxer(const char* formatName, AVRational framerate) : Muxer(formatName, std::array {framerate})
        {}

        template <long unsigned VideoStreamsCount>
        Muxer(const char* formatName, const std::array<AVRational, VideoStreamsCount>& framerates) : BaseMuxer(formatName)
        {
            static_assert(VideoStreamsCount <= StreamsCount);

            auto currentStream = streams.begin();
            for(auto& fps : framerates)
            {
                if(fps.num <= 0 || fps.den <= 0)
                    throw std::invalid_argument("Framerate can't be zero");
                
                *(currentStream++) = containerCtxt->createStream(fps);
            }

            while(currentStream != streams.end())
                *(currentStream++) = containerCtxt->createStream();
        }

        void updateStreamRelativeTimeAhead(MediaStreamWrapper& mediaCtxt, int64_t diff) override
        {
            mediaCtxt.updateRelativeTimeAhead(diff);
            if(!shouldStreamBeLimited(mediaCtxt))
                return;
            
            auto minTimeAhead = std::numeric_limits<int64_t>::max();
            std::for_each(streams.begin(), streams.end(), [&minTimeAhead](const auto& current)
            {
                if(auto time = current->getRelativeTimeAhead(); time < minTimeAhead)
                    minTimeAhead = time;
            });

            std::for_each(streams.begin(), streams.end(), [minTimeAhead](auto& current)
            {
                current->updateRelativeTimeAhead(-minTimeAhead);
            });
        }

        static auto getStreamsCount()
        {
            return StreamsCount;
        }

        bool shouldStreamBeLimited(MediaStreamWrapper& mediaCtxt) override
        {
            return mediaCtxt.getRelativeTimeAhead() > timeAheadInCommonTimebaseLimit;
        }

        template <unsigned StreamNumber, class ContainerT>
        bool muxMediaData(const ContainerT& inputData)
        {
            static_assert(StreamNumber < StreamsCount);
            BaseMuxer::muxMediaData(*streams[StreamNumber], ByteArray { inputData.data(), inputData.size() });
            return hasMuxedData();
        }

        bool flush()
        {
            flushAllStreams(std::make_index_sequence<StreamsCount>());
            return hasMuxedData();
        }

        static constexpr unsigned STREAMS_COUNT = StreamsCount;

        private:
            template <std::size_t... StreamsIndices>
            int flushAllStreams(std::index_sequence<StreamsIndices...>)
            {
                ByteVector dummy;
                auto result = (muxMediaData<StreamsIndices>(dummy) + ...);
                return result;
            }
    
    protected:
        std::array<WrappedMediaStreamSharedPtr, StreamsCount> streams;
};
}
