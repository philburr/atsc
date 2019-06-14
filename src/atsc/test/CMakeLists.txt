cmake_minimum_required(VERSION 3.12)

file(GLOB data_files RELATIVE ${EXTERNAL_PROJECTS_SOURCE_DIR}/atsc_test_data ${EXTERNAL_PROJECTS_SOURCE_DIR}/atsc_test_data/*.data)
foreach(data_file ${data_files})
	configure_file("${EXTERNAL_PROJECTS_SOURCE_DIR}/atsc_test_data/${data_file}" "${CMAKE_CURRENT_BINARY_DIR}/data/${data_file}" COPYONLY)
endforeach()


add_executable(randomize_ut randomize_ut.cpp)
target_link_libraries(randomize_ut PRIVATE unittest build_settings)

add_executable(reed_solomon_ut reed_solomon_ut.cpp)
target_link_libraries(reed_solomon_ut PRIVATE unittest build_settings)

add_executable(interleaver_ut interleaver_ut.cpp)
target_link_libraries(interleaver_ut PRIVATE unittest build_settings)

add_executable(trellis_ut trellis_ut.cpp)
target_link_libraries(trellis_ut PRIVATE unittest build_settings)
target_compile_options(trellis_ut
	PRIVATE
	${CXX_BMI2_FLAGS}
	${CXX_AVX2_FLAGS}
	${CXX_FMA_FLAGS}
)

add_executable(field_sync_ut field_sync_ut.cpp)
target_link_libraries(field_sync_ut PRIVATE unittest build_settings)

add_executable(tune_ut tune_ut.cpp)
target_link_libraries(tune_ut PRIVATE unittest build_settings)
target_compile_options(tune_ut
	PRIVATE
	${CXX_BMI2_FLAGS}
	${CXX_AVX2_FLAGS}
	${CXX_FMA_FLAGS}
)

add_executable(filter_ut filter_ut.cpp)
target_link_libraries(filter_ut PRIVATE unittest build_settings)
target_compile_options(filter_ut
	PRIVATE
	${CXX_BMI2_FLAGS}
	${CXX_AVX2_FLAGS}
	${CXX_FMA_FLAGS}
)


include(CTest)
include(Catch)
catch_discover_tests(randomize_ut)
catch_discover_tests(reed_solomon_ut)
catch_discover_tests(interleaver_ut)
catch_discover_tests(trellis_ut)
catch_discover_tests(field_sync_ut)
catch_discover_tests(tune_ut)
catch_discover_tests(filter_ut)
