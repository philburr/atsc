#pragma once
#include "opencl.h"
#include "common/atsc_parameters.h"
#include "common/interleaver.h"

struct atsc_interleaver_cl : virtual opencl_base {

tprotected:
    cl_event interleave(cl_array<uint8_t, ATSC_DATA_PER_FIELD> frame1, cl_array<uint8_t, ATSC_DATA_PER_FIELD> frame2, cl_array<uint8_t, ATSC_DATA_PER_FIELD> input, cl_event event = nullptr)
    {
        size_t count = ATSC_DATA_PER_FIELD;
        gpuErrchk(clSetKernelArg(_interleave, 0, sizeof(cl_mem), &frame1.data()));
        gpuErrchk(clSetKernelArg(_interleave, 1, sizeof(cl_mem), &frame2.data()));
        gpuErrchk(clSetKernelArg(_interleave, 2, sizeof(cl_mem), &input.data()));
        gpuErrchk(clSetKernelArg(_interleave, 3, sizeof(cl_mem), &_interleave_table));
        return start_kernel(_interleave, 1, &count, event);
    }

protected:
    atsc_interleaver_cl() : opencl_base() {

        size_t sz = sizeof(uint16_t) * 2 * _interleaver_table.table.size(); (void)sz;
        _interleave_table = cl_alloc_ro(sz);
        to_device(_interleave_table, _interleaver_table.table.data(), sz);

        compile();
    }

    ~atsc_interleaver_cl() {
        clReleaseKernel(_interleave);
        clReleaseProgram(_program);
        clReleaseMemObject(_interleave_table);
    }


tprivate:
    interleaver<ATSC_TRELLIS_INPUT_OPENCL> _interleaver_table;

private:
    cl_program _program;
    cl_kernel _interleave;
    cl_mem _interleave_table;

    void compile() {

        _program = compile_program(kernel_prefix + R"#(
__kernel void interleave(__global uint8_t* frame1, __global uint8_t* frame2, __global uint8_t* input, __global uint16_t* matrix)
{
    const int i = get_global_id(0);
    
    if (i == 0) return;
    uint16_t field = matrix[i * 2 + 0];
    uint16_t index = matrix[i * 2 + 1];
    if (field == 0) {
        frame1[index] = input[i];
    } else {
        frame2[index] = input[i];
    }
}
)#");
        _interleave = compile_kernel(_program, "interleave");
    }

};