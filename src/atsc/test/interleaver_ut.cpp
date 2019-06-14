#include "catch2/catch.hpp"
#include "../interleaver.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {
    atsc_interleaver<atsc_parameters> interleave;

    auto input = random_vector_data<atsc_parameters::atsc_field_data>();
    auto output1 = std::make_unique<atsc_parameters::atsc_field_data>();
    auto output2 = std::make_unique<atsc_parameters::atsc_field_data>();
    memset(output1->data(), 0, sizeof((*output1)[0]) * output1->size());
    memset(output2->data(), 0, sizeof((*output2)[0]) * output2->size());

    interleave.process(output1->data(), output2->data(), input->data());

    save_vector_data("interleaver_input.data", input.get());
    save_vector_data("interleaver_output1.data", output1.get());
    save_vector_data("interleaver_output2.data", output2.get());    
}

TEST_CASE("ATSC Interleaver", "[interleave]") {

    atsc_interleaver<atsc_parameters> interleave;

    auto input = load_vector_data<atsc_parameters::atsc_field_data>("interleaver_input.data");
    auto output1 = load_vector_data<atsc_parameters::atsc_field_data>("interleaver_output1.data");
    auto output2 = load_vector_data<atsc_parameters::atsc_field_data>("interleaver_output2.data");
    auto test1 = std::make_unique<atsc_parameters::atsc_field_data>();
    auto test2 = std::make_unique<atsc_parameters::atsc_field_data>();
    memset(test1->data(), 0, sizeof((*test1)[0]) * test1->size());
    memset(test2->data(), 0, sizeof((*test2)[0]) * test2->size());
    

    interleave.process(test1->data(), test2->data(), input->data());

    for (size_t i = 0; i < output1->size(); i++) {
        REQUIRE((*output1)[i] == (*test1)[i]);
        REQUIRE((*output2)[i] == (*test2)[i]);
    }
}