cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Project name and language
project(protocol LANGUAGES CXX)

# Use c++11
add_definitions(-std=c++11)
add_definitions(-fPIC)

# Directories
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(3RD_DIR ${ROOT_DIR}/third-party)
set(SRC_DIR ${ROOT_DIR}/src)
set(PROT_SRC_DIR ${SRC_DIR}/common/protocol)

# Debug or release
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g -ggdb")
    set(OUT_DIR ${ROOT_DIR}/output/common/protocol/linux/debug)
    message("Debug mode")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")
    set(OUT_DIR ${ROOT_DIR}/output/common/protocol/linux/release)
    message("Release mode")
endif()

# Output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})

# Source directories
aux_source_directory(${PROT_SRC_DIR} SRC_FILES)

# Include directories
include_directories(${SRC_DIR}/common/public)
include_directories(${SRC_DIR}/common/util)
include_directories(${3RD_DIR}/protobuf/include)

# Library target
add_library(protocol STATIC ${SRC_FILES})

