#pragma once

template<typename T>
struct differential_table {
    constexpr differential_table() {}

private:
    struct initializer {
        constexpr initializer() : table() {
            for (int i = 0; i < 256; i++) {
                table[i] = diff_encode(nibble_swap((uint8_t)i));
                table2[i] = diff_encode(half_nibble_swap((uint8_t)i));
            }
        }

        constexpr uint8_t diff_encode(uint8_t v) {
            // The incoming carry bit will be accounted for
            // elsewhere.
            uint8_t carry = 0;
            uint8_t encoded = 0;
            for (int j = 0; j < 8; j++) {
                carry = (carry ^ v) & 1;
                v >>= 1;
                encoded >>= 1;
                encoded |= carry << 7;
            }
            return encoded;
        }

        // In ATSC, each byte is fed into a trellis encoder and is
        // consumed in MSB first order.  The byte is further decompsed
        // into some differential encoders (for Z2, every other bit)
        // and since the unused bits are already removed, each nibble
        // in the input represents one input byte into the trellis decoder
        // and since we need to convert from BE to LE, we need to bit
        // swap each nibble.
        constexpr uint8_t nibble_swap(uint8_t v) {
            return ((v & 0x88) >> 3) |
                   ((v & 0x44) >> 1) |
                   ((v & 0x22) << 1) |
                   ((v & 0x11) << 3);
        }

        // For Z0(a/b)
        constexpr uint8_t half_nibble_swap(uint8_t v) {
            return ((v & 0xaa) >> 1) |
                   ((v & 0x55) << 1);
        }

        std::array<int8_t, 256> table;
        std::array<int8_t, 256> table2;
    };

public:
    static inline std::array<int8_t, 256> table = initializer().table;
    static inline std::array<int8_t, 256> table2 = initializer().table2;
};