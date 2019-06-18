#include "catch2/catch.hpp"
#include "../opencl.cpp"
#include "common/interleaver.h"
#include "atsc/interleaver.h"

TEST_CASE("ATSC Interleaver Refactor", "[interleaver,refactor]") {
    auto the_new = interleaver<ATSC_TRELLIS_INPUT_OPENCL>();
    auto the_old = interleaver_old<ATSC_TRELLIS_INPUT_OPENCL>();
    for (size_t i = 0; i < the_old.table.size(); i++) {
        auto field = the_new.table[i][0];
        auto index = the_new.table[i][1];
        auto val = (((uint32_t)field) << 16) | index;
        REQUIRE(val == the_old.table[i]);
    }
}

TEST_CASE("ATSC Interleaver SW vs HW", "[interleaver,refactor]") {
    auto the_old = interleaver<ATSC_TRELLIS_INPUT_SOFTWARE>();
    auto the_new = interleaver<ATSC_TRELLIS_INPUT_OPENCL>();

    auto the_old_table = the_old.table.data();
    auto the_new_table = the_new.table.data();
    REQUIRE(the_old.table.size() == the_new.table.size());

    for (unsigned index = 0; index < the_old.table.size(); index++) {
        REQUIRE(the_old_table[index][0] == the_new_table[index][0]);

        auto the_old_destination = the_old_table[index][1];
        auto the_new_destination = the_new_table[index][1];

        // Destination is an array of [414][12][13]
        auto block = the_old_destination / (12 * 13);
        auto the_old_trellis = (the_old_destination % (12 * 13)) / 13;
        auto the_old_index = ((the_old_destination % (12 * 13)) % 13) + block * 13;

        auto the_new_trellis = the_new_destination / 5382;
        auto the_new_index = the_new_destination % 5382;

        REQUIRE(the_new_trellis == the_old_trellis);
        REQUIRE(the_new_index == the_old_index); 
    }

}

TEST_CASE("ATSC Interleaver", "[interleaver,opencl]") {

    atsc_opencl atsc;
    auto intr_table = interleaver<ATSC_TRELLIS_INPUT_OPENCL>().table;
    (void)intr_table;
    for (unsigned i = 0; i < intr_table.size(); i++) {
        if (intr_table[i][0] == 0 && intr_table[i][1] == 0) {
            printf("%d\n", i);
        }
    }

    auto data = atsc_field_data();
    for (unsigned seg = 0; seg < data.size(); seg++) {
        data[seg] = seg;
    }

    auto valid_frame1 = atsc_field_data();
    auto valid_frame2 = atsc_field_data();
    memset(valid_frame1.data(), 0, valid_frame1.size());
    memset(valid_frame2.data(), 0, valid_frame2.size());

    auto interleaver = atsc_interleaver<false>();
    auto ref_table = interleaver.table_.data(); (void)ref_table;
    interleaver.process(valid_frame1.data(), valid_frame2.data(), data.data());

    auto input = atsc.cl_alloc(data.size());
    atsc.to_device(input, data.data(), data.size());
    auto new_table = atsc._interleaver_table.table.data(); (void)new_table;

    auto output_frame1 = atsc_field_data();
    auto output_frame2 = atsc_field_data();
    memset(output_frame1.data(), 0, output_frame1.size());
    memset(output_frame2.data(), 0, output_frame2.size());

    auto output1 = atsc.cl_alloc(output_frame1.size());
    auto output2 = atsc.cl_alloc(output_frame2.size());
    atsc.to_device(output1, output_frame1.data(), output_frame1.size());
    atsc.to_device(output2, output_frame2.data(), output_frame2.size());
    auto event = atsc.interleave(output1, output2, input);
    atsc.to_host(output_frame1.data(), output1, output_frame1.size(), event);
    atsc.to_host(output_frame2.data(), output2, output_frame1.size());

    for (size_t i = 0; i < valid_frame1.size(); i++) {
        REQUIRE(output_frame1[i] == valid_frame1[i]);
    }
    for (size_t i = 0; i < valid_frame2.size(); i++) {
        REQUIRE(output_frame2[i] == valid_frame2[i]);
    }
}
