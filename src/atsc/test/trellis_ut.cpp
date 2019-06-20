#include "catch2/catch.hpp"
#include "../trellis.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {
    atsc_trellis_encoder trellis;

    auto input1 = random_vector_data<atsc_field_data>();
    auto input2 = random_vector_data<atsc_field_data>();

    auto output1 = std::make_unique<atsc_field_symbol_padded>();
    auto output2 = std::make_unique<atsc_field_symbol_padded>();

    trellis.process(*output1, *input1);
    trellis.process(*output2, *input2);

    save_vector_data("trellis_input1.data", input1.get());
    save_vector_data("trellis_input2.data", input1.get());
    save_vector_data("trellis_output1.data", output1.get());
    save_vector_data("trellis_output2.data", output2.get());
}

TEST_CASE("ATSC Trellis Encoder", "[trellis]") {
    atsc_trellis_encoder trellis;

    auto input1 = load_vector_data<atsc_field_data>("trellis_input1.data");
    auto input2 = load_vector_data<atsc_field_data>("trellis_input2.data");

    auto output1 = load_vector_data<atsc_field_symbol_padded>("trellis_output1.data");
    auto output2 = load_vector_data<atsc_field_symbol_padded>("trellis_output2.data");

    auto test1 = std::make_unique<atsc_field_symbol_padded>();
    auto test2 = std::make_unique<atsc_field_symbol_padded>();

    trellis.process(*test1, *input1);
    trellis.process(*test2, *input2);

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))

    for (size_t i = 0; i < input1->size(); i++) {
        REQUIRE(IS_CLOSE_CPLX((*output1)[i], (*test1)[i]));
        REQUIRE(IS_CLOSE_CPLX((*output2)[i], (*test2)[i]));
    }
}