#pragma once
#include <cassert>
#include <array>
#include "util.h"

template<typename TYPE, size_t MOD>
class abelian_group {
public:
    static constexpr int modulo = MOD;
    using type = TYPE;

    static constexpr type add(const type left, const type right) {
        if (left == modulo || right == modulo) return modulo;
        return modtable[left + right];
    }
    static constexpr type sub(const type left, const type right) {
        if (left == modulo || right == modulo) return modulo;
        return modtable[modulo + left - right];
    }
    static constexpr type neg(const type right) {
        if (right == modulo) return modulo;
        return modtable[modulo - right];
    }

private:
    struct mod_initializer {
        constexpr mod_initializer() : arr() {
            for (unsigned i = 0; i < arr.size(); i++) {
                arr[i] = i % modulo;
            }
        }
        std::array<type, 2 << detail::bit_size<type>::size> arr;
    };
    static inline constexpr std::array<type, 2 << detail::bit_size<type>::size> modtable = mod_initializer().arr;
};

template<size_t FIELD, size_t GENERATOR>
class galois_field {
public:
    static constexpr size_t field = FIELD;
    static constexpr size_t gen = GENERATOR;

    using type = typename detail::bit_container<detail::round_power_2(field)>::type;
    using large_type = typename detail::bit_container<detail::round_power_2(field+1)>::type;

    static constexpr large_type generator = GENERATOR;
    static constexpr large_type max = large_type(1) << field;
    static constexpr type modulo = max - 1;
    static constexpr type zero = type(0);
    static constexpr type infinity = type(modulo);
    static_assert((generator >> field) == 1, "Invalid generator");

    static constexpr type exp(type v) {
        return _exp[modulo + v];
    }
    static constexpr type log(type v) {
        return _log[v];
    }

    static constexpr type __exp(large_type v) {
        return _exp[v];
    }
    static constexpr type __log(large_type v) {
        return _log[v];
    }

    static constexpr type mul_slow(size_t x, size_t y) {
        type r = 0;
        while (y) {
            if (y & 1) {
                r = r ^ x;
            }
            y = y >> 1;
            x = x << 1;
            if (x & max) {
                x = x ^ GENERATOR;
            }
        }

        return r;
    }


private:
    struct gf_initializer {
        using type = typename detail::bit_container<detail::round_power_2(FIELD)>::type;
        static constexpr size_t max = size_t(1) << FIELD;
        static constexpr type modulo = max - 1;
        static constexpr type zero = type(0);
        static constexpr type infinity = type(modulo);

        constexpr gf_initializer() : log(), exp() {
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

    template<size_t F, size_t GEN>
    friend class galois;
    template<size_t F, size_t GEN>
    friend class galois_log;
    static inline constexpr std::array<type, 2*modulo + 1> _log = gf_initializer().log;
    static inline constexpr std::array<type, 2*modulo + 1> _exp = gf_initializer().exp;
};


template<size_t F, size_t GEN>
class galois;

template<size_t F, size_t GEN>
class galois_log {
public:
    using field = galois_field<F, GEN>;
    using other = galois<F, GEN>;

    using type = typename field::type;
    using large_type = typename field::large_type;

    static constexpr large_type generator = GEN;
    static constexpr large_type max = large_type(1) << F;
    static constexpr type modulo = max - 1;
    static constexpr type zero = type(0);
    static constexpr type infinity = type(modulo);
    static_assert((generator >> F) == 1, "Invalid generator");

    using ag = abelian_group<type, modulo>;

    #define EXP(x) galois_field<F, GEN>::_exp[(x)]
    #define LOG(x) galois_field<F, GEN>::_log[(x)]

    constexpr galois_log() : _value(infinity) {}
    constexpr galois_log(type v) : _value(v) {}

    constexpr galois_log(other v) : _value(LOG(type(v))) {}

    constexpr explicit operator type() const {
        return _value;
    }

    constexpr galois_log operator+(const galois_log other) const {
        return galois_log(LOG(EXP(modulo + _value) ^ EXP(modulo + other._value)));
    }
    constexpr galois_log operator-(const galois_log other) const {
        return galois_log(LOG(EXP(modulo + _value) ^ EXP(modulo + other._value)));
    }
    constexpr galois_log& operator+=(const galois_log other) const {
        _value = galois_log(LOG(EXP(modulo + _value) ^ EXP(modulo + other._value)));
        return *this;
    }
    constexpr galois_log& operator-=(const galois_log other) const {
        _value = galois_log(LOG(EXP(modulo + _value) ^ EXP(modulo + other._value)));
        return *this;
    }

    constexpr galois_log operator*(const galois_log other) const {
        if (_value == infinity || other._value == infinity) return galois_log(infinity);
        return galois_log(ag::add(_value, other._value));
    }
    constexpr galois_log operator*(const galois<F, GEN>& other) const {
        if (_value == infinity || other._value == galois<F, GEN>::zero) return galois_log(infinity);
        return galois_log(ag::add(_value, LOG(other._value)));
    }
    constexpr galois_log operator/(const galois_log other) const {
        if (_value == infinity || other._value == infinity) return galois_log(infinity);
        return galois_log(ag::sub(_value, other._value));
    }

    constexpr galois_log operator<<(unsigned shift) const {
        if (_value == 0) return galois_log(infinity);
        assert(shift <= max);
        return galois_log(ag::add(_value, shift));
    }

    constexpr galois_log inverse() {
        return galois_log(modulo - _value);
    }

    constexpr bool operator==(const galois_log other) const {
        return _value == other._value;
    }
    constexpr bool operator!=(const galois_log other) const {
        return !(_value == other._value);
    }


    #undef EXP
    #undef LOG
private:
    friend class galois<F, GEN>;
    type _value;
};

template<size_t F, size_t GEN>
class galois {
public:
    using field = galois_field<F, GEN>;
    using other = galois_log<F, GEN>;

    using type = typename field::type;
    using large_type = typename field::large_type;

    static constexpr large_type generator = GEN;
    static constexpr large_type max = large_type(1) << F;
    static constexpr type modulo = max - 1;
    static constexpr type zero = type(0);
    static constexpr type infinity = type(modulo);
    static_assert((generator >> F) == 1, "Invalid generator");

    //using log_type = strong_type<type, struct log_tag, addable, multiplicable, binopable, comparable>;
    using log_type = type;

    #define EXP(x) galois_field<F, GEN>::_exp[(x)]
    #define LOG(x) galois_field<F, GEN>::_log[(x)]

    constexpr galois() : _value(zero) {}
    constexpr galois(type v) : _value(v) {}
    constexpr galois(galois_log<F, GEN> v) : _value(EXP(modulo + type(v))) {}

    constexpr explicit operator type() const {
        return _value;
    }

    constexpr galois operator+(const galois other) const {
        return galois(_value ^ other._value);
    }
    constexpr galois operator-(const galois other) const {
        return galois(_value ^ other._value);
    }
    constexpr galois& operator+=(const galois other) {
        //_value ^= other._value;
        _value = galois(_value ^ other._value)._value;
        return *this;
    }
    constexpr galois& operator-=(const galois other) const {
        //_value ^= other._value;
        _value = galois(_value ^ other._value);
        return *this;
    }

    constexpr galois operator*(const galois& other) const {
        if (_value == 0 || other._value == 0) return galois(zero);
        auto v1 = LOG(_value);
        auto v2 = LOG(other._value);
        return galois(EXP(v1 + v2));
    }
    constexpr galois operator*(const galois_log<F, GEN>& other) const {
        if (_value == 0 || other._value == galois_log<F, GEN>::infinity) return galois(zero);
        return galois(EXP(LOG(_value) + other._value));
    }
    constexpr galois operator/(const galois& other) const {
        if (_value == 0 || other._value == 0) return galois(zero);
        return galois(EXP(modulo + LOG(_value) - LOG(other._value)));
    }

    constexpr galois operator<<(unsigned shift) const {
        // modulo + _log[_value] + shift can exceed the bounds of the array
        if (_value == 0) return galois(zero);
        int index = LOG(_value) + shift;
        while (index >= modulo)
            index -= modulo;

        return galois(EXP(index));
    }

    constexpr bool operator==(const galois& other) const {
        return _value == other._value;
    }
    constexpr bool operator!=(const galois& other) const {
        return !(_value == other._value);
    }

    constexpr galois inverse() {
        return galois(EXP(modulo - LOG(_value)));
    }

    #undef EXP
    #undef LOG
private:
    friend class galois_log<F, GEN>;
    type _value;
};

template<typename T, size_t SZ>
class polynomial {
public:
    using type = T;
    static constexpr size_t size = SZ;

    constexpr polynomial() : degree_(0), elements{ }
    {
    }
    constexpr polynomial(size_t degree) : degree_(degree), elements() {
        for (unsigned i = 0; i < degree_; i++) {
            elements[i] = 0;
        }
    }
    constexpr polynomial(std::initializer_list<type> e) : degree_(e.size()), elements() {
        auto it = e.begin();
        for (unsigned i = 0; i < e.size(); i++) {
            elements[i] = *it++;
        }
    }
    constexpr polynomial(const polynomial& other) : degree_(other.degree_), elements(other.elements) {
    }
    template<typename OT>
    constexpr polynomial(const polynomial<OT, SZ>& other) : degree_(other.degree_), elements() {
        for (unsigned i = 0; i < other.degree_; i++) {
            elements[i] = other.elements[i];
        }
    }
    constexpr polynomial(polynomial&& other) : degree_(other.degree_), elements(std::move(other.elements)) {}
    constexpr polynomial& operator=(const polynomial& other) { degree_ = other.degree_; elements = other.elements; return *this; }

    constexpr void find_degree() {
        degree = 0;
        for (unsigned i = 0; i < size; i++) {
            if (elements[i] != type()) {
                degree_ = i + 1;
            }
        }
    }

    unsigned& degree() { return degree_; }

    constexpr void eval(const type* data, size_t count, size_t incr = 1) {
        //static_assert(incr == 1 || incr == -1, "invalid increment");

        struct powers_gen {
            constexpr powers_gen() : arr() {
                for (size_t i = 1; i <= size; i++) {
                    arr[i-1] = type(1) << i;
                }
            }
            std::array<typename type::other, size> arr;
        };
        constexpr auto powers = powers_gen().arr;

        for (size_t n = 0; n < SZ; n++) {
            elements[n] = *data;
        }
        data += incr;

        for (size_t i = 1; i < count; i++) {
            for (size_t n = 0; n < SZ; n++) {
                if (elements[n] == type()) {
                    elements[n] = *data;
                } else {
                    elements[n] = *data + elements[n] * powers[n];
                }
            }
            data += incr;
        }
        find_degree();
    }

    constexpr void scale(type scalar, const type f) {
        for (unsigned i = 0; i < SZ; i++) {
            elements[i] = f.mul(elements[i], scalar);
        }
    }

    template<typename RT>
    constexpr RT mul(const polynomial& other) const {
        RT res;
        for (unsigned i = 0; i < degree; i++) {
            auto acc = typename RT::type();
            for (int j = std::min(other.degree, i); j >= 0; j--) {
                acc = acc + elements[i-j] * other[j];
            }
            res[i] = acc;
            if (acc != typename RT::type()) {
                res.degree_ = i+1;
            }
        }
        return res;
    }

    constexpr polynomial operator*(const polynomial& other) const {
        return mul<polynomial>(other);
    }

    constexpr type& operator[](size_t index) {
        return elements[index];
    }

    constexpr type const& operator[](size_t index) const {
        return elements[index];
    }

private:
    template<typename type, size_t Z>
    friend class polynomial;
public:
    unsigned degree_;
    std::array<type, SZ> elements;
};

template<typename T, size_t SZ>
class counted_array {
public:
    using type = T;
    static constexpr size_t size = SZ;

    constexpr counted_array() : degree_(0), elements_{} {}
    constexpr counted_array(std::initializer_list<type> e) : degree_(e.size()), elements_() {
        auto it = e.begin();
        for (unsigned i = 0; i < e.size(); i++) {
            elements_[i] = *it++;
        }
    }
    constexpr counted_array(const counted_array& other) : degree_(other.degree_), elements_(other.elements_) {
    }
    template<typename OT>
    constexpr counted_array(const counted_array<OT, SZ>& other) : degree_(other.degree_), elements_() {
        for (unsigned i = 0; i < other.degree_; i++) {
            elements_[i] = other.elements_[i];
        }
    }
    constexpr counted_array(counted_array&& other) : degree_(other.degree_), elements_(std::move(other.elements_)) {}
    constexpr counted_array& operator=(const counted_array& other) { degree_ = other.degree_; elements_ = other.elements_; return *this; }

    constexpr void find_degree() {
        degree_ = 0;
        for (unsigned i = 0; i < size; i++) {
            if (elements_[i] != type()) {
                degree_ = i + 1;
            }
        }
    }

    constexpr size_t& degree() { return degree_; }
    constexpr std::array<type, size>& elements() { return elements_; }

    constexpr type& operator[](size_t index) {
        return elements_[index];
    }

    constexpr type const& operator[](size_t index) const {
        return elements_[index];
    }

protected:
    size_t degree_;
    std::array<type, SZ> elements_;
};

