#include "catch2/catch.hpp"
#include "../field_sync.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {
    atsc_field_sync<atsc_parameters> field_sync;

    // test even/odd fields
    auto input1 = random_vector_data<atsc_parameters::atsc_field_signal>();
    auto input2 = random_vector_data<atsc_parameters::atsc_field_signal>();
    for (size_t i = 0; i < atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT; i++) {
        (*input1)[i] = atsc_parameters::atsc_signal_type();
        (*input2)[i] = atsc_parameters::atsc_signal_type();
    }
    for (size_t i = 1; i < atsc_parameters::ATSC_SEGMENTS_PER_FIELD; i++) {
        (*input1)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 0] = atsc_parameters::atsc_signal_type();
        (*input1)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 1] = atsc_parameters::atsc_signal_type();
        (*input1)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 2] = atsc_parameters::atsc_signal_type();
        (*input1)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 3] = atsc_parameters::atsc_signal_type();
        (*input2)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 0] = atsc_parameters::atsc_signal_type();
        (*input2)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 1] = atsc_parameters::atsc_signal_type();
        (*input2)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 2] = atsc_parameters::atsc_signal_type();
        (*input2)[i * atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + 3] = atsc_parameters::atsc_signal_type();
    }
    save_vector_data("field_sync_input1.data", input1.get());
    save_vector_data("field_sync_input2.data", input2.get());

    field_sync.process_field(input1->data(), input2->data());
    field_sync.process_field(input2->data(), input1->data());

    save_vector_data("field_sync_output1.data", input1.get());
    save_vector_data("field_sync_output2.data", input2.get());    
}

TEST_CASE("ATSC Field Sync", "[field_sync]") {
    atsc_field_sync<atsc_parameters> field_sync;

    // test even/odd fields
    auto input1 = load_vector_data<atsc_parameters::atsc_field_signal>("field_sync_input1.data");
    auto input2 = load_vector_data<atsc_parameters::atsc_field_signal>("field_sync_input2.data");
    auto output1 = load_vector_data<atsc_parameters::atsc_field_signal>("field_sync_output1.data");
    auto output2 = load_vector_data<atsc_parameters::atsc_field_signal>("field_sync_output2.data");

    field_sync.process_field(input1->data(), input2->data());
    field_sync.process_field(input2->data(), input1->data());

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))

    for (size_t i = 0; i < output1->size(); i++) {
        REQUIRE(IS_CLOSE_CPLX((*output1)[i], (*input1)[i]));
        REQUIRE(IS_CLOSE_CPLX((*output2)[i], (*input2)[i]));
    }
}