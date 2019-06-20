#pragma once
#include "opencl.h"
#include "common/atsc_parameters.h"
#include "common/reed_solomon.h"

struct atsc_reed_solomon_cl : virtual opencl_base {

tprotected:
    cl_event rs_encode(cl_array<uint8_t, ATSC_DATA_PER_FIELD> in, cl_event event = nullptr)
    {
        size_t count = ATSC_DATA_SEGMENTS;

        gpuErrchk(clSetKernelArg(_rs_encode, 0, sizeof(cl_mem), &in.data()));
        gpuErrchk(clSetKernelArg(_rs_encode, 1, sizeof(cl_mem), &_gf_exp));
        gpuErrchk(clSetKernelArg(_rs_encode, 2, sizeof(cl_mem), &_gf_log));
        gpuErrchk(clSetKernelArg(_rs_encode, 3, sizeof(cl_mem), &_modulo_table));
        gpuErrchk(clSetKernelArg(_rs_encode, 4, sizeof(cl_mem), &_gen_poly));
        return start_kernel(_rs_encode, 1, &count, event);
    }

protected:
    atsc_reed_solomon_cl() : opencl_base() {

        _gf_log = cl_alloc_ro(_galois_tables.log.size());
        to_device(_gf_log, _galois_tables.log.data(), _galois_tables.log.size());
        _gf_exp = cl_alloc_ro(_galois_tables.exp.size());
        to_device(_gf_exp, _galois_tables.exp.data(), _galois_tables.exp.size());
        _modulo_table = cl_alloc_ro(_mod_table.table.size());
        to_device(_modulo_table, _mod_table.table.data(), _mod_table.table.size());

        std::array<uint8_t, 21> genpoly = {1};
        for (size_t i = 0, root = 0; i < 20; i++, root++) {
            genpoly[i+1] = 1;

            for (size_t j = i; j > 0; j--) {
                if (genpoly[j] != 0) {
                    genpoly[j] = genpoly[j - 1] ^ _galois_tables.exp[_mod_table.table[_galois_tables.log[genpoly[j]] + root]];
                } else {
                    genpoly[j] = genpoly[j - 1];
                }
            }
            genpoly[0] = _galois_tables.exp[_mod_table.table[_galois_tables.log[genpoly[0]] + root]];
        }
        for (size_t i = 0; i <= 20; i++) {
            genpoly[i] = _galois_tables.log[genpoly[i]];
        }

        _gen_poly = cl_alloc_ro(genpoly.size());
        to_device(_gen_poly, genpoly.data(), genpoly.size());


        compile();
    }

    ~atsc_reed_solomon_cl() {
        clReleaseKernel(_rs_encode);
        clReleaseProgram(_program);
        clReleaseMemObject(_gf_log);
        clReleaseMemObject(_gf_exp);
        clReleaseMemObject(_modulo_table);
        clReleaseMemObject(_gen_poly);
    }


private:
    cl_program _program;
    cl_kernel _rs_encode;
    cl_mem _gf_log;
    cl_mem _gf_exp;
    cl_mem _modulo_table;
    cl_mem _gen_poly;

    modulo_table<255> _mod_table;
    galois_tables<8, 0x11d> _galois_tables;

    void compile() {
        _program = compile_program(kernel_prefix + R"#(
typedef uint8_t gf;
typedef uint8_t gf_log;

#define POLY 0x11d
#define NROOTS 20

gf_log left_shift(uint8_t val, uint8_t shift, __global uint8_t* mod_table) {
    return mod_table[(int)val + shift];
}

void encode_rs(__global uint8_t* bb, __global uint8_t* buffer, __global uint8_t* exp_table, __global uint8_t* log_table, __global uint8_t* mod_table, __global uint8_t* genpoly) {
    gf_log feedback;
    const gf_log infinity = 255;

    for (unsigned i = 0; i < 187; i++) {
        feedback = log_table[buffer[i] ^ bb[0]];
        if (feedback != infinity) {
            for (unsigned j = 1; j < NROOTS; j++) {
                bb[j] ^= exp_table[left_shift(genpoly[NROOTS - j], feedback, mod_table)];
            }
        }
        //memmove(&bb[0], &bb[1], sizeof(gf) * (NROOTS - 1));
        for (unsigned j = 0; j < NROOTS - 1; j++) {
            bb[j] = bb[j + 1];
        }

        if (feedback != infinity) {
            bb[NROOTS-1] = exp_table[left_shift(genpoly[0], feedback, mod_table)];
        } else {
            bb[NROOTS-1] = 0;
        }
    }
}


__kernel void rs_encode(__global uint8_t* in, __global uint8_t* exp_table, __global uint8_t* log_table, __global uint8_t* mod_table, __global uint8_t* genpoly)
{
    const int i = get_global_id(0);

    __global uint8_t* rs = in + i * 207;
    encode_rs(rs + 187, rs, exp_table, log_table, mod_table, genpoly);
}
)#");
         _rs_encode = compile_kernel(_program, "rs_encode");
   }
};