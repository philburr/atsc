#include "catch2/catch.hpp"
#include "../opencl.cpp"
#include "atsc/field_sync.h"
#include <random>
#include <algorithm>

TEST_CASE("ATSC Field Sync", "[opencl,field_sync]") {
    atsc_opencl atsc;

    using signal = std::vector<atsc_symbol_type>;

    auto eng = std::mt19937{0};
    auto rnd = std::uniform_real_distribution<float>(-1, 1); 

    auto field = signal(ATSC_SYMBOLS_PER_FIELD);
    for (size_t i = 0; i < field.size(); i++) {
        if (i < ATSC_SYMBOLS_PER_SEGMENT ||
            (i % ATSC_SYMBOLS_PER_SEGMENT) < 4) {
            field[i] = atsc_symbol_type(0, 0);
        } else {
            field[i] = atsc_symbol_type(rnd(eng), rnd(eng));
        }
    }
    auto saved_symbols = std::array<atsc_symbol_type, ATSC_RESERVED_SYMBOLS>();
    for (size_t i = 0; i < saved_symbols.size(); i++) {
        saved_symbols[i] = atsc_symbol_type(0, 0);
    }

    // Device
    auto input = atsc.cl_alloc(field.size() * sizeof(field[0]));
    atsc.to_device(input, field.data(), field.size() * sizeof(field[0]));

    auto event = atsc.field_sync(input);

    auto test_data = signal(ATSC_SYMBOLS_PER_FIELD);
    atsc.to_host(test_data.data(), input, test_data.size() * sizeof(test_data[0]), event);
    atsc.cl_free(input);

    // Host
    auto valid_data = signal(ATSC_SYMBOLS_PER_FIELD);
    for (size_t i = 0; i < field.size(); i++) {
        valid_data[i] = field[i];
    }

    auto host_fs = atsc_field_sync<void>();
    host_fs.process_field(valid_data.data(), saved_symbols.data());

    #define EPSILON 0.000001f
    #define IS_CLOSE(a, b) (fabsf((a) - (b)) < EPSILON)
    #define IS_CLOSE_CPLX(a, b) (IS_CLOSE(a.real(), b.real()) && IS_CLOSE(a.imag(), b.imag()))
    for (size_t i = 0; i < ATSC_SYMBOLS_PER_FIELD; i++) {
        REQUIRE(IS_CLOSE_CPLX(valid_data[i], test_data[i]));
    }

    // One more time to make certain that the reserved symbols are being propagated..
    for (size_t i = 0; i < saved_symbols.size(); i++) {
        saved_symbols[i] = field[ATSC_SYMBOLS_PER_FIELD - ATSC_RESERVED_SYMBOLS + i];
    }
    for (size_t i = 0; i < field.size(); i++) {
        if (i < ATSC_SYMBOLS_PER_SEGMENT ||
            (i % ATSC_SYMBOLS_PER_SEGMENT) < 4 ||
            i < ATSC_SYMBOLS_PER_FIELD - ATSC_RESERVED_SYMBOLS) {
            field[i] = atsc_symbol_type(0, 0);
        } else {
            field[i] = atsc_symbol_type(rnd(eng), rnd(eng));
        }
    }

    // Device
    input = atsc.cl_alloc(field.size() * sizeof(field[0]));
    atsc.to_device(input, field.data(), field.size() * sizeof(field[0]));

    event = atsc.field_sync(input);

    atsc.to_host(test_data.data(), input, test_data.size() * sizeof(test_data[0]), event);
    atsc.cl_free(input);

    // Host
    for (size_t i = 0; i < field.size(); i++) {
        valid_data[i] = field[i];
    }
    host_fs.process_field(valid_data.data(), saved_symbols.data());

    for (size_t i = 0; i < ATSC_SYMBOLS_PER_FIELD; i++) {
        if (!IS_CLOSE_CPLX(valid_data[i], test_data[i]))
        REQUIRE(IS_CLOSE_CPLX(valid_data[i], test_data[i]));
    }
}