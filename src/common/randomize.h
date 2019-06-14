#pragma once
#pragma once
#include <cassert>
#include <cstring>
#include "common/atsc_parameters.h"

template<typename PARAMETERS>
class randomize_table {
public:
    constexpr randomize_table() {}
    static constexpr unsigned table_size = PARAMETERS::ATSC_SEGMENT_BYTES * PARAMETERS::ATSC_DATA_SEGMENTS;

private:
    struct initializer {
        static constexpr uint16_t initial_state = 0xf180;
        static constexpr unsigned len = PARAMETERS::ATSC_SEGMENT_BYTES * PARAMETERS::ATSC_DATA_SEGMENTS;
        static constexpr uint32_t generator = 0x9c65;   // x16 x13 x12 x11 x7 x6 x3 x1

        constexpr initializer() : table() {
            uint32_t state = initial_state;
            for (size_t i = 0; i < len; i++) {
                uint16_t output = 0;
                output  = (state & 0x3c00) >> 6;
                output |= (state & 0x0040) >> 3;
                output |= (state & 0x000c) >> 1;
                output |= (state & 0x0001) >> 0;
                assert(output <= UINT8_MAX);

                table[i] = output;

                // advance state
                state <<= 1;
                if (state & 0x10000) {
                    state ^= (generator << 1) | 1;
                }
            }
        }
        std::array<uint8_t, len> table;
    };

public:
    static inline constexpr std::array<uint8_t, initializer::len> table = initializer().table;
};


