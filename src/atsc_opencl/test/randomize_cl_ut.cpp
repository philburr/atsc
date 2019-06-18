#include "catch2/catch.hpp"
#include "../opencl.cpp"
#include "atsc/randomize.h"

TEST_CASE("ATSC Randomize", "[randomize]") {

    atsc_opencl atsc;

    // One frame worth of mpeg input
    auto data = atsc_field_mpeg2();
    memset(data.data(), 0x55, data.size());

    // Use atsc_randomizer to generate validation data
    auto valid_data = atsc_field_data();
    auto randomizer = atsc_randomize<void>();
    randomizer.randomize_pkts(valid_data.data(), data.data());

    auto input = atsc.cl_alloc(data.size());
    auto output = atsc.cl_alloc(valid_data.size());
    auto final = atsc_field_data();

    atsc.to_device(input, data.data(), data.size());
    auto event = atsc.randomize(output, input);
    atsc.to_host(final.data(), output, final.size(), event);

    for (size_t i = 0; i < final.size(); i++) {
        REQUIRE(final[i] == valid_data[i]);
    }

    atsc.cl_free(input);
    atsc.cl_free(output);
}