#pragma once

#include<cstdint>

template<typename T>
using unique_freeable_ptr = std::unique_ptr<T,std::function<void(T*)>>;

template <typename T, size_t N> struct alignas(32) aligned_array : public std::array<T,N> {
    aligned_array(const std::array<T,N>& o) : std::array<T,N>(o) {}
};
template<typename T> struct array_params;
template<typename T, size_t N> struct array_params<std::array<T, N>> {
    using type = T;
    static constexpr size_t size = N;
};

template<typename T>
struct alignas(32) aligned : public std::array<typename array_params<T>::type, array_params<T>::size> {};

namespace std {
    template<std::size_t I, class T, std::size_t N>
    struct tuple_element<I, aligned_array<T,N> >
    {
        using type = T;
    };    
    template<std::size_t I, class T, std::size_t N>
    struct tuple_element<I, aligned<std::array<T,N>> >
    {
        using type = T;
    };
    template< class T, size_t N >
    class tuple_size< aligned_array<T, N> > : public integral_constant<size_t, N>
    { };
    template< class T, size_t N >
    class tuple_size< aligned<std::array<T, N> > > : public integral_constant<size_t, N>
    { };
}

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

template <typename T, unsigned B>
inline T signextend(const T x)
{
  struct {T x:B;} s;
  return s.x = x;
}

struct reverse_table_initializer {
    constexpr reverse_table_initializer() : table() {
        for (int i = 0; i < 256; i++) {
            table[i] = ((i * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;;
        }
    }

    std::array<uint8_t, 256> table;
};

template<typename T>
struct null_xform {
    T xform(T v) { return v; }
};

}

#define BYTEN(v, b) (((v) >> (b*8)) & 0xff)
#define BYTE0(v) BYTEN(v, 0)
#define BYTE1(v) BYTEN(v, 1)
#define BYTE2(v) BYTEN(v, 2)
#define BYTE3(v) BYTEN(v, 3)
#define BYTE4(v) BYTEN(v, 4)
#define BYTE5(v) BYTEN(v, 5)
#define BYTE6(v) BYTEN(v, 6)
#define BYTE7(v) BYTEN(v, 7)
