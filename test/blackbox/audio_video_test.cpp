#include <iostream>
#include <fstream>
#include <string_view>

#include "Mp4Muxer.hpp"
#include "utils.hpp"

constexpr auto BUFSIZE = 256 * 4096;

extern "C"
{
    #include <libavutil/log.h>
}

class TestLogger : public AVMuxer::ILogger
{
    public:
        void logAVMuxerMessage(const ::std::string& msg, AVMuxer::LogLevel level) override
        {
            std::cout << msg << std::endl;
        }
};

int main(int argc, char** argv)
{
    if(argc < 5)
    {
        std::cout << "Usage: audio_video_test <input H264 video stream file path> "
                     "<input FLAC stream file path> <output .mp4 file path> <fps>" << std::endl;
        return 1;
    }

    int fps = std::atoi(argv[4]);
    std::fstream videoFile(argv[1], std::ios::in | std::ios::binary);
    if(!videoFile.is_open())
    {
        std::cout << "Could not open video file " << argv[1] << std::endl;
        return 1;
    }

    std::fstream audioFile(argv[2], std::ios::in | std::ios::binary);
    if(!audioFile.is_open())
    {
        std::cout << "Could not open audio file " << argv[2] << std::endl;
        return 1;
    }

    std::fstream outputFile(argv[3], std::ios::out | std::ios::binary | std::ios::trunc);
    if(!outputFile.is_open())
    {
        std::cout << "Could not open output file " << argv[3] << std::endl;
        return 1;
    }

    av_log_set_level(AV_LOG_TRACE);
    AVMuxer::setLogger(std::make_unique<TestLogger>());
    uint8_t buff[BUFSIZE];
    AVMuxer::Mp4Muxer muxer({1, fps});
    for(;
        !videoFile.eof() && ( !(videoFile.eof() || videoFile.fail()) || !(audioFile.eof() || audioFile.fail()) );)
    {
        //Video
        if(videoFile)
        {
            std::cout << "Muxing some video\n";
            videoFile.read(reinterpret_cast<char*>(buff), BUFSIZE);
            auto readSize = videoFile.gcount();
            std::cout << "Input video batch size: " << readSize << std::endl;
            if(muxer.muxVideoData(AVMuxer::ByteVector(buff, buff + readSize)))
            {
                auto data = muxer.getMuxedData();
                outputFile.write(reinterpret_cast<char*>(data.data()), data.size());
            }
            else
                std::cout << "No output data...\n";
            if(outputFile.fail())
            {
                std::cout << "Error writting output file" << std::endl;
                break;
            }
        }

        //Audio
        if(audioFile)
        {
            std::cout << "Muxing some audio\n";
            audioFile.read(reinterpret_cast<char*>(buff), BUFSIZE);
            auto readSize = audioFile.gcount();
            std::cout << "Input audio batch size: " << readSize << std::endl;
            if(muxer.muxAudioData(AVMuxer::ByteVector(buff, buff + readSize)))
            {
                auto data = muxer.getMuxedData();
                outputFile.write(reinterpret_cast<char*>(data.data()), data.size());
            }
            else
                std::cout << "No output data...\n";
            if(outputFile.fail())
            {
                std::cout << "Error writting output file" << std::endl;
                break;
            }
        }

        //Flushed data
        std::cout << "Flushing muxer\n";
        if(muxer.flush())
        {
            auto data = muxer.getMuxedData();
            outputFile.write(reinterpret_cast<char*>(data.data()), data.size());
        }
        else
                std::cout << "No output data...\n";
        if(outputFile.fail())
        {
            std::cout << "Error writting output file" << std::endl;
            break;
        }
    }

    std::cout << "[TEST] videoFile.eof() = " << videoFile.eof() << " videoFile.fail() = " << videoFile.fail()
                  << " audioFile.eof() = " << audioFile.eof() << " audioFile.fail() = " << audioFile.fail() << std::endl;

    std::cout << "Video and audio muxed successfully" << std::endl;
    return 0;
}