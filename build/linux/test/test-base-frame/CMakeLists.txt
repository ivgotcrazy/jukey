cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Project name
project(test-base-frame LANGUAGES CXX)

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
    set(OUT_DIR ${ROOT_DIR}/output/test/test-base-frame/linux/debug)
    message("Debug mode")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")
    set(OUT_DIR ${ROOT_DIR}/output/test-base-frame/linux/release)
    message("Release mode")
endif()

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})

# Source directories
aux_source_directory(${ROOT_DIR}/test/test-base-frame SRC_FILES)

# Include directories
include_directories(${SRC_DIR}/common/util)
include_directories(${SRC_DIR}/core/base-frame/include)
include_directories(${ROOT_DIR}/test/test-component)

# Library directories
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    link_directories(${ROOT_DIR}/output/core/base-frame/linux/debug)
    link_directories(${ROOT_DIR}/output/common/util/linux/debug)
else()
    link_directories(${ROOT_DIR}/output/core/base-frame/linux/release)
    link_directories(${ROOT_DIR}/output/common/util/linux/release)
endif()

# Target
add_executable(test-base-frame ${SRC_FILES})

# Link library
target_link_libraries(test-base-frame base-frame)
target_link_libraries(test-base-frame util)
target_link_libraries(test-base-frame -ldl)
