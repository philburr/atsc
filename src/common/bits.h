#pragma once

namespace detail {

template<typename T>
struct bit_size {
    enum { size = 8 * sizeof(T) };
};

template<size_t N>
struct bit_container {
};

template<>
struct bit_container<8> {
    using type = uint8_t;
};

template<>
struct bit_container<16> {
    using type = uint16_t;
};

template<>
struct bit_container<32> {
    using type = uint32_t;
};

template<>
struct bit_container<64> {
    using type = uint64_t;
};

constexpr int round_power_2(uint64_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return (int)v;
}

constexpr int log2(uint64_t v) {
    int r = 0;
    while (v) {
        r += 1;
        v >>= 1;
    }
    return r;
}

template <typename T, unsigned B>
inline T signextend(const T x)
{
  struct {T x:B;} s;
  return s.x = x;
}

}