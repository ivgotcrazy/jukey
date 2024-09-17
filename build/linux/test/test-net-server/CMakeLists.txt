cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Project name
project(test-net-server LANGUAGES CXX)

# Use c++11
add_definitions(-std=c++17)
add_definitions(-D_LINUX)

# Pre-defined directories
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(3RD_DIR ${ROOT_DIR}/third-party)
set(SRC_DIR ${ROOT_DIR}/src)

# Debug or release
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g -ggdb")
    set(OUT_DIR ${ROOT_DIR}/output/test/test-net-server/linux/debug)
    message("Debug mode")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")
    set(OUT_DIR ${ROOT_DIR}/output/test/test-net-server/linux/release)
    message("Release mode")
endif()

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})

# Source directories
aux_source_directory(${ROOT_DIR}/test/test-net-server SRC_FILES)

# Include directories
include_directories(${SRC_DIR}/common/public)
include_directories(${SRC_DIR}/common/util)
include_directories(${SRC_DIR}/common/timer)
include_directories(${SRC_DIR}/core/base-frame/include)
include_directories(${SRC_DIR}/core/net-frame/include)
include_directories(${3RD_DIR}/clipp/include)
include_directories(${3RD_DIR})

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

# Target
add_executable(test-net-server ${SRC_FILES})

# Link library
target_link_libraries(test-net-server base-frame)
target_link_libraries(test-net-server util)
target_link_libraries(test-net-server event)
target_link_libraries(test-net-server dl)
target_link_libraries(test-net-server pthread)
