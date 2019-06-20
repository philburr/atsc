#pragma once
#include "opencl.h"
#include "common/atsc_parameters.h"
#include "common/filter.h"

struct atsc_filter_cl : virtual opencl_base {

tprotected:
    cl_event filter(cl_array<atsc_symbol_type, ATSC_SYMBOLS_PER_FIELD> output, cl_array<atsc_symbol_type, ATSC_SYMBOLS_PER_FIELD+ATSC_SYMBOLS_PER_SEGMENT> input, cl_event event = nullptr) {
        size_t count = ATSC_SYMBOLS_PER_FIELD;
        gpuErrchk(clSetKernelArg(_fir_filter, 0, sizeof(cl_mem), &output.data()));
        gpuErrchk(clSetKernelArg(_fir_filter, 1, sizeof(cl_mem), &input.data()));
        gpuErrchk(clSetKernelArg(_fir_filter, 2, sizeof(cl_mem), &_taps));
        gpuErrchk(clSetKernelArg(_fir_filter, 3, sizeof(_ntaps), &_ntaps));
        return start_kernel(_fir_filter, 1, &count, event);
    }

protected:
    atsc_filter_cl() : opencl_base() {

        _ntaps = _filter.filter.size();
        _taps = cl_alloc_ro(_filter.filter.size() * sizeof(float));
        to_device(_taps, _filter.filter.data(), _filter.filter.size() * sizeof(float));

        compile();
    }

    ~atsc_filter_cl() {
        clReleaseKernel(_fir_filter);
        clReleaseProgram(_program);
        clReleaseMemObject(_taps);
    }

private:
    cl_program _program;
    cl_kernel _fir_filter;
    cl_mem _taps;
    uint32_t _ntaps;

    rrc_filter _filter;

    void compile() {
        _program = compile_program(kernel_prefix + R"#(

static inline float2 complex_mul(float2 a, float b)
{
    float2 r;
    r.x = a.x * b;
    r.y = a.y * b;
    return r;
}

static inline float2 complex_sum(float2 a, float2 b)
{
    float2 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}

__kernel void fir_filter(__global float2* output, __global float2* input, __global float* filter, uint32_t ntaps)
{
    const int gid = get_global_id(0);
    const int tid = get_local_id(0);
    const int lsize = get_local_size(0);

    // Preload the filter into shared memory
    // Max filter size 128!!!
    __local float local_filter[128];

    // Load and reverse the filter
    for (int index = tid; index < ntaps; index += lsize) {
        local_filter[(ntaps-1) - index] = filter[index];
    }

    // Wait for all to finish!
    barrier(CLK_LOCAL_MEM_FENCE);

    // now apply the filter
    float2 res = {0, 0};
    for (int index = 0; index < ntaps; index++) {
        float2 sample = input[gid + index];
        float tap = local_filter[index];
        res = complex_sum(res, complex_mul(sample, tap));
    }
    output[gid] = res;
}
)#");
        _fir_filter = compile_kernel(_program, "fir_filter");
    }

};