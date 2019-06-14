#pragma once
#include "bits.h"

template<size_t MODULO>
struct modulo_table {
    static constexpr int modulo = MODULO;
    using type = typename detail::bit_container<detail::round_power_2(detail::log2(modulo))>::type;

private:
    struct initializer {
        constexpr initializer() : arr() {
            for (unsigned i = 0; i < arr.size(); i++) {
                arr[i] = i % modulo;
            }
        }
        std::array<type, 2 * MODULO> arr;
    };

public:
    static inline constexpr std::array<type, 2 * MODULO> table = initializer().arr;

};

template<size_t FIELD, size_t GENERATOR>
struct galois_tables {
    using type = typename detail::bit_container<detail::round_power_2(FIELD)>::type;
    static constexpr size_t max = size_t(1) << FIELD;
    static constexpr type modulo = max - 1;
    static constexpr type zero = type(0);
    static constexpr type infinity = type(modulo);

private:
    struct initializer {

        constexpr initializer() : log(), exp() {
            log[0] = infinity;
            exp[2*modulo] = zero;

            size_t v = 1;
            for (size_t i = 0; i < modulo; i++) {
                log[v] = i;
                exp[i] = v;
                log[v + modulo] = i;
                exp[i + modulo] = v;

                v <<= 1;
                if (v & (1 << FIELD)) {
                    v ^= GENERATOR;
                }
                v &= max - 1;
            }
        }

        std::array<type, 2*modulo + 1> log;
        std::array<type, 2*modulo + 1> exp;
    };    

public:
    static inline constexpr std::array<type, 2*modulo + 1> log = initializer().log;
    static inline constexpr std::array<type, 2*modulo + 1> exp = initializer().exp;
};