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

struct atsc_encoder_impl : public atsc::atsc_encoder {

    atsc_encoder_impl()
        : field1(std::make_unique<atsc_field_data>())
        , field2(std::make_unique<atsc_field_data>())
        , current_field(*field1)
        , next_field(*field2)
        , input_received(0)
    {
        for (unsigned i = 0; i < ATSC_RESERVED_SYMBOLS; i++) {
            saved_symbols[i] = atsc_symbol_to_signal<atsc_symbol_type>::xform(0);
        }
        
        input = std::make_unique<atsc_field_mpeg2>();
        encoded = std::make_unique<atsc_field_data>();

        out = std::make_unique<aligned<atsc_field_symbol_padded>>();
        filtered = std::make_unique<aligned<atsc_field_symbol>>();

        memset(field1->data(), 0, field1->size());
    }

    atsc_reserved_symbols saved_symbols;
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
    std::unique_ptr<atsc_field_symbol_padded> out;
    std::unique_ptr<atsc_field_symbol> filtered;

    atsc_field_data& current_field;
    atsc_field_data& next_field;

    size_t input_received;

    // Process N 188-byte MPEG-2 transport packets
    void process(uint8_t* pkt, unsigned count, std::function<void(void*,unsigned)> fn) override {
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
        randomizer.randomize_pkts(*encoded, *input);
        fec.process_field(*encoded);
        interleaver.process(current_field, next_field, *encoded);
        trellis.process(*out, current_field);
        field_sync.process_field(*out, saved_symbols);
        offset.process_field(*out);
        filter.process_field(*filtered, *out);

        std::swap(current_field, next_field);
        fn(filtered->data(), ATSC_SYMBOLS_PER_FIELD * sizeof(std::complex<float>));
    }
};

std::unique_ptr<atsc::atsc_encoder> atsc::atsc_encoder::create() {
    return std::make_unique<atsc_encoder_impl>();
}
