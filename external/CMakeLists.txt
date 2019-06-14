cmake_minimum_required(VERSION 3.12)
project(EXTERNAL_PROJECTS)
add_subdirectory(catch2)

add_library(unittest
    unittest_runner.cpp
)
target_link_libraries(unittest PUBLIC Catch2::Catch2 stdc++fs)
target_compile_definitions(unittest PUBLIC -DBUILD_UNITTEST)
target_compile_features(unittest PUBLIC cxx_std_17)

include(Catch)

add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cxxopts/include)