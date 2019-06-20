#pragma once
#include "opencl.h"
#include "common/atsc_parameters.h"
#include "common/differential.h"
#include "common/trellis_output_commutator.h"
#include "common/string.h"

struct atsc_trellis_cl : virtual opencl_base {

tprotected:
    cl_event trellis(cl_array<atsc_symbol_type, ATSC_SYMBOLS_PER_FIELD + ATSC_SYMBOLS_PER_SEGMENT> output, 
                     cl_array<uint8_t, ATSC_DATA_PER_FIELD> input, cl_event event = nullptr)
    {
        gpuErrchk(clSetKernelArg(_trellis_sort_bits, 0, sizeof(cl_mem), &_trellis_bits));
        gpuErrchk(clSetKernelArg(_trellis_sort_bits, 1, sizeof(cl_mem), &input.data()));
        size_t work[] = { rounded_input_per_trellis/sizeof(uint32_t), ATSC_TRELLIS_ENCODERS, differential_encoders };
        event = start_kernel(_trellis_sort_bits, 3, &work[0], event);

        gpuErrchk(clSetKernelArg(_trellis_popcount, 0, sizeof(cl_mem), &_trellis_popcount1));
        gpuErrchk(clSetKernelArg(_trellis_popcount, 1, sizeof(cl_mem), &_trellis_bits));
        size_t pop_work[] = { trellis_popcount_size, ATSC_TRELLIS_ENCODERS, 4 };
        event = start_kernel(_trellis_popcount, 3, &pop_work[0], event);

        size_t sum_work[] = { trellis_popcount_size + 1, ATSC_TRELLIS_ENCODERS, differential_encoders - 1 };
        for (uint32_t stride = 1; stride < trellis_popcount_size + 1; stride <<= 1) {
            gpuErrchk(clSetKernelArg(_trellis_popcount_sum, 0, sizeof(cl_mem), &_trellis_popcount2));
            gpuErrchk(clSetKernelArg(_trellis_popcount_sum, 1, sizeof(cl_mem), &_trellis_popcount1));
            gpuErrchk(clSetKernelArg(_trellis_popcount_sum, 2, sizeof(uint32_t), &stride));
            event = start_kernel(_trellis_popcount_sum, 3, &sum_work[0], event);

            std::swap(_trellis_popcount1, _trellis_popcount2);
        }

        gpuErrchk(clSetKernelArg(_trellis, 0, sizeof(cl_mem), &_trellis_bits));
        gpuErrchk(clSetKernelArg(_trellis, 1, sizeof(cl_mem), &_trellis_popcount1));
        gpuErrchk(clSetKernelArg(_trellis, 2, sizeof(cl_mem), &_diff_table));
        gpuErrchk(clSetKernelArg(_trellis, 3, sizeof(cl_mem), &_diff_table2));
        size_t diff_work[] = { trellis_popcount_size, ATSC_TRELLIS_ENCODERS, differential_encoders - 1 };
        event = start_kernel(_trellis, 3, &diff_work[0], event);

        gpuErrchk(clSetKernelArg(_trellis_encode_signal, 0, sizeof(cl_mem), &output.data()));
        gpuErrchk(clSetKernelArg(_trellis_encode_signal, 1, sizeof(cl_mem), &_trellis_bits));
        gpuErrchk(clSetKernelArg(_trellis_encode_signal, 2, sizeof(cl_mem), &_trellis_popcount1));
        gpuErrchk(clSetKernelArg(_trellis_encode_signal, 3, sizeof(cl_mem), &_output_commutator_table));
        size_t encode_work[] = { ATSC_TRELLIS_INPUT_OPENCL * ATSC_SYMBOLS_PER_BYTE, ATSC_TRELLIS_ENCODERS };
        return start_kernel(_trellis_encode_signal, 2, &encode_work[0], event);
    }

tprivate:
    std::array<std::array<uint8_t, 12>, 3> get_carry() {
        auto popcnt = new uint8_t[ATSC_TRELLIS_ENCODERS][differential_encoders][trellis_popcount_size + 1];
        to_host(popcnt, _trellis_popcount1, (trellis_popcount_size + 1) * differential_encoders * ATSC_TRELLIS_ENCODERS);
        std::array<std::array<uint8_t, 12>, 3> carry;
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 12; j++) {
                carry[i][j] = popcnt[j][i > 0?i+1:i][0];
            }
        }
        delete[] popcnt;
        return carry;
    }


protected:
    atsc_trellis_cl() : opencl_base() {

        _trellis_bits = cl_alloc(diff_encoder_table_size * differential_encoders * ATSC_TRELLIS_ENCODERS);
        cl_memset(_trellis_bits, diff_encoder_table_size * differential_encoders * ATSC_TRELLIS_ENCODERS);

        _trellis_popcount1 = cl_alloc((trellis_popcount_size + 1) * ATSC_TRELLIS_ENCODERS * differential_encoders);
        _trellis_popcount2 = cl_alloc((trellis_popcount_size + 1) * ATSC_TRELLIS_ENCODERS * differential_encoders);
        cl_memset(_trellis_popcount1, (trellis_popcount_size + 1) * ATSC_TRELLIS_ENCODERS * differential_encoders);

        // differential encoder table
        _diff_table = cl_alloc_ro(_differential_table.table.size());
        _diff_table2 = cl_alloc_ro(_differential_table.table2.size());
        to_device(_diff_table, _differential_table.table.data(), _differential_table.table.size());
        to_device(_diff_table2, _differential_table.table2.data(), _differential_table.table2.size());

        // trellis output commutator
        _output_commutator_table = cl_alloc_ro(_trellis_output_commutator_table.table.size() * sizeof(uint32_t));
        to_device(_output_commutator_table, _trellis_output_commutator_table.table.data(), _trellis_output_commutator_table.table.size() * sizeof(uint32_t));

        compile();
    }

    ~atsc_trellis_cl() {
        clReleaseKernel(_trellis);
        clReleaseKernel(_trellis_sort_bits);
        clReleaseKernel(_trellis_popcount);
        clReleaseKernel(_trellis_popcount_sum);
        clReleaseKernel(_trellis_encode_signal);
        clReleaseProgram(_program);
        clReleaseMemObject(_trellis_bits);
        clReleaseMemObject(_trellis_popcount1);
        clReleaseMemObject(_trellis_popcount2);
        clReleaseMemObject(_diff_table);
        clReleaseMemObject(_diff_table2);
        clReleaseMemObject(_output_commutator_table);
    }


private:
    static constexpr unsigned input_per_trellis = ATSC_DATA_PER_FIELD/ATSC_TRELLIS_ENCODERS;
    static constexpr unsigned rounded_input_per_trellis = (input_per_trellis + 3) & ~3;
    static constexpr unsigned diff_encoder_table_size = ((input_per_trellis + 7) & ~7) / 2;
    static constexpr uint32_t final_input_mask = UINT32_MAX >> (8 * (rounded_input_per_trellis - input_per_trellis));
    static constexpr unsigned trellis_popcount_size = diff_encoder_table_size / sizeof(uint32_t);
    static constexpr unsigned differential_encoders = 4;
    static constexpr unsigned bits_per_z12_table = input_per_trellis * 8 / 2;
    static constexpr unsigned bits_per_z0_table = input_per_trellis * 8 / 4;

    differential_table<void> _differential_table;
    trellis_output_commutator<ATSC_TRELLIS_INPUT_OPENCL> _trellis_output_commutator_table;

    cl_program _program;
    cl_kernel _trellis;
    cl_kernel _trellis_sort_bits;
    cl_kernel _trellis_popcount;
    cl_kernel _trellis_popcount_sum;
    cl_kernel _trellis_encode_signal;

    cl_mem _trellis_bits;
    cl_mem _trellis_popcount1;
    cl_mem _trellis_popcount2;
    cl_mem _diff_table;
    cl_mem _diff_table2;
    cl_mem _output_commutator_table;

    void compile() {
        _program = compile_program(kernel_prefix + R"#(
#define BYTEN(v, b) (((v) >> (b*8)) & 0xff)
#define BYTE0(v) BYTEN(v, 0)
#define BYTE1(v) BYTEN(v, 1)
#define BYTE2(v) BYTEN(v, 2)
#define BYTE3(v) BYTEN(v, 3)

__kernel void trellis_sort_bits(__global uint8_t* bits, __global uint8_t* input) {
    const int kind = get_global_id(2);
    const int trellis = get_global_id(1);
    const int trellis_index = get_global_id(0);

    const unsigned input_per_trellis = )#" + string_from<unsigned, input_per_trellis>::value + R"#(;
    const unsigned diff_encoder_table_size = )#" + string_from<unsigned, diff_encoder_table_size>::value + R"#(;
    const __global uint16_t* trellis_base = (const __global uint16_t*)(input + input_per_trellis * trellis);
    const __global uint8_t* output_base = (const __global uint8_t*)(bits + diff_encoder_table_size * (4*trellis + kind));

    uint32_t value = trellis_base[2 * trellis_index];
             value |= (uint32_t)trellis_base[2 * trellis_index + 1] << 16;
             
    if (trellis_index == get_global_size(0) - 1) {
        // Mask off the relevent bits.  We're reading 32bit chunks so the last index will overlap the first index of the 
        // next trellis
        const uint32_t mask = )#" + string_from<uint32_t, final_input_mask>::value + R"#(;
        value &= mask;
    }

    if (kind == 0) {
        // Calculate Z2
        uint32_t z2 = (value & 0xaaaaaaaaUL) >> 1;
                z2 = (z2 | (z2 >> 1)) & 0x33333333ULL;
                z2 = (z2 | (z2 >> 2)) & 0x0f0f0f0fULL;
                z2 = (z2 | (z2 >> 4)) & 0x00ff00ffULL;
                z2 = (z2 | (z2 >> 8)) & 0x0000ffffULL;

        __global uint16_t* output = (__global uint16_t*)output_base;
        output[trellis_index] = z2;

    } else if (kind == 1) {
        // Calculate Z1
        uint32_t z1 = value & 0x55555555UL;
                z1 = (z1 | (z1 >> 1)) & 0x33333333ULL;
                z1 = (z1 | (z1 >> 2)) & 0x0f0f0f0fULL;
                z1 = (z1 | (z1 >> 4)) & 0x00ff00ffULL;
                z1 = (z1 | (z1 >> 8)) & 0x0000ffffULL;

        // nibble swap (z2, z0a, z0b have this built into their tables)
        z1 = ((z1 & 0x8888) >> 3)
           | ((z1 & 0x4444) >> 1)
           | ((z1 & 0x2222) << 1)
           | ((z1 & 0x1111) << 3);

        __global uint16_t* output = (__global uint16_t*)output_base;
        output[trellis_index] = z1;

    } else if (kind == 2) {
        // Calculate Z0_a
        uint32_t z0a = (value & 0x44444444UL) >> 2;
                z0a = (z0a | (z0a >> 3)) & 0x03030303ULL;
                z0a = (z0a | (z0a >> 6)) & 0x000f000fULL;
                z0a = (z0a | (z0a >>12)) & 0x000000ffULL;

        __global uint8_t* output = (__global uint8_t*)output_base;

        output[trellis_index] = z0a;
    } else {
        // Calculate Z0_b
        uint32_t z0b = value & 0x11111111UL;
                z0b = (z0b | (z0b >> 3)) & 0x03030303ULL;
                z0b = (z0b | (z0b >> 6)) & 0x000f000fULL;
                z0b = (z0b | (z0b >>12)) & 0x000000ffULL;

        __global uint8_t* output = (__global uint8_t*)output_base;

        output[trellis_index] = z0b;
    }
}

__kernel void trellis_popcount_sum(__global uint8_t* output, __global uint8_t* input, uint32_t stride) {
    int kind = get_global_id(2);
    const unsigned trellis = get_global_id(1);
    const unsigned index = get_global_id(0);
    const unsigned trellis_popcount_size = )#" + string_from<unsigned, trellis_popcount_size+1>::value + R"#(;

    if (kind > 0)
        kind ++;

    __global uint8_t* popcount_output = (__global uint8_t*)(output + trellis_popcount_size * (4*trellis + kind));
    __global uint8_t* popcount_input = (__global uint8_t*)(input + trellis_popcount_size * (4*trellis + kind));

    uint8_t r;
    uint8_t a = popcount_input[index];
    uint8_t b = 0;
    if (index >= stride) {
        b = popcount_input[index-stride];
    }
    r = (a + b) & 1;
    popcount_output[index] = r;
}

__kernel void trellis_popcount(__global uint8_t* output, __global uint32_t* input) {
    const int kind = get_global_id(2);
    const unsigned trellis = get_global_id(1);
    const unsigned index = get_global_id(0);
    const unsigned trellis_popcount_size = )#" + string_from<unsigned, trellis_popcount_size+1>::value + R"#(;

    __global uint8_t* popcount_output = (__global uint8_t*)(output + trellis_popcount_size * (4*trellis + kind) + 1);
    __global uint32_t* trellis_input = (__global uint32_t*)(input + get_global_size(0) * (4*trellis + kind));

    popcount_output[index] = popcount(trellis_input[index]) & 1;
}

__kernel void trellis(__global uint32_t* trellis_table, __global uint8_t* popcount_table, __global int8_t* z2_diff_table, __global int8_t* z0_diff_table) {

    int kind = get_global_id(2);
    const unsigned trellis = get_global_id(1);
    const unsigned index = get_global_id(0);
    const int sz_0 = get_global_size(0);

    // We don't waste threads on Z1, so Z0A and Z0B need to get incremented
    // to their proper index
    if (kind > 0)
        kind++;

    __global uint8_t* popcount_base = (__global uint8_t*)(popcount_table + (sz_0 + 1) * (4*trellis + kind)); 
    __global uint32_t* trellis_base = (__global uint32_t*)(trellis_table + sz_0 * (4*trellis + kind));

    if (kind == 0) {
        uint32_t value = trellis_base[index];
        uint32_t diffed  = (int32_t)z2_diff_table[BYTE0(value)] << 0;
                 diffed ^= (int32_t)z2_diff_table[BYTE1(value)] << 8;
                 diffed ^= (int32_t)z2_diff_table[BYTE2(value)] << 16;
                 diffed ^= (int32_t)z2_diff_table[BYTE3(value)] << 24;
                 diffed ^= popcount_base[index] ? 0xffffffff : 0;
        trellis_base[index] = diffed;
    } else if (kind == 2) {
        if (index < (sz_0+1) / 2) {
            uint32_t value = trellis_base[index];
            uint32_t diffed  = (int32_t)z0_diff_table[BYTE0(value)] << 0;
                     diffed ^= (int32_t)z0_diff_table[BYTE1(value)] << 8;
                     diffed ^= (int32_t)z0_diff_table[BYTE2(value)] << 16;
                     diffed ^= (int32_t)z0_diff_table[BYTE3(value)] << 24;
                     diffed ^= popcount_base[index] ? 0xffffffff : 0;
            trellis_base[index] = diffed;
        }
    } else if (kind == 3) {
        if (index < (sz_0+1) / 2) {
            uint32_t value = trellis_base[index];
            uint32_t diffed  = (int32_t)z0_diff_table[BYTE0(value)] << 0;
                     diffed ^= (int32_t)z0_diff_table[BYTE1(value)] << 8;
                     diffed ^= (int32_t)z0_diff_table[BYTE2(value)] << 16;
                     diffed ^= (int32_t)z0_diff_table[BYTE3(value)] << 24;
                     diffed ^= popcount_base[index] ? 0xffffffff : 0;
            trellis_base[index] = diffed;
        }
    }
}

__kernel void trellis_encode_signal(__global float* output, __global uint8_t* trellis_table, __global uint8_t* popcount_table, __global uint32_t* output_commutator) {

    const unsigned trellis = get_global_id(1);
    const unsigned index = get_global_id(0);
          unsigned symbol_index = output_commutator[trellis * get_global_size(0) + index];

    const unsigned z0_last_bit_index = )#" + string_from<unsigned, bits_per_z0_table-1>::value + R"#(;
    const unsigned diff_encoder_table_size = )#" + string_from<unsigned, diff_encoder_table_size>::value + R"#(;
    const unsigned trellis_popcount_size = )#" + string_from<unsigned, trellis_popcount_size+1>::value + R"#(;

    const unsigned z0_index = index >> 1;
    const bool z0a = (index & 1) == 1;

    __global uint8_t* z2_base = (__global uint8_t*)(trellis_table + diff_encoder_table_size * (4*trellis + 0));
    __global uint8_t* z1_base = (__global uint8_t*)(trellis_table + diff_encoder_table_size * (4*trellis + 1));
    __global uint8_t* z0a_base = (__global uint8_t*)(trellis_table + diff_encoder_table_size * (4*trellis + 2));
    __global uint8_t* z0b_base = (__global uint8_t*)(trellis_table + diff_encoder_table_size * (4*trellis + 3));
    __global uint8_t* z2_popcount_base = (__global uint8_t*)(popcount_table + trellis_popcount_size * (4*trellis + 0)); 
    __global uint8_t* z0a_popcount_base = (__global uint8_t*)(popcount_table + trellis_popcount_size * (4*trellis + 2)); 
    __global uint8_t* z0b_popcount_base = (__global uint8_t*)(popcount_table + trellis_popcount_size * (4*trellis + 3)); 

    const unsigned z2_byte_index = index / 8;
    const unsigned z2_bit_index = index % 8;

    int8_t value = 0;
    if (index == 0) {
        // z0b is delayed, so its bit is z0b_popcount_base[0]
        value  = ((z2_base[z2_byte_index] >> z2_bit_index) & 1) << 2;
        value |= ((z1_base[z2_byte_index] >> z2_bit_index) & 1) << 1;
        value |= z0b_popcount_base[0] & 1;

        // z0b_popcount_base[0] needs to become the last bit in z0b_base
        z2_popcount_base[0] = z2_popcount_base[trellis_popcount_size-1];
        z0a_popcount_base[0] = z0a_popcount_base[trellis_popcount_size-1];
        z0b_popcount_base[0] = (z0b_base[z0_last_bit_index/8] >> (z0_last_bit_index % 8)) & 1;
    } else if (z0a) {
        // z0a
        const unsigned z0_byte_index = z0_index / 8;
        const unsigned z0_bit_index = z0_index % 8;

        value  = ((z2_base[z2_byte_index] >> z2_bit_index) & 1) << 2;
        value |= ((z1_base[z2_byte_index] >> z2_bit_index) & 1) << 1;
        value |= (z0a_base[z0_byte_index] >> z0_bit_index) & 1;
    } else {
        // z0b with delay
        const unsigned z0_byte_index = (z0_index - 1) / 8;
        const unsigned z0_bit_index = (z0_index - 1) % 8;

        value  = ((z2_base[z2_byte_index] >> z2_bit_index) & 1) << 2;
        value |= ((z1_base[z2_byte_index] >> z2_bit_index) & 1) << 1;
        value |= (z0b_base[z0_byte_index] >> z0_bit_index) & 1;
    }

    output[2*symbol_index] = (float)(value * 2 - 7) + 1.25f;
    output[2*symbol_index+1] = 0.f;
}

)#");

        _trellis = compile_kernel(_program, "trellis");
        _trellis_sort_bits = compile_kernel(_program, "trellis_sort_bits");
        _trellis_popcount = compile_kernel(_program, "trellis_popcount");
        _trellis_popcount_sum = compile_kernel(_program, "trellis_popcount_sum");
        _trellis_encode_signal = compile_kernel(_program, "trellis_encode_signal");
   }
};