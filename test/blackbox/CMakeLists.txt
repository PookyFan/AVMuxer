cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(video_only_test "video_only_test.cpp")
target_include_directories(video_only_test PUBLIC "./")
target_link_libraries(video_only_test AVMuxerLib)

add_executable(audio_video_test "audio_video_test.cpp")
target_include_directories(audio_video_test PUBLIC "./")
target_link_libraries(audio_video_test AVMuxerLib)
