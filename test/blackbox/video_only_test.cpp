
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "Mp4Muxer.hpp"
#include "utils.hpp"

#include "TestLogger.hpp"

extern "C"
{
    #include <libavutil/log.h>
}

int main(int argc, char** argv)
try
{
    if(argc < 4)
    {
        std::cout << "Usage: single_video_stream_test <input video stream file path> <output .mp4 file path> <fps>" << std::endl;
        return 1;
    }

    int fps = std::atoi(argv[3]);
    std::fstream inputFile(argv[1], std::ios::in | std::ios::binary);
    if(!inputFile.is_open())
    {
        std::cout << "Could not open input file " << argv[1] << std::endl;
        return 1;
    }

    std::fstream outputFile(argv[2], std::ios::out | std::ios::binary | std::ios::trunc);
    if(!outputFile.is_open())
    {
        std::cout << "Could not open output file " << argv[2] << std::endl;
        return 1;
    }

    av_log_set_level(AV_LOG_TRACE);
    AVMuxer::setLogger(std::make_unique<TestLogger>());
    AVMuxer::VideoOnlyMp4Muxer muxer({1, fps});
    std::istreambuf_iterator it(inputFile);
    decltype(it) end;
    std::vector<uint8_t> readBuffer(it, end);
    if(muxer.muxVideoData(readBuffer))
    {
        auto muxedData = muxer.getMuxedData();
        outputFile.write(reinterpret_cast<char*>(muxedData.data()), muxedData.size());
    }

    if(!inputFile && !inputFile.eof())
    {
        std::cout << "Error reading input file" << std::endl;
        return 2;
    }

    if(!outputFile)
    {
        std::cout << "Error reading output file" << std::endl;
        return 2;
    }

    std::cout << "Video muxed successfully" << std::endl;
    return 0;
}
catch(std::exception e)
{
    std::cout << e.what() << std::endl;
    return 1;
}
catch(...)
{
    std::cout << "An unknown error occurred!" << std::endl;
    return 3;
}