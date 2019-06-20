#include "catch2/catch.hpp"
#include "../opencl.cpp"
#include "atsc/trellis.h"

TEST_CASE("ATSC Trellis OpenCL", "[trellis,opencl]") {

    atsc_opencl atsc;

    auto data = atsc_field_data();
    for (unsigned i = 0; i < data.size(); i++) {
        if (i == 0) 
            data[i] = 0x36;
        else
            data[i] = i;
    }

    auto atsc_data = atsc_field_data();
    auto deinterleaver = trellis_interleaver<ATSC_TRELLIS_INPUT_OPENCL>::table;
    auto interleaver = trellis_deinterleaver<ATSC_TRELLIS_INPUT_SOFTWARE>::table;

    for (unsigned i = 0; i < data.size(); i++) {
        auto index = deinterleaver[i];
        index = interleaver[index];

        if (i == 0)
            atsc_data[index] = 0x36;
        else
            atsc_data[index] = i;
    }

    auto valid_data = atsc_field_symbol_padded();

    auto trellis = atsc_trellis_encoder();
    trellis.process(valid_data, atsc_data);

    auto input = atsc.cl_alloc_arr<atsc_field_data>();
    atsc.to_device(input.data(), data.data(), data.size());

    auto output_data = atsc_field_symbol();
    auto output = atsc.cl_alloc_arr<atsc_field_symbol_padded>();
    cl_event event = atsc.trellis(output, input);
    atsc.to_host(output_data.data(), output.data(), output_data.size() * sizeof(atsc_symbol_type), event);

    for (size_t i = 0; i < valid_data.size(); i++) {
        REQUIRE(valid_data[i] == output_data[i]);
    }

    auto valid_carry = trellis.get_carry();
    auto carry = atsc.get_carry();
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 12; j++) {
            REQUIRE(valid_carry[i][j] == carry[i][j]);
        }
    }

}
