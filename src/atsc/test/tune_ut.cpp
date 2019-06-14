#include "catch2/catch.hpp"
#include "../util.h"
#include "../offset.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {
    atsc_offset<atsc_parameters> tuner;

    auto data = random_vector_data<aligned<atsc_parameters::atsc_field_signal>>();
    save_vector_data("tuner_input.data", data.get());

    tuner.process_field(data->data());

    save_vector_data("tuner_output.data", data.get());
}

TEST_CASE("ATSC Tuner", "[tuner]") {
    atsc_offset<atsc_parameters> tuner;

    auto data = load_vector_data<aligned<atsc_parameters::atsc_field_signal>>("tuner_input.data");
    auto valid = load_vector_data<atsc_parameters::atsc_field_signal>("tuner_output.data");

    tuner.process_field(data->data());

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))

    for (size_t i = 0; i < valid->size(); i++) {
        REQUIRE(IS_CLOSE_CPLX((*valid)[i], (*data)[i]));
    }
}