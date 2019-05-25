cmake_minimum_required(VERSION 3.12)

find_package(SoapySDR)

add_executable(atsc_encoder
    atsc_encoder.cpp
)
target_link_libraries(atsc_encoder build_settings atsc cxxopts SoapySDR)

add_executable(atsc_encoder_null
    atsc_encoder_null.cpp
)
target_link_libraries(atsc_encoder_null build_settings atsc)


