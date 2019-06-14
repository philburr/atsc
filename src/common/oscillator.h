#pragma once

template<typename PARAMETERS>
struct oscillator_table {
    constexpr oscillator_table() {}

private:
    static constexpr double frequency = 309411.0 - 3000000.0;
    static constexpr double amplitude = 0.9;
    static constexpr double sampling_frequency = 4500000.0 / 286 * 684;
    struct initializer {

        initializer() : table() {
            double phase_increment = 2 * M_PI * frequency / sampling_frequency;

            for (size_t i = 0; i < table.size(); i++) {
                table[i] = std::complex<float>(amplitude * cosf(phase_increment * i), 
                                               amplitude * sinf(phase_increment * i));
            }

            scale = std::complex<float>(cosf(phase_increment * table.size()),
                                           sinf(phase_increment * table.size()));
        }

        std::complex<float> scale;
        std::array<std::complex<float>, PARAMETERS::ATSC_SYMBOLS_PER_FIELD> table;
    };

public:
    static inline std::complex<float> scale = initializer().scale;
    static inline std::array<std::complex<float>, PARAMETERS::ATSC_SYMBOLS_PER_FIELD> table = initializer().table;
};