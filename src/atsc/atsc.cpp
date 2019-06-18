#include <cstdio>
#include <memory>
#include <functional>
#include <complex>
#include "atsc.h"
#include "galois.h"
#include "randomize.h"
#include "reed_solomon.h"
#include "interleaver.h"
#include "trellis.h"
#include "field_sync.h"
#include "offset.h"
#include "filter.h"

struct atsc_encoder_impl : public atsc_encoder {

    atsc_encoder_impl() : input_received(0) {
        for (unsigned i = 0; i < ATSC_RESERVED_SYMBOLS; i++) {
            saved_symbols[i] = atsc_symbol_to_signal<atsc_symbol_type>::xform(0);
        }
        
        input = std::make_unique<atsc_field_mpeg2>();
        encoded = std::make_unique<atsc_field_data>();
        field1 = std::make_unique<atsc_field_data>();
        field2 = std::make_unique<atsc_field_data>();

        out = unique_freeable_ptr<std::complex<float>>((std::complex<float>*)_mm_malloc(sizeof(atsc_symbol_type) * (ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT), 32), _mm_free);
        field_sync.process_segment(out.get() + ATSC_SYMBOLS_PER_FIELD);
        filtered = unique_freeable_ptr<std::complex<float>>((std::complex<float>*)_mm_malloc(sizeof(atsc_symbol_type) * ATSC_SYMBOLS_PER_FIELD, 32), _mm_free);

        memset(field1->data(), 0, field1->size());

        current_field = field1.get()->data();
        next_field = field2.get()->data();
    }

    atsc_symbol_type saved_symbols[ATSC_RESERVED_SYMBOLS];
    atsc_randomize<void> randomizer;
    atsc_reed_solomon fec;
    atsc_interleaver<true> interleaver;
    atsc_trellis_encoder trellis;
    atsc_field_sync<void> field_sync;
    atsc_offset offset;
    atsc_rrc_filter filter;

    std::unique_ptr<atsc_field_mpeg2> input;
    std::unique_ptr<atsc_field_data> encoded;
    std::unique_ptr<atsc_field_data> field1;
    std::unique_ptr<atsc_field_data> field2;
    unique_freeable_ptr<std::complex<float>> out;
    unique_freeable_ptr<std::complex<float>> filtered;

    uint8_t* current_field;
    uint8_t* next_field;

    size_t input_received;

    // Process N 188-byte MPEG-2 transport packets
    void process(uint8_t* pkt, unsigned count, std::function<void(void*,unsigned)> fn) {
        uint8_t* in_data = input->data();
        size_t byte_count = count * ATSC_MPEG2_BYTES;

        while (byte_count > 0) {
            size_t remaining = input->size() - input_received; 
            size_t to_copy = std::min(byte_count, remaining);
            memcpy(in_data + input_received, pkt, to_copy);

            input_received += to_copy;
            pkt += to_copy;
            byte_count -= to_copy;

            if (input_received == input->size()) {
                process_field(fn);
                input_received = 0;
            }
        }
    }

    void process_field(std::function<void(void*,unsigned)> fn) {
        uint8_t* in_data = input.get()->data();
        uint8_t* encoded_data = encoded.get()->data();
        atsc_symbol_type* out_data = out.get();

        randomizer.randomize_pkts(encoded_data, in_data);
        fec.process_field(encoded_data);
        interleaver.process(current_field, next_field, encoded_data);
        trellis.process(out_data, current_field);
        field_sync.process_field(out_data, saved_symbols);
        offset.process_field(out_data);
        filter.process_field(filtered.get(), out_data);

        std::swap(current_field, next_field);
        fn(filtered.get(), ATSC_SYMBOLS_PER_FIELD * sizeof(std::complex<float>));
    }
};

std::unique_ptr<atsc_encoder> atsc_encoder::create() {
    return std::make_unique<atsc_encoder_impl>();
}

void atsc_encoder::process(uint8_t* pkt, unsigned count, std::function<void(void*,unsigned)> fn) {
    dynamic_cast<atsc_encoder_impl*>(this)->process(pkt, count, fn);
}
