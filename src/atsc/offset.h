#pragma once
#include <cmath>
#include <immintrin.h>
#include "common/atsc_parameters.h"
#include "util.h"

struct atsc_offset {

    atsc_offset() : _table(std::make_unique<aligned_array<std::complex<float>, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT>>(table)) {}

    void process_field(atsc_field_symbol_padded& field) {
        unsigned items = (ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT) / 4;

        std::complex<float> *a = field.data();
        std::complex<float> *b = _table->data();
        std::complex<float> *s = scale.data();

#if HAVE_FMA
        const __m256 e = _mm256_load_ps((float*)s);
        const __m256 el = _mm256_moveldup_ps(e); // Load el with er,er,fr,fr
        const __m256 eh = _mm256_movehdup_ps(e); // Load eh with ei,ei,fi,fi

        for(unsigned i = 0; i < items; i++) {

            const __m256 x = _mm256_load_ps((float*)a); // Load the ar + ai, br + bi as ar,ai,br,bi
            const __m256 y = _mm256_load_ps((float*)b); // Load the cr + ci, dr + di as cr,ci,dr,di

            const __m256 yl = _mm256_moveldup_ps(y); // Load yl with cr,cr,dr,dr
            const __m256 yh = _mm256_movehdup_ps(y); // Load yh with ci,ci,di,di

            const __m256 tmp2x = _mm256_permute_ps(x,0xB1); // Re-arrange x to be ai,ar,bi,br
            const __m256 tmp2y = _mm256_permute_ps(y,0xB1); // Re-arrange y to be ci,cr,di,dr

            const __m256 tmp2 = _mm256_mul_ps(tmp2x, yh); // tmp2 = ai*ci,ar*ci,bi*di,br*di
            const __m256 tmp3 = _mm256_mul_ps(tmp2y, eh); // tmp3 = ci*ei,cr*ei,di*fi,dr*fi

            const __m256 z = _mm256_fmaddsub_ps(x, yl, tmp2); // ar*cr-ai*ci, ai*cr+ar*ci, br*dr-bi*di, bi*dr+br*di
            const __m256 r = _mm256_fmaddsub_ps(y, el, tmp3); // cr*er-ci*ei, ci*er+cr*ei, dr*fr-di*fi, di*fr+dr*fi

            _mm256_store_ps((float*)a, z); // Store the results back into the C container
            _mm256_store_ps((float*)b, r);

            a += 4;
            b += 4;
        }
#elif HAVE_AVX
        const __m256 e = _mm256_load_ps((float*)s);
        const __m256 el = _mm256_moveldup_ps(e); // Load el with er,er,fr,fr
        const __m256 eh = _mm256_movehdup_ps(e); // Load eh with ei,ei,fi,fi

        for(unsigned i = 0; i < items; i++) {

            const __m256 x = _mm256_load_ps((float*)a); // Load the ar + ai, br + bi as ar,ai,br,bi
            const __m256 y = _mm256_load_ps((float*)b); // Load the cr + ci, dr + di as cr,ci,dr,di

            const __m256 yl = _mm256_moveldup_ps(y); // Load yl with cr,cr,dr,dr
            const __m256 yh = _mm256_movehdup_ps(y); // Load yh with ci,ci,di,di

            const __m256 tmp2x = _mm256_permute_ps(x,0xB1); // Re-arrange x to be ai,ar,bi,br
            const __m256 tmp2y = _mm256_permute_ps(y,0xB1); // Re-arrange y to be ci,cr,di,dr

            const __m256 tmp2 = _mm256_mul_ps(tmp2x, yh); // tmp2h = ai*ci,ar*ci,bi*di,br*di
            const __m256 tmp3 = _mm256_mul_ps(tmp2y, eh);  // tmp3h = ci*ei,cr*ei,di*fi,dr*fi

            const __m256 z = _mm256_addsub_ps(_mm256_mul_ps(x, yl), tmp2); // ar*cr-ai*ci, ai*cr+ar*ci, br*dr-bi*di, bi*dr+br*di
            const __m256 r = _mm256_addsub_ps(_mm256_mul_ps(y, el), tmp3); // cr*er-ci*ei, ci*er+cr*ei, dr*fr-di*fi, di*fr+dr*fi

            _mm256_store_ps((float*)a, z); // Store the results back into the C container
            _mm256_store_ps((float*)b, r);

            a += 4;
            b += 4;
        }
#else
#error Not implemented        
#endif

    }

private:
    std::unique_ptr<aligned_array<std::complex<float>, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT>> _table;


    struct oscillator_table_generator {
        static constexpr double frequency = 309411.0 - 3000000.0;
        static constexpr double amplitude = 0.9;
        static constexpr double sampling_frequency = 4500000.0 / 286 * 684;

        oscillator_table_generator() : table() {
            double phase_increment = 2 * M_PI * frequency / sampling_frequency;

            for (size_t i = 0; i < table.size(); i++) {
                table[i] = std::complex<float>(amplitude * cosf(phase_increment * i), 
                                               amplitude * sinf(phase_increment * i));
            }

            scale[0] = std::complex<float>(cosf(phase_increment * ATSC_SYMBOLS_PER_FIELD),
                                           sinf(phase_increment * ATSC_SYMBOLS_PER_FIELD));
            scale[1] = scale[0];
            scale[2] = scale[0];
            scale[3] = scale[0];
        }

        std::array<std::complex<float>, 4> scale;
        std::array<std::complex<float>, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT> table;
    };

    static inline std::array<std::complex<float>, 4> scale = oscillator_table_generator().scale;
    static inline std::array<std::complex<float>, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT> table = oscillator_table_generator().table;
};