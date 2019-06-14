#include <cstdio>
#include <cstring>
#include <memory>
#include <functional>
#include <immintrin.h>

#include "atsc/atsc.h"
#include "common/atsc_parameters.h"

template<typename T>
using unique_freeable_ptr = std::unique_ptr<T,std::function<void(T*)>>;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Invalid parameters\n");
        return 0;
    }
    auto input = unique_freeable_ptr<FILE>();
    if (strcmp(argv[1], "-") == 0) {
        input = unique_freeable_ptr<FILE>(stdin, [](FILE*){});
    } else {
        input = unique_freeable_ptr<FILE>(fopen(argv[1], "rb"), [](FILE* fp) { fclose(fp); });
    }
    if (!input) {
        fprintf(stderr, "Failed to read input file\n");
        return 0;
    }

    auto encoder = atsc_encoder::create();
    if (!encoder) {
        printf("Failed to create encoder\n");
        return 0;
    }

    int16_t* result = (int16_t*)_mm_malloc(sizeof(int16_t) * atsc_parameters::ATSC_SYMBOLS_PER_FIELD, 32);

    auto in = std::make_unique<atsc_parameters::atsc_field_mpeg2>();
    uint8_t* in_data = in.get()->data();
    while (fread(in_data, atsc_parameters::ATSC_DATA_SEGMENTS * atsc_parameters::ATSC_MPEG2_BYTES, 1, input.get()) == 1) {
        encoder->process(in_data, atsc_parameters::ATSC_DATA_SEGMENTS, [](void*, unsigned) {
        });
    }

    _mm_free(result);

    return 0;
}