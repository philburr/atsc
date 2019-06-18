#include "catch2/catch.hpp"
#include "../opencl.cpp"
#include "atsc/reed_solomon.h"

TEST_CASE("ATSC Reed Solomon", "[rs_encode]") {

    atsc_opencl atsc;

    // One frame worth of mpeg input
    auto data = atsc_field_data();
    for (unsigned seg = 0; seg < ATSC_DATA_SEGMENTS; seg++) {
        memset(data.data() + seg * ATSC_SEGMENT_FEC_BYTES, seg, ATSC_SEGMENT_BYTES);
        memset(data.data() + seg * ATSC_SEGMENT_FEC_BYTES + ATSC_SEGMENT_BYTES, 0, ATSC_RS_BYTES);
    }

    // Use atsc_randomizer to generate validation data
    auto valid_data = atsc_field_data();
    memcpy(valid_data.data(), data.data(), valid_data.size());
    auto rs_encode = atsc_reed_solomon();
    rs_encode.process_field(valid_data.data());

    auto input = atsc.cl_alloc(data.size());
    auto output = atsc.cl_alloc(data.size());
    auto final = atsc_field_data();

    atsc.to_device(input, data.data(), data.size());
    auto event = atsc.rs_encode(input);
    atsc.to_host(final.data(), input, data.size(), event);

    for (size_t i = 0; i < final.size(); i++) {
        REQUIRE(final[i] == valid_data[i]);
    }

    atsc.cl_free(input);
    atsc.cl_free(output);
}