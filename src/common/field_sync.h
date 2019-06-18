#pragma once
#include "lfsr.h"

template<typename T>
struct field_sync_table {

private:
    struct initializer {
        constexpr static inline size_t len = ATSC_SYMBOLS_PER_SEGMENT - ATSC_RESERVED_SYMBOLS - ATSC_SYMBOLS_PER_BYTE;

        constexpr initializer(bool even, std::array<uint8_t, 511> pn511, std::array<uint8_t, 63> pn63) : table() {

            size_t sym = 0;
            for (size_t i = 0; i < pn511.size(); i++, sym++) {
                table[sym] = pn511[i] ? 6 : 1;
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = pn63[i] ? 6 : 1;
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = (!!pn63[i] == even) ? 6 : 1;
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = pn63[i] ? 6 : 1;
            }

            std::array<uint8_t, 24> vsb_mode = {0,0,0,0, 1,0,1,0, 0,1,0,1, 1,1,1,1, 0,1,0,1, 1,0,1,0};
            for (size_t i = 0; i < vsb_mode.size(); i++, sym++) {
                table[sym] = vsb_mode[i] ? 6 : 1;
            }

            for (size_t i = 0; i < 104 - ATSC_RESERVED_SYMBOLS; i++, sym++) {
                table[sym] = pn63[i % pn63.size()] ? 6 : 1;
            }
        }

        std::array<uint8_t, len> table;
    };

public:
    static inline constexpr std::array<uint8_t, initializer::len> field_sync_even = initializer(
            true,
            lfsr<0b10, 8, 0b110110110, 511>::table,
            lfsr<0b111001, 5, 0b110000, 63>::table
        ).table;
    static inline constexpr std::array<uint8_t, initializer::len> field_sync_odd = initializer(
            false,
            lfsr<0b10, 8, 0b110110110, 511>::table,
            lfsr<0b111001, 5, 0b110000, 63>::table
        ).table;


};