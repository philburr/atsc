#pragma once

template<typename T>
struct atsc_symbol_to_signal {};

template<>
struct atsc_symbol_to_signal<float> {
    static constexpr float xform(uint8_t v) {
        return float(v*2 - 7) + 1.25;
    }
};

template<>
struct atsc_symbol_to_signal<std::complex<float>> {
    static constexpr std::complex<float> xform(uint8_t v) {
        return std::complex<float>(float(v*2 - 7) + 1.25, 0);
    }
};

