#pragma once
#include <cassert>
#include "common/atsc_parameters.h"


template<typename PARAMETERS, bool SOFTWARE_DECODE>
class atsc_interleaver {
public:
    constexpr atsc_interleaver() {}

    void process(uint8_t* current_field, uint8_t* next_field, uint8_t* input) {
        uint8_t* fields[2] = {current_field, next_field};
        for (size_t i = 0; i < PARAMETERS::ATSC_SEGMENT_FEC_BYTES * PARAMETERS::ATSC_DATA_SEGMENTS; i++) {
            auto destination = table_[i];
            auto field = destination >> 16;
            auto position = destination & 65535;
            fields[field][position] = *input++;
        }
    }

private:
    struct table_initializer {
        static inline constexpr size_t len = PARAMETERS::ATSC_SEGMENT_FEC_BYTES * PARAMETERS::ATSC_DATA_SEGMENTS;
        static inline constexpr size_t contiguous_trellis_input = 13;

        /* constexpr is unfortunately slow here */
        table_initializer() : table(), map(), indices() {
            for (int i = 0; i < 52; i++) {
                for (int j = 0; j < 52*4; j++) {
                    map[i][j] = -1;
                }
                indices[i] = 0;
            }

            std::array<uint32_t, PARAMETERS::ATSC_DATA_SEGMENTS * 3> trellis_shift_points;
            for (unsigned i = 0; i < PARAMETERS::ATSC_DATA_SEGMENTS * 3; i++) {
                trellis_shift_points[i] = ((PARAMETERS::ATSC_SEGMENT_FEC_BYTES * i + PARAMETERS::ATSC_TRELLIS_ENCODERS - 1) / PARAMETERS::ATSC_TRELLIS_ENCODERS) * PARAMETERS::ATSC_TRELLIS_ENCODERS;
            }
            uint32_t *next_boundary = &trellis_shift_points[1];
            uint32_t shift = 0;

            int line = 0;
            for (uint32_t input = 0, output = 0; input < len * 2; input++, output++) {
                if (output == *next_boundary) {
                    next_boundary++;
                    shift += 4;
                    if (shift == PARAMETERS::ATSC_TRELLIS_ENCODERS) {
                        shift = 0;
                    }
                }

                // transpose
                size_t destination;
                if (SOFTWARE_DECODE) {
                    constexpr uint32_t trellis_input_size = 13;
                    const uint32_t group_size = trellis_input_size * PARAMETERS::ATSC_TRELLIS_ENCODERS; 
                    uint32_t group = output / group_size;
                    uint32_t row = (output % group_size) / PARAMETERS::ATSC_TRELLIS_ENCODERS;
                    uint32_t col = (output + shift) % PARAMETERS::ATSC_TRELLIS_ENCODERS;
                    destination = group * group_size + col * trellis_input_size + row;
                } else {
                    constexpr uint32_t trellis_input_size = PARAMETERS::ATSC_DATA_PER_FIELD / PARAMETERS::ATSC_TRELLIS_ENCODERS;
                    const uint32_t group_size = trellis_input_size * PARAMETERS::ATSC_TRELLIS_ENCODERS; 
                    uint32_t group = output / group_size;
                    uint32_t row = (output % group_size) / PARAMETERS::ATSC_TRELLIS_ENCODERS;
                    uint32_t col = (output + shift) % PARAMETERS::ATSC_TRELLIS_ENCODERS;
                    destination = group * group_size + col * trellis_input_size + row;
                }

                // split into current/next field
                if (destination >= len) {
                    destination -= len;
                    destination += 65536;
                }

                if (line == 0) {
                    if (input < len) {
                        table[input] = destination;
                    }
                } else {
                    int index = indices[line];
                    int r = map[line][index];
                    map[line][index] = input;

                    if (r >= 0 && (size_t)r < len) {
                        table[r] = destination;
                    }

                    index++;
                    if (index >= 4*line) {
                        index = 0;
                    }
                    indices[line] = index;
                }

                line++;
                if (line == 52) {
                    line = 0;
                }
                if (input == len - 1) {
                    line = 0;
                }
            }
        }
        std::array<uint32_t, len> table;
        std::array<std::array<int, 52*4>, 54> map;
        std::array<int, 52> indices;
    };

public:
    static inline std::array<uint32_t, table_initializer::len> table_ = table_initializer().table;

};
