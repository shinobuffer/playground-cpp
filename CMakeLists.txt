cmake_minimum_required(VERSION 3.21)

# 默认设置为 Release 模式
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(playground)

include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Turn off google benchmark test")
FetchContent_Declare(benchmark GIT_REPOSITORY https://github.com/google/benchmark GIT_TAG main)
FetchContent_Declare(cxxopts GIT_REPOSITORY https://github.com/jarro2783/cxxopts)
FetchContent_MakeAvailable(benchmark cxxopts)

add_executable(main main.cpp)
target_include_directories(main PRIVATE src 3rdparty)
target_link_libraries(main PRIVATE benchmark::benchmark cxxopts)
