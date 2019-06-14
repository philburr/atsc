#pragma once
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <array>

template<size_t CONTIGUOUS_TRELLIS_INPUT>
struct trellis_deinterleaver {
    constexpr trellis_deinterleaver() {}

private:
    static inline constexpr size_t len = ATSC_DATA_PER_FIELD;
    struct initializer {
        constexpr initializer() : table() {
            for (size_t index = 0; index < len; index++) {
                const uint32_t group_size = CONTIGUOUS_TRELLIS_INPUT * ATSC_TRELLIS_ENCODERS; 
                uint32_t group = index / group_size;
                uint32_t row = (index % group_size) / ATSC_TRELLIS_ENCODERS;
                uint32_t col = (index) % ATSC_TRELLIS_ENCODERS;
                uint32_t destination = group * group_size + col * CONTIGUOUS_TRELLIS_INPUT + row;

                table[index] = destination;
            }
        }
        std::array<uint16_t, len> table;
    };

public:
    static inline std::array<uint16_t, len> table = initializer().table;
};

template<size_t CONTIGUOUS_TRELLIS_INPUT>
struct trellis_interleaver {
    constexpr trellis_interleaver() {}

private:
    static inline constexpr size_t len = ATSC_DATA_PER_FIELD;
    struct initializer {
        initializer() : table() {
            for (size_t index = 0; index < len; index++) {
                const uint32_t group_size = CONTIGUOUS_TRELLIS_INPUT * ATSC_TRELLIS_ENCODERS; 
                uint32_t group = index / group_size;
                uint32_t row = (index % group_size) / ATSC_TRELLIS_ENCODERS;
                uint32_t col = (index) % ATSC_TRELLIS_ENCODERS;
                uint32_t destination = group * group_size + col * CONTIGUOUS_TRELLIS_INPUT + row;

                table[destination] = index;
            }
        }
        std::array<uint16_t, len> table;
    };

public:
    static inline std::array<uint16_t, len> table = initializer().table;
};

template<size_t CONTIGUOUS_TRELLIS_INPUT>
struct interleaver {
    constexpr interleaver() {}

private:
    struct initializer {
        static inline constexpr size_t len   = ATSC_DATA_PER_FIELD;
        static inline constexpr uint32_t trellis_input_size = CONTIGUOUS_TRELLIS_INPUT; // SOFTWARE_DECODE ? 13 : (ATSC_DATA_PER_FIELD / ATSC_TRELLIS_ENCODERS);
        static inline auto deinterleaver_table = trellis_deinterleaver<trellis_input_size>::table;

        uint32_t trellis_rotate(uint32_t index) {
            static std::array<uint32_t, ATSC_DATA_SEGMENTS * 3> trellis_rotate_points = []() {
                std::array<uint32_t, ATSC_DATA_SEGMENTS * 3> arr;
                for (unsigned i = 0; i < ATSC_DATA_SEGMENTS * 3; i++) {
                    arr[i] = ((ATSC_SEGMENT_FEC_BYTES * i + ATSC_TRELLIS_ENCODERS - 1) / ATSC_TRELLIS_ENCODERS) * ATSC_TRELLIS_ENCODERS;
                }
                return arr;
            }();
            static int shift = 0;
            static int next_shift_point = 1;

            if (index == trellis_rotate_points[next_shift_point]) {
                shift += 4;
                next_shift_point++;
                if (shift == ATSC_TRELLIS_ENCODERS) {
                    shift = 0;
                }
            }

            uint32_t trellis_group = index / ATSC_TRELLIS_ENCODERS;
            uint32_t trellis_index = (index + shift) % ATSC_TRELLIS_ENCODERS;
            uint32_t destination = trellis_group * ATSC_TRELLIS_ENCODERS + trellis_index;

            return destination;
        }

        uint32_t trellis_transpose(uint32_t index) {
            return deinterleaver_table[index % len] + len * (index / len);
        }

        /* constexpr is unfortunately slow here */
        initializer() : table(), map(), indices() {
            for (int i = 0; i < 52; i++) {
                for (int j = 0; j < 52*4; j++) {
                    map[i][j] = -1;
                }
                indices[i] = 0;
            }

            int line = 0;
            for (uint32_t input = 0, output = 0; input < len * 2; input++, output++) {

                // transpose
                uint32_t destination = output;
                destination = trellis_rotate(destination);
                destination = trellis_transpose(destination);

                // split into current/next field
                int field = 0;
                if (destination >= len) {
                    destination -= len;
                    field = 1;
                }

                if (line == 0) {
                    if (input < len) {
                        table[input][0] = field;
                        table[input][1] = (uint16_t)destination;
                    }
                } else {
                    int index = indices[line];
                    int r = map[line][index];
                    map[line][index] = input;

                    if (r >= 0 && (size_t)r < len) {
                        table[r][0] = field;
                        table[r][1] = (uint16_t)destination;
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
        std::array<std::array<uint16_t, 2>, len> table;
        std::array<std::array<int, 52*4>, 54> map;
        std::array<int, 52> indices;
    };

public:
    static inline std::array<std::array<uint16_t, 2>, initializer::len> table = initializer().table;

};

template<size_t CONTIGUOUS_TRELLIS_INPUT>
class interleaver_old {
public:
    constexpr interleaver_old() {}

private:
    struct table_initializer {
        static inline constexpr size_t len = ATSC_SEGMENT_FEC_BYTES * ATSC_DATA_SEGMENTS;
        static inline constexpr size_t contiguous_trellis_input = CONTIGUOUS_TRELLIS_INPUT;

        /* constexpr is unfortunately slow here */
        table_initializer() : table(), map(), indices() {
            for (int i = 0; i < 52; i++) {
                for (int j = 0; j < 52*4; j++) {
                    map[i][j] = -1;
                }
                indices[i] = 0;
            }

            std::array<uint32_t, ATSC_DATA_SEGMENTS * 3> trellis_rotate_points;
            for (unsigned i = 0; i < ATSC_DATA_SEGMENTS * 3; i++) {
                trellis_rotate_points[i] = ((ATSC_SEGMENT_FEC_BYTES * i + ATSC_TRELLIS_ENCODERS - 1) / ATSC_TRELLIS_ENCODERS) * ATSC_TRELLIS_ENCODERS;
            }
            uint32_t *next_boundary = &trellis_rotate_points[1];
            uint32_t shift = 0;

            int line = 0;
            for (uint32_t input = 0, output = 0; input < len * 2; input++, output++) {
                if (output == *next_boundary) {
                    next_boundary++;
                    shift += 4;
                    if (shift == ATSC_TRELLIS_ENCODERS) {
                        shift = 0;
                    }
                }

                // transpose
                size_t destination;
                constexpr uint32_t trellis_input_size = contiguous_trellis_input;
                const uint32_t group_size = trellis_input_size * ATSC_TRELLIS_ENCODERS; 
                uint32_t group = output / group_size;
                uint32_t row = (output % group_size) / ATSC_TRELLIS_ENCODERS;
                uint32_t col = (output + shift) % ATSC_TRELLIS_ENCODERS;
                destination = group * group_size + col * trellis_input_size + row;

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
    static inline std::array<uint32_t, table_initializer::len> table = table_initializer().table;

};
