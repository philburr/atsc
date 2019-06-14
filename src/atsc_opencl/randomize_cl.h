#pragma once
#include "opencl.h"
#include "common/atsc_parameters.h"
#include "common/randomize.h"

struct atsc_randomize_cl : virtual opencl_base {

tprotected:
    cl_event randomize(cl_mem out, cl_mem in, cl_event event = nullptr)
    {
        size_t count = atsc_parameters::ATSC_DATA_PER_FIELD;
        gpuErrchk(clSetKernelArg(_randomize, 0, sizeof(cl_mem), &out));
        gpuErrchk(clSetKernelArg(_randomize, 1, sizeof(cl_mem), &in));
        gpuErrchk(clSetKernelArg(_randomize, 2, sizeof(cl_mem), &_random));
        return start_kernel(_randomize, 1, &count, event);
    }

protected:
    atsc_randomize_cl() : opencl_base() {

        _random = cl_alloc_ro(_randomize_table.table.size());
        to_device(_random, _randomize_table.table.data(), _randomize_table.table.size());

        compile();
    }

    ~atsc_randomize_cl() {
        clReleaseKernel(_randomize);
        clReleaseProgram(_program);
        clReleaseMemObject(_random);
    }

private:
    cl_program _program;
    cl_kernel _randomize;
    cl_mem _random;

    randomize_table<atsc_parameters> _randomize_table;

    void compile() {

        _program = compile_program(kernel_prefix + R"#(

uint8_t xor(uint8_t a, uint8_t b)
{
    return a ^ b;
}

__kernel void randomize(__global uint8_t* out, __global uint8_t* in, __global uint8_t* rtable)
{
    const int i = get_global_id(0);
    const int packet = i / 207;
    const int index = i % 207;

    if (index < 187) {
        const int rindex = packet * 187 + index;
        const int iindex = packet * 188 + index + 1;
        out[i] = xor(in[iindex], rtable[rindex]);
    } else {
        // zero the RS bytes
        out[i] = 0;
    }
}
)#");
        _randomize = compile_kernel(_program, "randomize");
    }

};