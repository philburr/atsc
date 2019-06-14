#pragma once

#include<cstdint>
#include "common/bits.h"

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
