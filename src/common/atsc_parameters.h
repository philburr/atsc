#pragma once
#include <complex>
#include <array>

struct atsc_parameters {
    static constexpr unsigned ATSC_SEGMENTS_PER_FIELD = 313;
    static constexpr unsigned ATSC_DATA_SEGMENTS = ATSC_SEGMENTS_PER_FIELD - 1;
    static constexpr unsigned ATSC_MPEG2_BYTES = 188;
    static constexpr unsigned ATSC_SEGMENT_BYTES = ATSC_MPEG2_BYTES - 1;
    static constexpr unsigned ATSC_RS_BYTES = 20;
    static constexpr unsigned ATSC_SEGMENT_FEC_BYTES = ATSC_SEGMENT_BYTES + ATSC_RS_BYTES;
    static constexpr unsigned ATSC_SYMBOLS_PER_BYTE = 4;
    static constexpr unsigned ATSC_SYMBOLS_PER_SEGMENT = (ATSC_SEGMENT_FEC_BYTES+1) * ATSC_SYMBOLS_PER_BYTE;
    static constexpr unsigned ATSC_DATA_SYMBOLS_PER_SEGMENT = ATSC_SEGMENT_FEC_BYTES * ATSC_SYMBOLS_PER_BYTE;
    static constexpr unsigned ATSC_DATA_PER_FIELD = ATSC_SEGMENT_FEC_BYTES * ATSC_DATA_SEGMENTS;
    static constexpr unsigned ATSC_DATA_SYMBOLS_PER_FIELD = ATSC_DATA_SYMBOLS_PER_SEGMENT * ATSC_DATA_SEGMENTS;
    static constexpr unsigned ATSC_SYMBOLS_PER_FIELD = ATSC_SYMBOLS_PER_SEGMENT * ATSC_SEGMENTS_PER_FIELD;
    static constexpr unsigned ATSC_RESERVED_SYMBOLS = 12;
    static constexpr unsigned ATSC_TRELLIS_ENCODERS = 12;

    using atsc_field_mpeg2 = std::array<uint8_t, ATSC_DATA_SEGMENTS * ATSC_MPEG2_BYTES>;
    using atsc_field_data = std::array<uint8_t, ATSC_DATA_PER_FIELD>;

    using atsc_symbol_type = std::complex<float>;
    using atsc_field_signal = std::array<atsc_symbol_type, ATSC_SYMBOLS_PER_FIELD>;
};

static constexpr unsigned ATSC_TRELLIS_INPUT_SOFTWARE = 13;
static constexpr unsigned ATSC_TRELLIS_INPUT_OPENCL = atsc_parameters::ATSC_DATA_PER_FIELD / atsc_parameters::ATSC_TRELLIS_ENCODERS;