cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Project name and language
project(util LANGUAGES CXX)

# Use c++11
add_definitions(-std=c++17)
add_definitions(-D_LINUX)
add_definitions(-fPIC)

# Directories
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(3RD_DIR ${ROOT_DIR}/third-party)
set(SRC_DIR ${ROOT_DIR}/src)
set(UTIL_SRC_DIR ${SRC_DIR}/common/util)

# Debug or release
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g -ggdb")
    set(OUT_DIR ${ROOT_DIR}/output/common/util/linux/debug)
    message("Debug mode")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall")
    set(OUT_DIR ${ROOT_DIR}/output/common/util/linux/release)
    message("Release mode")
endif()

# Output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})

# Source directories
aux_source_directory(${UTIL_SRC_DIR}/common SRC_FILES)
aux_source_directory(${UTIL_SRC_DIR}/log SRC_FILES)
aux_source_directory(${UTIL_SRC_DIR}/msg-bus SRC_FILES)
aux_source_directory(${UTIL_SRC_DIR}/public SRC_FILES)
aux_source_directory(${UTIL_SRC_DIR}/event SRC_FILES)
aux_source_directory(${UTIL_SRC_DIR}/thread SRC_FILES)
aux_source_directory(${UTIL_SRC_DIR}/async SRC_FILES)

# Exclude srouce files
list(FILTER SRC_FILES EXCLUDE REGEX ".*util-ffmpeg.cpp$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*util-mf.cpp$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*util-sdl.cpp$")

# Include directories
include_directories(${SRC_DIR}/common/public)
include_directories(${SRC_DIR}/common/util)
include_directories(${SRC_DIR}/common/timer)
include_directories(${SRC_DIR}/common/property)
include_directories(${SRC_DIR}/common/protocol)
include_directories(${SRC_DIR}/common/amqp-client)
include_directories(${SRC_DIR}/core/base-frame/include)
include_directories(${SRC_DIR}/core/streamer/include)
include_directories(${SRC_DIR}/core/net-frame/include)
include_directories(${3RD_DIR})
include_directories(${3RD_DIR}/json)
include_directories(${3RD_DIR}/concurrentqueue)
include_directories(${3RD_DIR}/libevent/include)

# Library target
add_library(util STATIC ${SRC_FILES})

