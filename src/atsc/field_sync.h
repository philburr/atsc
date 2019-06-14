#pragma once
#include "common/atsc_parameters.h"
#include "signal.h"

template<unsigned INITIAL_STATE, unsigned SHIFT_OUT, unsigned MASK, unsigned BITS>
struct lfsr {
    
private:
    struct lfsr_generator {

        constexpr lfsr_generator() : table() {
            unsigned state = INITIAL_STATE;
            unsigned out_shift = SHIFT_OUT;
            unsigned mask = MASK;

            for (unsigned i = 0; i < BITS; i++) {
                table[i] = (state >> out_shift) & 1;

                unsigned update = state & mask;

                update ^= update >> 16;
                update ^= update >> 8;
                update ^= update >> 4;
                update &= 0xf;
                update = (0x6996 >> update) & 1;
                state = (state << 1) | update;
            }
        }

        std::array<uint8_t, BITS> table;
    };

public:
    static inline constexpr std::array<uint8_t, BITS> table = lfsr_generator().table;
};


template<typename PARAMETERS>
struct atsc_field_sync {

    atsc_field_sync() : current_sync(&field_sync_even), next_sync(&field_sync_odd) {}

    void process_field(typename PARAMETERS::atsc_signal_type* field, typename PARAMETERS::atsc_signal_type* saved_symbols) {
        const field_sync_array& field_sync = *current_sync;

        memcpy(field, field_sync.data(), field_sync.size() * sizeof(typename PARAMETERS::atsc_signal_type));
        memcpy(field + PARAMETERS::ATSC_SYMBOLS_PER_SEGMENT - PARAMETERS::ATSC_RESERVED_SYMBOLS, saved_symbols, PARAMETERS::ATSC_RESERVED_SYMBOLS * sizeof(typename PARAMETERS::atsc_signal_type));

        for (size_t i = PARAMETERS::ATSC_SYMBOLS_PER_SEGMENT; i < PARAMETERS::ATSC_SYMBOLS_PER_FIELD; i += PARAMETERS::ATSC_SYMBOLS_PER_SEGMENT) {
            memcpy(field + i, segment_sync.data(), segment_sync.size() * sizeof(typename PARAMETERS::atsc_signal_type));
        }

        memcpy(saved_symbols, field + PARAMETERS::ATSC_SYMBOLS_PER_FIELD - PARAMETERS::ATSC_RESERVED_SYMBOLS, PARAMETERS::ATSC_RESERVED_SYMBOLS * sizeof(typename PARAMETERS::atsc_signal_type));

        std::swap(current_sync, next_sync);
    }

    void process_segment(typename PARAMETERS::atsc_signal_type* field) {
        const field_sync_array& field_sync = *current_sync;

        memcpy(field, field_sync.data(), field_sync.size() * sizeof(typename PARAMETERS::atsc_signal_type));
    }

private:
    using xformer = atsc_symbol_to_signal<typename PARAMETERS::atsc_signal_type>;
    struct segment_sync_generator {
        constexpr segment_sync_generator() : table() {
            table[0] = xformer::xform(6);
            table[1] = xformer::xform(1);
            table[2] = xformer::xform(1);
            table[3] = xformer::xform(6);
        }
        std::array<typename PARAMETERS::atsc_signal_type, 4> table;
    };

    struct field_sync_generator {

        constexpr field_sync_generator(bool even, std::array<uint8_t, 511> pn511, std::array<uint8_t, 63> pn63) : table() {
            table[0] = xformer::xform(6);
            table[1] = xformer::xform(1);
            table[2] = xformer::xform(1);
            table[3] = xformer::xform(6);

            size_t sym = 4;
            for (size_t i = 0; i < pn511.size(); i++, sym++) {
                table[sym] = xformer::xform(pn511[i] ? 6 : 1);
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = xformer::xform(pn63[i] ? 6 : 1);
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = xformer::xform((!!pn63[i] == even) ? 6 : 1);
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = xformer::xform(pn63[i] ? 6 : 1);
            }

            std::array<uint8_t, 24> vsb_mode = {0,0,0,0, 1,0,1,0, 0,1,0,1, 1,1,1,1, 0,1,0,1, 1,0,1,0};
            for (size_t i = 0; i < vsb_mode.size(); i++, sym++) {
                table[sym] = xformer::xform(vsb_mode[i] ? 6 : 1);
            }

            for (size_t i = 0; i < 104 - PARAMETERS::ATSC_RESERVED_SYMBOLS; i++, sym++) {
                table[sym] = xformer::xform(pn63[i % pn63.size()] ? 6 : 1);
            }
        }

        std::array<typename PARAMETERS::atsc_signal_type, PARAMETERS::ATSC_SYMBOLS_PER_SEGMENT - PARAMETERS::ATSC_RESERVED_SYMBOLS> table;
    };

    static inline constexpr std::array<typename PARAMETERS::atsc_signal_type, 4> segment_sync = segment_sync_generator().table;

    using field_sync_array = std::array<typename PARAMETERS::atsc_signal_type, PARAMETERS::ATSC_SYMBOLS_PER_SEGMENT - PARAMETERS::ATSC_RESERVED_SYMBOLS>;
    static inline constexpr field_sync_array field_sync_even = 
        field_sync_generator(
            true,
            lfsr<0b10, 8, 0b110110110, 511>::table,
            lfsr<0b111001, 5, 0b110000, 63>::table
        ).table;
    static inline constexpr field_sync_array field_sync_odd = 
        field_sync_generator(
            false,
            lfsr<0b10, 8, 0b110110110, 511>::table,
            lfsr<0b111001, 5, 0b110000, 63>::table
        ).table;

    const field_sync_array* current_sync;
    const field_sync_array* next_sync;
};