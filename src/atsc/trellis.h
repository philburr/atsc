#pragma once
#include <immintrin.h>
#include <cstring>
#include "common/atsc_parameters.h"
#include "util.h"
#include "signal.h"

static std::array<uint8_t, 256> BitReverseTable256 = detail::reverse_table_initializer().table; 

class atsc_trellis_encoder_ref {

    struct differential_encoder {
        differential_encoder() : D(0) {}

        uint8_t encode(bool bit) {
            D = D ^ bit;
            return (uint8_t)D << 2;
        }
    private:
        bool D;
    };

    struct convolutional_encoder {
        convolutional_encoder() : D1(0), D2(0) {}

        uint8_t encode(bool bit) {
            bool z0 = D1;
            D1 = bit ^ D2;
            D2 = z0;
            return ((uint8_t)bit << 1) | (uint8_t)z0;
        }
    private:
        bool D1, D2;
    };

    struct trellis_encoder {
        trellis_encoder() {}

        void process(uint8_t* output, uint8_t input) {
            input = BitReverseTable256[input];

            *output++ = diff.encode(input & 1) | conv.encode((input >> 1) & 1); input >>= 2;
            *output++ = diff.encode(input & 1) | conv.encode((input >> 1) & 1); input >>= 2;
            *output++ = diff.encode(input & 1) | conv.encode((input >> 1) & 1); input >>= 2;
            *output++ = diff.encode(input & 1) | conv.encode((input >> 1) & 1); input >>= 2;
        }
    private:
        differential_encoder diff;
        convolutional_encoder conv;
    };
};

class atsc_trellis_encoder {
public:
    constexpr atsc_trellis_encoder() : z2_carry(), z0a_carry(), z0b_carry() {
        memset(z2_carry.data(), 0, z2_carry.size() * sizeof(z2_carry[0]));
        memset(z0a_carry.data(), 0, z0a_carry.size() * sizeof(z0a_carry[0]));
        memset(z0b_carry.data(), 0, z0b_carry.size() * sizeof(z0b_carry[0]));
    }

    static constexpr unsigned TRELLIS_ENCODERS = 12;
    static constexpr unsigned BYTES_PER_ENCODE = 13;
    static constexpr unsigned ROUNDS = ATSC_DATA_PER_FIELD / (TRELLIS_ENCODERS * BYTES_PER_ENCODE);
    static constexpr unsigned SYMBOLS_PER_ROUND = ATSC_DATA_SYMBOLS_PER_FIELD / ROUNDS;

    void process(atsc_field_symbol_padded& output, atsc_field_data& input) {

        uint8_t* data = input.data();
        for (unsigned i = 0; i < ROUNDS; i++) {
            trellis_encode(output, data, i, 0); data += 13;
            trellis_encode(output, data, i, 1); data += 13;
            trellis_encode(output, data, i, 2); data += 13;
            trellis_encode(output, data, i, 3); data += 13;
            trellis_encode(output, data, i, 4); data += 13;
            trellis_encode(output, data, i, 5); data += 13;
            trellis_encode(output, data, i, 6); data += 13;
            trellis_encode(output, data, i, 7); data += 13;
            trellis_encode(output, data, i, 8); data += 13;
            trellis_encode(output, data, i, 9); data += 13;
            trellis_encode(output, data, i, 10); data += 13;
            trellis_encode(output, data, i, 11); data += 13;
        }
    }

    std::array<std::array<uint8_t, 12>, 3> get_carry() {
        std::array<std::array<uint8_t, 12>, 3> carry;
        for (size_t i = 0; i < 12; i++) {
            carry[0][i] = z2_carry[i] ? 1 : 0;
        }
        for (size_t i = 0; i < 12; i++) {
            carry[1][i] = z0a_carry[i] ? 1 : 0;
        }
        for (size_t i = 0; i < 12; i++) {
            carry[2][i] = z0b_carry[i] ? 1 : 0;
        }
        return carry;
    }

private:
    uint64_t diff_encode_56(uint64_t v) {
        int64_t r;
        r = (int64_t)diff_table[BYTE0(v)] << 0;
        r ^= (int64_t)diff_table[BYTE1(v)] << 8;
        r ^= (int64_t)diff_table[BYTE2(v)] << 16;
        r ^= (int64_t)diff_table[BYTE3(v)] << 24;
        r ^= (int64_t)diff_table[BYTE4(v)] << 32;
        r ^= (int64_t)diff_table[BYTE5(v)] << 40;
        r ^= (int64_t)diff_table[BYTE6(v)] << 48;
        return r;
    }
    uint32_t diff_encode_32(uint32_t v) {
        int32_t r;
        r = (int32_t)diff_half_table[BYTE0(v)] << 0;
        r ^= (int32_t)diff_half_table[BYTE1(v)] << 8;
        r ^= (int32_t)diff_half_table[BYTE2(v)] << 16;
        r ^= (int32_t)diff_half_table[BYTE3(v)] << 24;
        return r;
    }

    // Input is 13 bytes, output is 52 bytes
    void trellis_encode(atsc_field_symbol_padded& out, uint8_t *data, int round, int encoder) {
        // The input data is processed in chunks of twelve bytes
        // each byte is fed to a different trellis encoder
        // Each byte is then split up into bit pairs from MSB to LSB (7,6), (5,4), (3,2), (1,0)
        // and each bit pair is fed to the trellis encoder.  The MSB of the two is fed to the
        // differential encoder and the LSB of the two is fed to the convolutional encoder
        // The input to the differential encoder, will be fed bits 7,5,3,1 and then will
        // be fed the same bit positions from the next byte 12 bytes later.

        // The interleaver will rearrange bytes for us such that instead of the stream
        // producing a single byte for each sequential trellis encoder, it will batch
        // up the data into N bytes per trellis decoder.  N is chose to use as many bits
        // as possible in a 32/64 bit register while making certain that our trellis decoding
        // does not span an atsc data field.  A data field (only data) is 312 segments * 207 bytes/segment
        // and since there are 12 trellis encoders, we pick N to be 13 as (12*13) evenly divides (312*207)
         
        // We aggregate bits as follows (shown for a single trellis encoder)
        // For the differential bits, BYTE[0]:7, BYTE[0]:5, BYTE[0]:3, BYTE[0]:1, BYTE[12]:7, BYTE[12]:5, ...
        // For the convolution encoder, the bits are further subdivided to account for the two delay lines:
        // BYTE[0]:6, BYTE[0]:2, BYTE[12]:6, BYTE[12]:2, BYTE[24]:6, BYTE[24]:2, ...
        // BYTE[0]:4, BYTE[0]:0, BYTE[12]:4, BYTE[12]:0, BYTE[24]:4, BYTE[24]:2, ...

        uint64_t* bits = (uint64_t*)data;
#if HAVE_BMI2
        uint64_t z2  = _pext_u64(bits[0], 0xaaaaaaaaaaaaaaaaULL) |  // e0f0g0h0 a0b0c0d0  needs to become  hgfedcba which needs a nibble bitswap
                      (_pext_u64(bits[1], 0xaaaaaaaaaa) << 32);
#else
        uint64_t _z2 = (bits[0] & 0xaaaaaaaaaaaaaaaaULL) >> 1;
                 _z2 = (_z2 | (_z2 >> 1)) & 0x3333333333333333ULL;
                 _z2 = (_z2 | (_z2 >> 2)) & 0x0f0f0f0f0f0f0f0fULL;
                 _z2 = (_z2 | (_z2 >> 4)) & 0x00ff00ff00ff00ffULL;
                 _z2 = (_z2 | (_z2 >> 8)) & 0x0000ffff0000ffffULL;
                 _z2 = (_z2 | (_z2 >> 16))& 0x00000000ffffffffULL;
        uint64_t _z22 = (bits[1] & 0xaaaaaaaaaa) >> 1;
                 _z22 = (_z22 | (_z22 >> 1)) & 0x3333333333333333ULL;
                 _z22 = (_z22 | (_z22 >> 2)) & 0x0f0f0f0f0f0f0f0fULL;
                 _z22 = (_z22 | (_z22 >> 4)) & 0x00ff00ff00ff00ffULL;
                 _z22 = (_z22 | (_z22 >> 8)) & 0x0000ffff0000ffffULL;
                 _z22 = (_z22 | (_z22 >> 16))& 0x00000000ffffffffULL;
        uint64_t z2 = (_z22 << 32) | _z2;
        //uint64_t __z2  = _pext_u64(bits[0], 0xaaaaaaaaaaaaaaaaULL) |  // e0f0g0h0 a0b0c0d0  needs to become  hgfedcba which needs a nibble bitswap
        //        (_pext_u64(bits[1], 0xaaaaaaaaaa) << 32);
        //assert(z2 == __z2);

#endif
#if HAVE_BMI2
        uint64_t z1  = _pext_u64(bits[0], 0x5555555555555555ULL) |  // ditto for swap
                      (_pext_u64(bits[1], 0x5555555555) << 32);
#else
        uint64_t _z1 = (bits[0] & 0x5555555555555555ULL);
                 _z1 = (_z1 | (_z1 >> 1)) & 0x3333333333333333ULL;
                 _z1 = (_z1 | (_z1 >> 2)) & 0x0f0f0f0f0f0f0f0fULL;
                 _z1 = (_z1 | (_z1 >> 4)) & 0x00ff00ff00ff00ffULL;
                 _z1 = (_z1 | (_z1 >> 8)) & 0x0000ffff0000ffffULL;
                 _z1 = (_z1 | (_z1 >> 16))& 0x00000000ffffffffULL;
        uint64_t _z12 = (bits[1] & 0x5555555555);
                 _z12 = (_z12 | (_z12 >> 1)) & 0x3333333333333333ULL;
                 _z12 = (_z12 | (_z12 >> 2)) & 0x0f0f0f0f0f0f0f0fULL;
                 _z12 = (_z12 | (_z12 >> 4)) & 0x00ff00ff00ff00ffULL;
                 _z12 = (_z12 | (_z12 >> 8)) & 0x0000ffff0000ffffULL;
                 _z12 = (_z12 | (_z12 >> 16))& 0x00000000ffffffffULL;
        uint64_t z1 = (_z12 << 32) | _z1;
        //uint64_t __z1  = _pext_u64(bits[0], 0x5555555555555555ULL) |  // ditto for swap
        //              (_pext_u64(bits[1], 0x5555555555) << 32);
        //assert(z1 == __z1);
#endif
#if HAVE_BMI2
        uint32_t z0a = _pext_u64(bits[0], 0x4444444444444444ULL) |  // 0g000h00 0e000f00 0c000d00 0a000b00 needs a half-nibble bitswap
                      (_pext_u64(bits[1], 0x4444444444) << 16);
#else
        uint64_t _z0a_low = (bits[0] & 0x4444444444444444ULL) >> 2;
                 _z0a_low = (_z0a_low | (_z0a_low >> 3)) & 0x0303030303030303ULL;
                 _z0a_low = (_z0a_low | (_z0a_low >> 6)) & 0x000f000f000f000fULL;
                 _z0a_low = (_z0a_low | (_z0a_low >>12)) & 0x000000ff000000ffULL;
                 _z0a_low = (_z0a_low | (_z0a_low >>24)) & 0x000000000000ffffULL;
        uint64_t _z0a_hi  = (bits[1]  & 0x4444444444) >> 2;
                 _z0a_hi = (_z0a_hi | (_z0a_hi >> 3)) & 0x0303030303030303ULL;
                 _z0a_hi = (_z0a_hi | (_z0a_hi >> 6)) & 0x000f000f000f000fULL;
                 _z0a_hi = (_z0a_hi | (_z0a_hi >>12)) & 0x000000ff000000ffULL;
                 _z0a_hi = (_z0a_hi | (_z0a_hi >>24)) & 0x000000000000ffffULL;
        uint32_t z0a = (_z0a_hi << 16) | _z0a_low;
        //uint32_t _z0a = _pext_u64(bits[0], 0x4444444444444444ULL) |  // 0g000h00 0e000f00 0c000d00 0a000b00 needs a half-nibble bitswap
        //              (_pext_u64(bits[1], 0x4444444444) << 16);
        //assert(z0a == _z0a);
#endif
#if HAVE_BMI2
        uint32_t z0b = _pext_u64(bits[0], 0x1111111111111111ULL) |
                      (_pext_u64(bits[1], 0x1111111111) << 16);
#else
        uint64_t _z0b_low = (bits[0] & 0x1111111111111111ULL);
                 _z0b_low = (_z0b_low | (_z0b_low >> 3)) & 0x0303030303030303ULL;
                 _z0b_low = (_z0b_low | (_z0b_low >> 6)) & 0x000f000f000f000fULL;
                 _z0b_low = (_z0b_low | (_z0b_low >>12)) & 0x000000ff000000ffULL;
                 _z0b_low = (_z0b_low | (_z0b_low >>24)) & 0x000000000000ffffULL;
        uint64_t _z0b_hi  = (bits[1]  & 0x1111111111);
                 _z0b_hi = (_z0b_hi | (_z0b_hi >> 3)) & 0x0303030303030303ULL;
                 _z0b_hi = (_z0b_hi | (_z0b_hi >> 6)) & 0x000f000f000f000fULL;
                 _z0b_hi = (_z0b_hi | (_z0b_hi >>12)) & 0x000000ff000000ffULL;
                 _z0b_hi = (_z0b_hi | (_z0b_hi >>24)) & 0x000000000000ffffULL;
        uint32_t z0b = (_z0b_hi << 16) | _z0b_low;
        //uint32_t _z0b = _pext_u64(bits[0], 0x1111111111111111ULL) |
        //              (_pext_u64(bits[1], 0x1111111111) << 16);
        //assert(z0b == _z0b);
#endif
    
        // Differentially encode z2, z0a, z0b
        z2 = diff_encode_56(z2) ^ z2_carry[encoder];

        // nibble bitswap z1
        z1 = (z1 & 0x8888888888888888ULL) >> 3 |
             (z1 & 0x4444444444444444ULL) >> 1 |
             (z1 & 0x2222222222222222ULL) << 1 |
             (z1 & 0x1111111111111111ULL) << 3;

        z0a = diff_encode_32(z0a) ^ z0a_carry[encoder];
        z0b = (diff_encode_32(z0b) << 1) ^ z0b_carry[encoder];

        // compute new carry, diff_encode_* functions above automagically sign extend
        // so the carry is in the top bit
        z2_carry[encoder] = ((int64_t)z2) >> 63;
        z0a_carry[encoder] = ((int32_t)z0a) >> 31;
        z0b_carry[encoder] = ((int32_t)z0b) >> 31;

        using xformer = atsc_symbol_to_signal<atsc_symbol_type>;

        // Redistribute bits
        uint32_t start = round * SYMBOLS_PER_ROUND + encoder * 52;
        uint64_t results;
#if HAVE_BMI2
        results = _pdep_u64(z2,  0x0404040404040404ULL) |
                  _pdep_u64(z1,  0x0202020202020202ULL) |
                  _pdep_u64(z0a, 0x0100010001000100ULL) |
                  _pdep_u64(z0b, 0x0001000100010001ULL);
#else
        {
            uint64_t r_z2 = (z2 & 0xff);
                    r_z2 = ((r_z2 << 28) | r_z2) & 0x0000000f0000000f;
                    r_z2 = ((r_z2 << 14) | r_z2) & 0x0003000300030003;
                    r_z2 = ((r_z2 <<  7) | r_z2) & 0x0101010101010101;
                    r_z2 <<= 2;
            uint64_t r_z1 = (z1 & 0xff);
                    r_z1 = ((r_z1 << 28) | r_z1) & 0x0000000f0000000f;
                    r_z1 = ((r_z1 << 14) | r_z1) & 0x0003000300030003;
                    r_z1 = ((r_z1 <<  7) | r_z1) & 0x0101010101010101;
                    r_z1 <<= 1;
            uint64_t r_z0a = (z0a & 0xf);
                    r_z0a = ((r_z0a << 30) | r_z0a) & 0x0000000300000003;
                    r_z0a = ((r_z0a << 15) | r_z0a) & 0x0001000100010001;
                    r_z0a <<= 8;
            uint64_t r_z0b = (z0b & 0xf);
                    r_z0b = ((r_z0b << 30) | r_z0b) & 0x0000000300000003;
                    r_z0b = ((r_z0b << 15) | r_z0b) & 0x0001000100010001;
            results = r_z2 | r_z1 | r_z0a | r_z0b;
        }
#endif
        out[output_table[start +  0]] = xformer::xform(BYTE0(results));
        out[output_table[start +  1]] = xformer::xform(BYTE1(results));
        out[output_table[start +  2]] = xformer::xform(BYTE2(results));
        out[output_table[start +  3]] = xformer::xform(BYTE3(results));
        out[output_table[start +  4]] = xformer::xform(BYTE4(results));
        out[output_table[start +  5]] = xformer::xform(BYTE5(results));
        out[output_table[start +  6]] = xformer::xform(BYTE6(results));
        out[output_table[start +  7]] = xformer::xform(BYTE7(results));

        z2 >>= 8; z1 >>= 8; z0a >>=4; z0b >>= 4;
#if HAVE_BMI2
        results = _pdep_u64(z2,  0x0404040404040404ULL) |
                  _pdep_u64(z1,  0x0202020202020202ULL) |
                  _pdep_u64(z0a, 0x0100010001000100ULL) |
                  _pdep_u64(z0b, 0x0001000100010001ULL);
#else
        {
            uint64_t r_z2 = (z2 & 0xff);
                    r_z2 = ((r_z2 << 28) | r_z2) & 0x0000000f0000000f;
                    r_z2 = ((r_z2 << 14) | r_z2) & 0x0003000300030003;
                    r_z2 = ((r_z2 <<  7) | r_z2) & 0x0101010101010101;
                    r_z2 <<= 2;
            uint64_t r_z1 = (z1 & 0xff);
                    r_z1 = ((r_z1 << 28) | r_z1) & 0x0000000f0000000f;
                    r_z1 = ((r_z1 << 14) | r_z1) & 0x0003000300030003;
                    r_z1 = ((r_z1 <<  7) | r_z1) & 0x0101010101010101;
                    r_z1 <<= 1;
            uint64_t r_z0a = (z0a & 0xf);
                    r_z0a = ((r_z0a << 30) | r_z0a) & 0x0000000300000003;
                    r_z0a = ((r_z0a << 15) | r_z0a) & 0x0001000100010001;
                    r_z0a <<= 8;
            uint64_t r_z0b = (z0b & 0xf);
                    r_z0b = ((r_z0b << 30) | r_z0b) & 0x0000000300000003;
                    r_z0b = ((r_z0b << 15) | r_z0b) & 0x0001000100010001;
            results = r_z2 | r_z1 | r_z0a | r_z0b;
        }
#endif
        out[output_table[start +  8]] = xformer::xform(BYTE0(results));
        out[output_table[start +  9]] = xformer::xform(BYTE1(results));
        out[output_table[start + 10]] = xformer::xform(BYTE2(results));
        out[output_table[start + 11]] = xformer::xform(BYTE3(results));
        out[output_table[start + 12]] = xformer::xform(BYTE4(results));
        out[output_table[start + 13]] = xformer::xform(BYTE5(results));
        out[output_table[start + 14]] = xformer::xform(BYTE6(results));
        out[output_table[start + 15]] = xformer::xform(BYTE7(results));

        z2 >>= 8; z1 >>= 8; z0a >>=4; z0b >>= 4;
#if HAVE_BMI2
        results = _pdep_u64(z2,  0x0404040404040404ULL) |
                  _pdep_u64(z1,  0x0202020202020202ULL) |
                  _pdep_u64(z0a, 0x0100010001000100ULL) |
                  _pdep_u64(z0b, 0x0001000100010001ULL);
#else
        {
            uint64_t r_z2 = (z2 & 0xff);
                    r_z2 = ((r_z2 << 28) | r_z2) & 0x0000000f0000000f;
                    r_z2 = ((r_z2 << 14) | r_z2) & 0x0003000300030003;
                    r_z2 = ((r_z2 <<  7) | r_z2) & 0x0101010101010101;
                    r_z2 <<= 2;
            uint64_t r_z1 = (z1 & 0xff);
                    r_z1 = ((r_z1 << 28) | r_z1) & 0x0000000f0000000f;
                    r_z1 = ((r_z1 << 14) | r_z1) & 0x0003000300030003;
                    r_z1 = ((r_z1 <<  7) | r_z1) & 0x0101010101010101;
                    r_z1 <<= 1;
            uint64_t r_z0a = (z0a & 0xf);
                    r_z0a = ((r_z0a << 30) | r_z0a) & 0x0000000300000003;
                    r_z0a = ((r_z0a << 15) | r_z0a) & 0x0001000100010001;
                    r_z0a <<= 8;
            uint64_t r_z0b = (z0b & 0xf);
                    r_z0b = ((r_z0b << 30) | r_z0b) & 0x0000000300000003;
                    r_z0b = ((r_z0b << 15) | r_z0b) & 0x0001000100010001;
            results = r_z2 | r_z1 | r_z0a | r_z0b;
        }
#endif
        out[output_table[start + 16]] = xformer::xform(BYTE0(results));
        out[output_table[start + 17]] = xformer::xform(BYTE1(results));
        out[output_table[start + 18]] = xformer::xform(BYTE2(results));
        out[output_table[start + 19]] = xformer::xform(BYTE3(results));
        out[output_table[start + 20]] = xformer::xform(BYTE4(results));
        out[output_table[start + 21]] = xformer::xform(BYTE5(results));
        out[output_table[start + 22]] = xformer::xform(BYTE6(results));
        out[output_table[start + 23]] = xformer::xform(BYTE7(results));

        z2 >>= 8; z1 >>= 8; z0a >>=4; z0b >>= 4;
#if HAVE_BMI2
        results = _pdep_u64(z2,  0x0404040404040404ULL) |
                  _pdep_u64(z1,  0x0202020202020202ULL) |
                  _pdep_u64(z0a, 0x0100010001000100ULL) |
                  _pdep_u64(z0b, 0x0001000100010001ULL);
#else
        {
            uint64_t r_z2 = (z2 & 0xff);
                    r_z2 = ((r_z2 << 28) | r_z2) & 0x0000000f0000000f;
                    r_z2 = ((r_z2 << 14) | r_z2) & 0x0003000300030003;
                    r_z2 = ((r_z2 <<  7) | r_z2) & 0x0101010101010101;
                    r_z2 <<= 2;
            uint64_t r_z1 = (z1 & 0xff);
                    r_z1 = ((r_z1 << 28) | r_z1) & 0x0000000f0000000f;
                    r_z1 = ((r_z1 << 14) | r_z1) & 0x0003000300030003;
                    r_z1 = ((r_z1 <<  7) | r_z1) & 0x0101010101010101;
                    r_z1 <<= 1;
            uint64_t r_z0a = (z0a & 0xf);
                    r_z0a = ((r_z0a << 30) | r_z0a) & 0x0000000300000003;
                    r_z0a = ((r_z0a << 15) | r_z0a) & 0x0001000100010001;
                    r_z0a <<= 8;
            uint64_t r_z0b = (z0b & 0xf);
                    r_z0b = ((r_z0b << 30) | r_z0b) & 0x0000000300000003;
                    r_z0b = ((r_z0b << 15) | r_z0b) & 0x0001000100010001;
            results = r_z2 | r_z1 | r_z0a | r_z0b;
        }
#endif
        out[output_table[start + 24]] = xformer::xform(BYTE0(results));
        out[output_table[start + 25]] = xformer::xform(BYTE1(results));
        out[output_table[start + 26]] = xformer::xform(BYTE2(results));
        out[output_table[start + 27]] = xformer::xform(BYTE3(results));
        out[output_table[start + 28]] = xformer::xform(BYTE4(results));
        out[output_table[start + 29]] = xformer::xform(BYTE5(results));
        out[output_table[start + 30]] = xformer::xform(BYTE6(results));
        out[output_table[start + 31]] = xformer::xform(BYTE7(results));

        z2 >>= 8; z1 >>= 8; z0a >>=4; z0b >>= 4;
#if HAVE_BMI2
        results = _pdep_u64(z2,  0x0404040404040404ULL) |
                  _pdep_u64(z1,  0x0202020202020202ULL) |
                  _pdep_u64(z0a, 0x0100010001000100ULL) |
                  _pdep_u64(z0b, 0x0001000100010001ULL);
#else
        {
            uint64_t r_z2 = (z2 & 0xff);
                    r_z2 = ((r_z2 << 28) | r_z2) & 0x0000000f0000000f;
                    r_z2 = ((r_z2 << 14) | r_z2) & 0x0003000300030003;
                    r_z2 = ((r_z2 <<  7) | r_z2) & 0x0101010101010101;
                    r_z2 <<= 2;
            uint64_t r_z1 = (z1 & 0xff);
                    r_z1 = ((r_z1 << 28) | r_z1) & 0x0000000f0000000f;
                    r_z1 = ((r_z1 << 14) | r_z1) & 0x0003000300030003;
                    r_z1 = ((r_z1 <<  7) | r_z1) & 0x0101010101010101;
                    r_z1 <<= 1;
            uint64_t r_z0a = (z0a & 0xf);
                    r_z0a = ((r_z0a << 30) | r_z0a) & 0x0000000300000003;
                    r_z0a = ((r_z0a << 15) | r_z0a) & 0x0001000100010001;
                    r_z0a <<= 8;
            uint64_t r_z0b = (z0b & 0xf);
                    r_z0b = ((r_z0b << 30) | r_z0b) & 0x0000000300000003;
                    r_z0b = ((r_z0b << 15) | r_z0b) & 0x0001000100010001;
            results = r_z2 | r_z1 | r_z0a | r_z0b;
        }
#endif
        out[output_table[start + 32]] = xformer::xform(BYTE0(results));
        out[output_table[start + 33]] = xformer::xform(BYTE1(results));
        out[output_table[start + 34]] = xformer::xform(BYTE2(results));
        out[output_table[start + 35]] = xformer::xform(BYTE3(results));
        out[output_table[start + 36]] = xformer::xform(BYTE4(results));
        out[output_table[start + 37]] = xformer::xform(BYTE5(results));
        out[output_table[start + 38]] = xformer::xform(BYTE6(results));
        out[output_table[start + 39]] = xformer::xform(BYTE7(results));

        z2 >>= 8; z1 >>= 8; z0a >>=4; z0b >>= 4;
#if HAVE_BMI2
        results = _pdep_u64(z2,  0x0404040404040404ULL) |
                  _pdep_u64(z1,  0x0202020202020202ULL) |
                  _pdep_u64(z0a, 0x0100010001000100ULL) |
                  _pdep_u64(z0b, 0x0001000100010001ULL);
#else
        {
            uint64_t r_z2 = (z2 & 0xff);
                    r_z2 = ((r_z2 << 28) | r_z2) & 0x0000000f0000000f;
                    r_z2 = ((r_z2 << 14) | r_z2) & 0x0003000300030003;
                    r_z2 = ((r_z2 <<  7) | r_z2) & 0x0101010101010101;
                    r_z2 <<= 2;
            uint64_t r_z1 = (z1 & 0xff);
                    r_z1 = ((r_z1 << 28) | r_z1) & 0x0000000f0000000f;
                    r_z1 = ((r_z1 << 14) | r_z1) & 0x0003000300030003;
                    r_z1 = ((r_z1 <<  7) | r_z1) & 0x0101010101010101;
                    r_z1 <<= 1;
            uint64_t r_z0a = (z0a & 0xf);
                    r_z0a = ((r_z0a << 30) | r_z0a) & 0x0000000300000003;
                    r_z0a = ((r_z0a << 15) | r_z0a) & 0x0001000100010001;
                    r_z0a <<= 8;
            uint64_t r_z0b = (z0b & 0xf);
                    r_z0b = ((r_z0b << 30) | r_z0b) & 0x0000000300000003;
                    r_z0b = ((r_z0b << 15) | r_z0b) & 0x0001000100010001;
            results = r_z2 | r_z1 | r_z0a | r_z0b;
        }
#endif
        out[output_table[start + 40]] = xformer::xform(BYTE0(results));
        out[output_table[start + 41]] = xformer::xform(BYTE1(results));
        out[output_table[start + 42]] = xformer::xform(BYTE2(results));
        out[output_table[start + 43]] = xformer::xform(BYTE3(results));
        out[output_table[start + 44]] = xformer::xform(BYTE4(results));
        out[output_table[start + 45]] = xformer::xform(BYTE5(results));
        out[output_table[start + 46]] = xformer::xform(BYTE6(results));
        out[output_table[start + 47]] = xformer::xform(BYTE7(results));

        z2 >>= 8; z1 >>= 8; z0a >>=4; z0b >>= 4;
#if HAVE_BMI2
        results = _pdep_u32(z2,  0x04040404) |
                  _pdep_u32(z1,  0x02020202) |
                  _pdep_u32(z0a, 0x01000100) |
                  _pdep_u32(z0b, 0x00010001);
#else
        {
            uint32_t r_z2 = (z2 & 0xf);
                    r_z2 = ((r_z2 << 14) | r_z2) & 0x00030003;
                    r_z2 = ((r_z2 <<  7) | r_z2) & 0x01010101;
                    r_z2 <<= 2;
            uint32_t r_z1 = (z1 & 0xf);
                    r_z1 = ((r_z1 << 14) | r_z1) & 0x00030003;
                    r_z1 = ((r_z1 <<  7) | r_z1) & 0x01010101;
                    r_z1 <<= 1;
            uint32_t r_z0a = (z0a & 0x3);
                    r_z0a = ((r_z0a << 15) | r_z0a) & 0x00010001;
                    r_z0a <<= 8;
            uint32_t r_z0b = (z0b & 0x3);
                    r_z0b = ((r_z0b << 15) | r_z0b) & 0x00010001;
            results = r_z2 | r_z1 | r_z0a | r_z0b;
        }
#endif
        out[output_table[start + 48]] = xformer::xform(BYTE0(results));
        out[output_table[start + 49]] = xformer::xform(BYTE1(results));
        out[output_table[start + 50]] = xformer::xform(BYTE2(results));
        out[output_table[start + 51]] = xformer::xform(BYTE3(results));

    }
    std::array<int64_t, 12> z2_carry;
    std::array<int32_t, 12> z0a_carry;
    std::array<int32_t, 12> z0b_carry;

    struct diff_table_initializer {
        diff_table_initializer() : table() {
            for (int i = 0; i < 256; i++) {
                uint8_t bits = (uint8_t)i;

                // nibble bitswap
                bits = ((bits & 0x88) >> 3) |
                       ((bits & 0x44) >> 1) |
                       ((bits & 0x22) << 1) |
                       ((bits & 0x11) << 3);
                uint8_t encoded = 0;

                uint8_t carry = 0;
                for (int j = 0; j < 8; j++) {
                    carry = (carry ^ bits) & 1;
                    bits >>= 1;
                    encoded >>= 1;
                    encoded |= carry << 7;
                }
                table[i] = encoded;

                bits = (uint8_t)i;
                // half nibble bitswap
                bits = ((bits & 0xaa) >> 1) |
                       ((bits & 0x55) << 1);
                encoded = 0;

                carry = 0;
                for (int j = 0; j < 8; j++) {
                    carry = (carry ^ bits) & 1;
                    bits >>= 1;
                    encoded >>= 1;
                    encoded |= carry << 7;
                }
                half_table[i] = encoded;

            }
        }
        std::array<int8_t, 256> table;
        std::array<int8_t, 256> half_table;
    };

    static inline std::array<int8_t, 256> diff_table = diff_table_initializer().table;
    static inline std::array<int8_t, 256> diff_half_table = diff_table_initializer().half_table;

    struct output_table_initializer {
        static inline constexpr size_t len = ATSC_DATA_SYMBOLS_PER_SEGMENT * ATSC_DATA_SEGMENTS;

        output_table_initializer() : table() {

            for (size_t index = 0; index < len; index++) {
                unsigned dseg = index / ATSC_DATA_SYMBOLS_PER_SEGMENT;
                unsigned dseg_offset = index % ATSC_DATA_SYMBOLS_PER_SEGMENT;

                unsigned start_trellis = (dseg * 4) % 12;
                unsigned trellis = (start_trellis + index) % 12;
                unsigned trellis_index = (index / 12) % 52;

                // The trellis output data is organized as 52 symbols per trellis
                unsigned chunk = index / (52 * 12);
                unsigned offset = chunk * 52 * 12 + trellis * 52 + trellis_index;

                // index is ordered output of the trellis output commutator
                // insert space for sync 
                unsigned adjusted_index = (dseg + 1) * ATSC_SYMBOLS_PER_SEGMENT + 4 + dseg_offset;

                table[offset] = adjusted_index;
            }
        }

        std::array<uint32_t, len> table;
    };
    static inline std::array<uint32_t, output_table_initializer::len> output_table = output_table_initializer().table;
};