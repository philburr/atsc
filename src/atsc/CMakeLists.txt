cmake_minimum_required(VERSION 3.12)

add_library(atsc
	atsc.cpp
)
target_link_libraries(atsc PRIVATE build_settings PUBLIC Threads::Threads)
target_compile_options(atsc
	PRIVATE
	-fPIC
	${CXX_SSE_FLAGS}
)
target_compile_definitions(atsc PRIVATE ${CXX_SSE_DEFINES})

target_include_directories(atsc
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/..
)
add_subdirectory(test)
