#pragma once
#include <array>
#include <cstdint>

template<unsigned INITIAL_STATE, unsigned SHIFT_OUT, unsigned MASK, unsigned BITS>
struct lfsr {
private:
    struct lfsr_generator {

        constexpr lfsr_generator() : table() {
            unsigned state = INITIAL_STATE;
            unsigned out_shift = SHIFT_OUT;
            unsigned mask = MASK;

            for (unsigned i = 0; i < BITS; i++) {
                table[i] = (state >> out_shift) & 1;

                unsigned update = state & mask;

                update ^= update >> 16;
                update ^= update >> 8;
                update ^= update >> 4;
                update &= 0xf;
                update = (0x6996 >> update) & 1;
                state = (state << 1) | update;
            }
        }

        std::array<uint8_t, BITS> table;
    };

public:
    static inline constexpr std::array<uint8_t, BITS> table = lfsr_generator().table;
};

