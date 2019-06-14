#include "catch2/catch.hpp"
#include "../util.h"
#include "../offset.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {
    atsc_offset tuner;

    auto data = random_vector_data<aligned<atsc_field_symbol_padded>>();
    save_vector_data("tuner_input.data", data.get());

    tuner.process_field(*data);

    save_vector_data("tuner_output.data", data.get());
}

TEST_CASE("ATSC Tuner", "[tuner]") {
    atsc_offset tuner;

    auto data = load_vector_data<aligned<atsc_field_symbol_padded>>("tuner_input.data");
    auto valid = load_vector_data<atsc_field_symbol>("tuner_output.data");

    tuner.process_field(*data);

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))

    for (size_t i = 0; i < valid->size(); i++) {
        REQUIRE(IS_CLOSE_CPLX((*valid)[i], (*data)[i]));
    }
}