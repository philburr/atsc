#include "catch2/catch.hpp"
#include "../randomize.h"
#include "vector_data.h"

void generate_test_data() {

    atsc_randomize<void> randomize;

    auto input = random_vector_data<atsc_field_mpeg2>();
    auto output = std::make_unique<atsc_field_data>();

    randomize.randomize_pkts(output->data(), input->data());

    save_vector_data("randomize_input.data", input.get());
    save_vector_data("randomize_output.data", output.get());

}

TEST_CASE("ATSC Randomize", "[randomize]") {

    atsc_randomize<void> randomize;

    auto input = load_vector_data<atsc_field_mpeg2>("randomize_input.data");
    auto output = load_vector_data<atsc_field_data>("randomize_output.data");
    auto test = std::make_unique<atsc_field_data>();

    randomize.randomize_pkts(test->data(), input->data());

    for (size_t i = 0; i < output->size(); i++) {
        REQUIRE((*output)[i] == (*test)[i]);
    }
}