cmake_minimum_required(VERSION 3.12)

add_library(dep2 INTERFACE)
target_link_libraries(dep2 INTERFACE unittest atsc_ocl ${OpenCL_LIBRARIES})
add_library(dep INTERFACE)
target_link_libraries(dep INTERFACE build_settings dep2)

add_executable(randomize_cl_ut randomize_cl_ut.cpp)
target_link_libraries(randomize_cl_ut PRIVATE dep)

add_executable(rs_encode_cl_ut rs_encode_cl_ut.cpp)
target_link_libraries(rs_encode_cl_ut PRIVATE dep)

add_executable(interleaver_cl_ut interleaver_cl_ut.cpp)
target_link_libraries(interleaver_cl_ut PRIVATE dep)

add_executable(trellis_cl_ut trellis_cl_ut.cpp)
target_link_libraries(trellis_cl_ut PRIVATE dep)
target_compile_options(trellis_cl_ut
	PRIVATE
	${CXX_BMI2_FLAGS}
	${CXX_AVX2_FLAGS}
	${CXX_FMA_FLAGS}
)

add_executable(field_sync_cl_ut field_sync_cl_ut.cpp)
target_link_libraries(field_sync_cl_ut PRIVATE dep)

add_executable(tune_cl_ut tune_cl_ut.cpp)
target_link_libraries(tune_cl_ut PRIVATE dep)
target_compile_options(tune_cl_ut
	PRIVATE
	${CXX_AVX2_FLAGS}
	${CXX_FMA_FLAGS}
)

add_executable(filter_cl_ut filter_cl_ut.cpp)
target_link_libraries(filter_cl_ut PRIVATE dep)
target_compile_options(filter_cl_ut
	PRIVATE
	${CXX_AVX2_FLAGS}
	${CXX_FMA_FLAGS}
)


include(CTest)
include(Catch)
catch_discover_tests(randomize_cl_ut)
catch_discover_tests(rs_encode_cl_ut)
catch_discover_tests(interleaver_cl_ut)
catch_discover_tests(trellis_cl_ut)
catch_discover_tests(tune_cl_ut)
catch_discover_tests(filter_cl_ut)
