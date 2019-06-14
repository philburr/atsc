#pragma once
#include "common/atsc_parameters.h"

#pragma once

#include "galois.h"
#include <cstring>


template <size_t F, size_t POLY, size_t ROOTS, size_t SKIP = 0, size_t INCR = 1>
class reed_solomon {
public:
    using gf = galois<F, POLY>;
    using gf_log = galois_log<F, POLY>;
    constexpr static size_t roots = ROOTS;

    using poly = polynomial<gf, ROOTS>;
    using lpoly = polynomial<gf_log, ROOTS>;
    using long_poly = polynomial<gf, ROOTS+1>;
    using long_lpoly = polynomial<gf_log, ROOTS+1>;


    constexpr reed_solomon() {
        long_poly genpoly = {1};
        for (size_t i = 0, root = 0; i < roots; i++, root++) {
            genpoly.degree() = i + 2;
            genpoly[i+1] = 1;

            for (size_t j = i; j > 0; j--) {
                if (genpoly[j] != 0) {
                    genpoly[j] = genpoly[j - 1] + (genpoly[j] << root);
                } else {
                    genpoly[j] = genpoly[j - 1];
                }
            }
            genpoly[0] = genpoly[0] << root;
        }
        _genpoly = genpoly;
    }

    bool calc_syndrome(typename gf::type* buffer, size_t count) {

        //_syndrome.eval<1> ((gf*)buffer, count);
        _syndrome.eval((gf*)buffer, count, INCR);
        bool ok = _syndrome.degree == 0;
        if (ok)
            return true;

        //return find_error_locator();
        return false;
    }

    void find_error_locator() {
        auto beta = long_poly();
        auto tmp = long_poly();
        auto lambda = long_poly {1};

        unsigned L = 0;
        for (unsigned r = 1; r <= ROOTS; r++) {
            gf delta = 0;
            for (unsigned i = 0; i < r; i++) {
                delta = delta + lambda[i] * _syndrome[r-i-1];
            }

            if (delta == 0) {
                // this is delta == 0, sigma == 0

                // lambda = lambda
                // beta = beta * x
                memmove(&beta[1], &beta[0], ROOTS * sizeof(gf));
                beta[0] = 0;
            }
            else {
                // delta != 0

                // lambda(tmp) = lambda - delta * beta * x
                tmp[0] = lambda[0];
                for (unsigned i = 0; i < ROOTS; i++) {
                    tmp[i + 1] = lambda[i + 1] - delta * beta[i];
                }

                // Calculation of beta depends on this:
                if (2 * L <= r - 1) {
                    // sigma = 1
                    L = r - L;

                    // beta = inv(delta) * lambda
                    gf inv_delta = delta.inverse();
                    for (unsigned i = 0; i <= ROOTS; i++) {
                        beta[i] = lambda[i] * inv_delta;
                    }
                } else {
                    // sigma = 0
                    // beta = beta * x
                    memmove(&beta[1], &beta[0], ROOTS * sizeof(gf));
                    beta[0] = 0;
                }

                // lambda = tmp
                memcpy(&lambda[0], &tmp[0], (ROOTS + 1) * sizeof(gf));
            }
        }

        lambda.find_degree();
        _lambda = lambda;
    }

    bool chien_search() {
        long_poly reg;

        reg = _lambda;
        unsigned nroots = 0;

        unsigned count = 0;
        for (unsigned i = 0; i < gf::modulo; i++) {
            auto q = gf(1);
            for (unsigned j = _lambda.degree; j > 0; j--) {
                if (reg[j] != gf(0)) {
                    reg[j] = reg[j] * gf_log(j);
                    q = q + reg[j];
                }
            }
            if (q != 0)
                continue;
            roots_[count] = i + 1;
            locations_[count++] = (i - SKIP) * INCR;
            if (count == _lambda.degree) {
                break;
            }
        }
        locations_.degree() = count;
        if (_lambda.degree - 1 != count) {
            return false;
        }
        return true;
    }

    void calc_omega() {
        // Calculate syndrome*lambda
        unsigned degree_omega;
        for (unsigned i = 0; i < ROOTS; i++) {
            gf acc;

            for (int j = std::min(_lambda.degree, i); j >= 0; j--) {
                acc = acc + _syndrome[i - j] * _lambda[j];
            }
            if (acc != gf::zero) {
                degree_omega = i;
            }

            _omega[i] = acc;
        }
        _omega.degree = degree_omega + 1;
    }

    bool correct_errors(typename gf::type* buffer, size_t count) {
        for (int j = locations_.degree() - 1; j >= 0; j--) {
            gf num1, num2, den;

            for (int i = _omega.degree - 1; i >= 0; i--) {
                num1 = num1 + (_omega[i] << (i * roots_[j]));
            }
            num2 = gf(1);

            for (int i = std::min((size_t)_lambda.degree - 1, roots - 1) & ~1; i >= 0; i-= 2) {
                den = den + (_lambda[i + 1] << (i * roots_[j]));
            }
            if (den == gf()) {
                return false;
            }

            if (num1 != gf()) {
                auto correction = num1 * num2 * den.inverse();
                buffer[locations_[j]] ^= typename gf::type(correction);
            }
        }
        return true;
    }

    bool correct(typename gf::type* buffer, size_t count) {
        if (calc_syndrome(buffer, count)) {
            return true;
        }
        find_error_locator();
        if (!chien_search()) {
            return false;
        }
        calc_omega();
        return false;
    }

    void encode_rs(gf* bb, gf* buffer, size_t count) {
        gf_log feedback;

        for (unsigned i = 0; i < count; i++) {
            feedback = buffer[i] + bb[0];
            if (feedback != gf_log::infinity) {
                for (unsigned j = 1; j < roots; j++) {
                    bb[j] += _genpoly[roots - j] << (uint8_t)feedback;
                }
            }
            memmove(&bb[0], &bb[1], sizeof(gf) * (roots - 1));
            if (feedback != gf_log::infinity) {
                bb[roots-1] = _genpoly[0] << (uint8_t)feedback;
            } else {
                bb[roots-1] = 0;
            }
        }
    }

public:
    poly& syndrome() { return _syndrome; }
    poly& omega() { return _omega; }
    long_poly& lambda() { return _lambda; }
    counted_array<int, ROOTS>& locations() { return locations_; }

private:
    poly _syndrome, _omega;
    long_poly _lambda;
    long_lpoly _genpoly;

    counted_array<int, ROOTS> locations_, roots_;
};

struct atsc_reed_solomon {
    atsc_reed_solomon() : rs_() {}

    using fec = reed_solomon<8, 0x11d, 20>;

    void process_field(atsc_field_data& buffer) {
        uint8_t padding[40];
        memset(padding, 0, sizeof(padding));

        for (size_t dseg = 0; dseg < ATSC_DATA_SEGMENTS; dseg++) {

            uint8_t* bb = buffer.data() + ATSC_SEGMENT_FEC_BYTES * dseg + ATSC_SEGMENT_BYTES;
            rs_.encode_rs((fec::gf*)bb, (fec::gf*)padding, 40);

            uint8_t* curr = buffer.data() + ATSC_SEGMENT_FEC_BYTES * dseg;
            rs_.encode_rs((fec::gf*)bb, (fec::gf*)curr, ATSC_SEGMENT_BYTES);
        }
    }


private:
    fec rs_;
};
