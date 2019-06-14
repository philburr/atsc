#pragma once
#include "interleaver.h"

template<size_t CONTIGUOUS_TRELLIS_INPUT>
struct trellis_output_commutator {
    constexpr trellis_output_commutator() {}

private:
    static inline constexpr size_t len = ATSC_DATA_SYMBOLS_PER_SEGMENT * ATSC_DATA_SEGMENTS;
    struct initializer {

        static constexpr unsigned symbols_per_trellis = ATSC_SYMBOLS_PER_BYTE * CONTIGUOUS_TRELLIS_INPUT;

        initializer() : table() {

            for (size_t index = 0; index < len; index++) {
                unsigned dseg = index / ATSC_DATA_SYMBOLS_PER_SEGMENT;
                unsigned dseg_offset = index % ATSC_DATA_SYMBOLS_PER_SEGMENT;

                unsigned start_trellis = (dseg * 4) % ATSC_TRELLIS_ENCODERS;
                unsigned trellis = (start_trellis + index) % ATSC_TRELLIS_ENCODERS;
                unsigned trellis_index = (index / ATSC_TRELLIS_ENCODERS) % symbols_per_trellis;

                // The trellis output data is organized as 52 symbols per trellis
                unsigned chunk = index / (symbols_per_trellis * ATSC_TRELLIS_ENCODERS);
                unsigned offset = chunk * symbols_per_trellis * ATSC_TRELLIS_ENCODERS + trellis * symbols_per_trellis + trellis_index;

                // index is ordered output of the trellis output commutator
                // insert space for sync 
                unsigned adjusted_index = (dseg + 1) * ATSC_SYMBOLS_PER_SEGMENT + 4 + dseg_offset;

                table[offset] = adjusted_index;
            }
        }

        std::array<uint32_t, len> table;
    };

public:
    std::array<uint32_t, len> table = initializer().table;
};
