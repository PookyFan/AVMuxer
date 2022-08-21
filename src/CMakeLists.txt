cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB Src "./*.cpp")
add_library(AVMuxerLib STATIC ${Src})
target_include_directories(AVMuxerLib PUBLIC ${AVMuxer_SOURCE_DIR}/include)
target_link_libraries(AVMuxerLib)
