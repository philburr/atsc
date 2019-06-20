#include "catch2/catch.hpp"
#include "../opencl.cpp"
#include "atsc/offset.h"

TEST_CASE("ATSC Tune", "[opencl,tune]") {

    atsc_opencl atsc;

    // One field of pure DC
    auto ocl_data = atsc_field_symbol();
    alignas(32) auto valid_data = atsc_field_symbol_padded();
    for (size_t i = 0; i < ocl_data.size(); i++) {
        ocl_data[i] = atsc_symbol_type(1, 0);
        valid_data[i] = atsc_symbol_type(1, 0);
    }

    // Use atsc_randomizer to generate validation data
    auto offset = atsc_offset();
    offset.process_field(valid_data);

    auto data = atsc.cl_alloc_arr<atsc_field_symbol>();
    atsc.to_device(data.data(), ocl_data.data(), ocl_data.size() * sizeof(ocl_data[0]));
    auto event = atsc.tune(data);
    atsc.to_host(ocl_data.data(), data.data(), ocl_data.size() * sizeof(ocl_data[0]), event);
    atsc.cl_free(data);

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))
    for (size_t i = 0; i < ocl_data.size(); i++) {
        REQUIRE(IS_CLOSE_CPLX(ocl_data[i], valid_data[i]));
    }
}