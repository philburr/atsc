#include "catch2/catch.hpp"
#include "../field_sync.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {
    atsc_field_sync<void> field_sync;

    // test even/odd fields
    auto input1 = random_vector_data<atsc_field_symbol_padded>();
    auto input2 = random_vector_data<atsc_field_symbol_padded>();
    for (size_t i = 0; i < ATSC_SYMBOLS_PER_SEGMENT; i++) {
        (*input1)[i] = atsc_symbol_type();
        (*input2)[i] = atsc_symbol_type();
        (*input1)[i + ATSC_SYMBOLS_PER_FIELD] = atsc_symbol_type();
        (*input2)[i + ATSC_SYMBOLS_PER_FIELD] = atsc_symbol_type();
    }
    for (size_t i = 1; i < ATSC_SEGMENTS_PER_FIELD; i++) {
        (*input1)[i * ATSC_SYMBOLS_PER_SEGMENT + 0] = atsc_symbol_type();
        (*input1)[i * ATSC_SYMBOLS_PER_SEGMENT + 1] = atsc_symbol_type();
        (*input1)[i * ATSC_SYMBOLS_PER_SEGMENT + 2] = atsc_symbol_type();
        (*input1)[i * ATSC_SYMBOLS_PER_SEGMENT + 3] = atsc_symbol_type();
        (*input2)[i * ATSC_SYMBOLS_PER_SEGMENT + 0] = atsc_symbol_type();
        (*input2)[i * ATSC_SYMBOLS_PER_SEGMENT + 1] = atsc_symbol_type();
        (*input2)[i * ATSC_SYMBOLS_PER_SEGMENT + 2] = atsc_symbol_type();
        (*input2)[i * ATSC_SYMBOLS_PER_SEGMENT + 3] = atsc_symbol_type();
    }
    save_vector_data("field_sync_input1.data", input1.get());
    save_vector_data("field_sync_input2.data", input2.get());

    atsc_reserved_symbols res1;
    atsc_reserved_symbols res2;
    for (size_t i = 0; i < res1.size(); i++) {
        res1[i] = 0x55;
        res2[i] = 0xaa;
    }

    field_sync.process_field(*input1, res1);
    field_sync.process_field(*input2, res2);

    save_vector_data("field_sync_output1.data", input1.get());
    save_vector_data("field_sync_output2.data", input2.get());    
}

TEST_CASE("ATSC Field Sync", "[field_sync]") {
    atsc_field_sync<void> field_sync;

    // test even/odd fields
    auto input1 = load_vector_data<atsc_field_symbol_padded>("field_sync_input1.data");
    auto input2 = load_vector_data<atsc_field_symbol_padded>("field_sync_input2.data");
    auto output1 = load_vector_data<atsc_field_symbol_padded>("field_sync_output1.data");
    auto output2 = load_vector_data<atsc_field_symbol_padded>("field_sync_output2.data");

    atsc_reserved_symbols res1;
    atsc_reserved_symbols res2;
    for (size_t i = 0; i < res1.size(); i++) {
        res1[i] = 0x55;
        res2[i] = 0xaa;
    }

    field_sync.process_field(*input1, res1);
    field_sync.process_field(*input2, res2);

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))

    for (size_t i = 0; i < output1->size(); i++) {
        REQUIRE(IS_CLOSE_CPLX((*output1)[i], (*input1)[i]));
        REQUIRE(IS_CLOSE_CPLX((*output2)[i], (*input2)[i]));
    }
}