cmake_minimum_required(VERSION 3.12)

add_library(atsc
	atsc.cpp
)
target_link_libraries(atsc build_settings)
target_compile_options(atsc
	PUBLIC
	${CXX_BMI2_FLAGS}
	${CXX_AVX2_FLAGS}
	${CXX_FMA_FLAGS}
)
target_compile_definitions(atsc PUBLIC ${CXX_SSE_DEFINES})

message("SSE: ${CXX_SSE_DEFINES}")

target_include_directories(atsc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)