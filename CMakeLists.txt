cmake_minimum_required(VERSION 3.12)

project(atsc LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/catch2/contrib")

include(CTest)
include(FindSSE)
include(CTest)
find_package(Threads)

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(util)
