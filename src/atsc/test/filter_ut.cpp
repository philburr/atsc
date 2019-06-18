#include "catch2/catch.hpp"
#include "../util.h"
#include "../filter.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {
    atsc_rrc_filter filter;
    using atsc_field_signal_padded = std::array<atsc_symbol_type, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT>;

    auto input = random_vector_data<aligned<atsc_field_signal_padded>>();
    auto output = std::make_unique<aligned<atsc_field_signal_padded>>();

    filter.process_field(output->data(), input->data());

    save_vector_data("filter_input.data", input.get());
    save_vector_data("filter_output.data", output.get());
}

TEST_CASE("ATSC Filter", "[filter]") {
    atsc_rrc_filter filter;
    using atsc_field_signal_padded = std::array<atsc_symbol_type, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT>;

    auto input = load_vector_data<aligned<atsc_field_signal_padded>>("filter_input.data");
    auto output = load_vector_data<atsc_field_signal_padded>("filter_output.data");
    auto test = std::make_unique<aligned<atsc_field_signal_padded>>();

    filter.process_field(test->data(), input->data());

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))

    for (size_t i = 0; i < output->size(); i++) {
        REQUIRE(IS_CLOSE_CPLX((*output)[i], (*test)[i]));
    }
}