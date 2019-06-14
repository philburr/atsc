#pragma once

template<typename T>
struct oscillator_table {
    constexpr oscillator_table() {}

    static constexpr double frequency = 309411.0 - 3000000.0;
    static constexpr double amplitude = 0.9;
    static constexpr double sampling_frequency = 4500000.0 / 286 * 684;

private:
    struct initializer {

        initializer() : table() {
            double phase_increment = 2 * M_PI * frequency / sampling_frequency;

            for (size_t i = 0; i < table.size(); i++) {
                table[i] = std::complex<float>(amplitude * cosf(phase_increment * i), 
                                               amplitude * sinf(phase_increment * i));
            }

            scale = std::complex<float>(cosf(phase_increment * ATSC_SYMBOLS_PER_FIELD),
                                           sinf(phase_increment * ATSC_SYMBOLS_PER_FIELD));
        }

        std::complex<float> scale;
        std::array<std::complex<float>, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT> table;
    };

public:
    static inline std::complex<float> scale = initializer().scale;
    static inline std::array<std::complex<float>, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT> table = initializer().table;
};