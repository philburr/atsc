cmake_minimum_required(VERSION 3.12)

find_package(Threads)

add_library(build_settings INTERFACE)
target_compile_options(build_settings INTERFACE -Wall -Werror)
if (ENABLE_ASAN)
	target_compile_options(build_settings INTERFACE -fsanitize=address)
	target_link_libraries(build_settings INTERFACE -static-libasan asan)
endif()

target_compile_features(build_settings INTERFACE cxx_std_17)
target_link_libraries(build_settings INTERFACE Threads::Threads)
target_include_directories(build_settings INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(atsc)
add_subdirectory(atsc_opencl)