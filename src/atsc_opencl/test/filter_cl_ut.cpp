#include "catch2/catch.hpp"
#include "../opencl.cpp"
#include "atsc/filter.h"
#include <random>
#include <algorithm>

void test_rrc_filter(long unsigned int seed) {

    atsc_opencl atsc;

    auto eng = std::mt19937{seed};
    auto rnd = std::uniform_real_distribution<float>(-1, 1); 

    auto field = std::make_unique<atsc_field_symbol_padded>();
    for (size_t i = 0; i < field->size(); i++) {
        (*field)[i] = atsc_symbol_type(rnd(eng), rnd(eng));
    }

    // Device
    auto device_field = std::make_unique<atsc_field_symbol>();
    auto input = atsc.cl_alloc_arr<atsc_field_symbol_padded>();
    atsc.to_device(input.data(), field->data(), field->size() * sizeof((*field)[0]));
    auto output = atsc.cl_alloc_arr<atsc_field_symbol>();
    auto event = atsc.filter(output, input);
    atsc.to_host(device_field->data(), output.data(), device_field->size() * sizeof((*device_field)[0]), event);
    atsc.cl_free(input);
    atsc.cl_free(output);

    // Host
    auto host_field = std::make_unique<atsc_field_symbol>();
    auto host_filter = atsc_rrc_filter();
    host_filter.process_field(*host_field, *field);

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))
    for (size_t i = 0; i < ATSC_SYMBOLS_PER_FIELD; i++) {
        REQUIRE(IS_CLOSE_CPLX((*host_field)[i], (*device_field)[i]));
    }
}

TEST_CASE("ATSC RRC Filter", "[opencl,filter]") {
    test_rrc_filter(0);
}