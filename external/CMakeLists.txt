cmake_minimum_required(VERSION 3.12)
add_subdirectory(catch2)

add_library(unittest
    unittest_runner.cpp
)
target_link_libraries(unittest PUBLIC Catch2::Catch2)

include(Catch)

add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cxxopts/include)