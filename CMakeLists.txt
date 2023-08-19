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

add_executable(main main.cpp)
target_include_directories(main PUBLIC include src 3rdparty)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Turn off google benchmark test")
add_subdirectory(3rdparty/benchmark)
target_link_libraries(main PUBLIC benchmark)