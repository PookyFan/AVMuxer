cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()
find_package(GTest)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)
file(GLOB TestSrc "./*.cpp")
add_executable(UnitTestsExec ${TestSrc})
target_link_libraries(UnitTestsExec GTest::gtest_main)
gtest_add_tests(TARGET UnitTestsExec)
