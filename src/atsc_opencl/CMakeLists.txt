cmake_minimum_required(VERSION 3.12)

find_package(OpenCL REQUIRED)

set(SOURCES
	opencl.cpp
)

add_library(atsc_ocl
	${SOURCES}
)
target_link_libraries(atsc_ocl PRIVATE build_settings PUBLIC Threads::Threads)
target_compile_options(atsc_ocl
	PRIVATE
	-fPIC
)
target_include_directories(atsc_ocl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(atsc_ocl_test
	${SOURCES}
)
target_link_libraries(atsc_ocl_test PRIVATE build_settings PUBLIC Threads::Threads unittest)
target_compile_options(atsc_ocl_test
	PRIVATE
	-fPIC
)
target_include_directories(atsc_ocl_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_subdirectory(test)