cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Project name
project(net-frame LANGUAGES CXX)

# Use c++11
add_definitions(-std=c++17)
add_definitions(-D_LINUX)
add_definitions(-fPIC)

# Directories
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(3RD_DIR ${ROOT_DIR}/third-party)
set(SRC_DIR ${ROOT_DIR}/src)

# Debug or release
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g -ggdb")
    set(OUT_DIR ${ROOT_DIR}/output/core/net-frame/linux/debug)
    message("Debug mode")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")
    set(OUT_DIR ${ROOT_DIR}/output/core/net-frame/linux/release)
    message("Release mode")
endif()

# Output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR})

# Source directories
aux_source_directory(${SRC_DIR}/core/net-frame SRC_FILES)

# Include directories
include_directories(${SRC_DIR}/common/public)
include_directories(${SRC_DIR}/common/util)
include_directories(${SRC_DIR}/common/timer)
include_directories(${SRC_DIR}/core/net-frame/include)
include_directories(${SRC_DIR}/core/base-frame/include)
include_directories(${3RD_DIR})
include_directories(${3RD_DIR}/json)
include_directories(${3RD_DIR}/libevent/include)
include_directories(${3RD_DIR}/concurrentqueue)
include_directories(${3RD_DIR}/fec)

# Library directories
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    link_directories(${ROOT_DIR}/output/core/base-frame/linux/debug)
    link_directories(${ROOT_DIR}/output/common/util/linux/debug)
    link_directories(${ROOT_DIR}/third-party/libevent/lib/linux)
else()
    link_directories(${ROOT_DIR}/output/core/base-frame/linux/release)
    link_directories(${ROOT_DIR}/output/common/util/linux/release)
    link_directories(${ROOT_DIR}/third-party/libevent/lib/linux)
endif()

# Library target
add_library(net-frame SHARED ${3RD_DIR}/fec/fec.c ${SRC_FILES})

# Link
target_link_libraries(net-frame base-frame util event)
