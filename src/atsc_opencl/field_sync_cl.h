#pragma once
#include "opencl.h"
#include "common/atsc_parameters.h"
#include "common/field_sync.h"

struct atsc_field_sync_cl : virtual opencl_base {

tprotected:
    cl_event field_sync(cl_mem signal, cl_event event = nullptr) {
        size_t count = atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT + atsc_parameters::ATSC_SYMBOLS_PER_BYTE * atsc_parameters::ATSC_DATA_SEGMENTS + atsc_parameters::ATSC_RESERVED_SYMBOLS;
        gpuErrchk(clSetKernelArg(_field_sync, 0, sizeof(cl_mem), &signal));
        gpuErrchk(clSetKernelArg(_field_sync, 1, sizeof(cl_mem), &_saved_symbols));
        gpuErrchk(clSetKernelArg(_field_sync, 2, sizeof(cl_mem), &_field_sync_curr));
        gpuErrchk(clSetKernelArg(_field_sync, 3, sizeof(cl_mem), &_last_saved_symbols));
        cl_event ev = start_kernel(_field_sync, 1, &count, event);
        if (_field_sync_curr == _field_sync_even) {
            _field_sync_curr = _field_sync_odd;
        } else {
            _field_sync_curr = _field_sync_even;
        }
        std::swap(_saved_symbols, _last_saved_symbols);
        return ev;
    }

protected:
    atsc_field_sync_cl() : opencl_base() {

        _field_sync_even = cl_alloc_ro(_fs_table.field_sync_even.size());
        _field_sync_odd = cl_alloc_ro(_fs_table.field_sync_odd.size());
        to_device(_field_sync_even, _fs_table.field_sync_even.data(), _fs_table.field_sync_even.size());
        to_device(_field_sync_odd, _fs_table.field_sync_odd.data(), _fs_table.field_sync_odd.size());
        _field_sync_curr = _field_sync_even;

        _saved_symbols = cl_alloc(atsc_parameters::ATSC_RESERVED_SYMBOLS * sizeof(atsc_parameters::atsc_field_signal));
        _last_saved_symbols = cl_alloc(atsc_parameters::ATSC_RESERVED_SYMBOLS * sizeof(atsc_parameters::atsc_field_signal));
        cl_memset(_last_saved_symbols, atsc_parameters::ATSC_RESERVED_SYMBOLS * sizeof(atsc_parameters::atsc_field_signal));

        compile();
    }

    ~atsc_field_sync_cl() {
        clReleaseKernel(_field_sync);
        clReleaseProgram(_program);
        cl_free(_field_sync_even);
        cl_free(_field_sync_odd);
        cl_free(_saved_symbols);
        cl_free(_last_saved_symbols);
    }

private:
    cl_program _program;
    cl_kernel _field_sync;
    cl_mem _field_sync_even;
    cl_mem _field_sync_odd;
    cl_mem _field_sync_curr;
    cl_mem _saved_symbols;
    cl_mem _last_saved_symbols;

    field_sync_table<atsc_parameters> _fs_table;

    void compile() {
        _program = compile_program(kernel_prefix + R"#(
__kernel void field_sync(__global float2* signal, __global float2* saved_symbols, __global uint8_t* sync_segment, __global float2* last_saved_symbols) {
    const int i = get_global_id(0);

    const unsigned symbols_per_field = )#" + string_from<unsigned, atsc_parameters::ATSC_SYMBOLS_PER_FIELD>::value + R"#(;
    const unsigned symbols_per_segment = )#" + string_from<unsigned, atsc_parameters::ATSC_SYMBOLS_PER_SEGMENT>::value + R"#(;
    const unsigned reserved_symbols = )#" + string_from<unsigned, atsc_parameters::ATSC_RESERVED_SYMBOLS>::value + R"#(;
    const unsigned segments = )#" + string_from<unsigned, atsc_parameters::ATSC_DATA_SEGMENTS>::value + R"#(;
    const unsigned reserved_index = symbols_per_segment + 4 * segments;

    float2 sig;
    if (i < symbols_per_segment) {
        if (i < 4) {
            if (i == 0 || i == 3) {
                sig.x = (float)(6 * 2 - 7) + 1.25f;
                sig.y = 0;
            } else {
                sig.x = (float)(1 * 2 - 7) + 1.25f;
                sig.y = 0;
            }
        } else if (i < symbols_per_segment - reserved_symbols) {
            sig.x = (float)(sync_segment[i - 4] * 2 - 7) + 1.25f;
            sig.y = 0;
        } else {
            unsigned offset = i - (symbols_per_segment - reserved_symbols);
            sig = last_saved_symbols[offset];
        }
        signal[i] = sig;
    } else if (i < reserved_index){
        unsigned offset = i - symbols_per_segment;
        unsigned segment = 1 + offset / 4;
        offset %= 4;

        if (offset == 0 || offset == 3) {
            sig.x = (float)(6 * 2 - 7) + 1.25f;
            sig.y = 0;
        } else {
            sig.x = (float)(1 * 2 - 7) + 1.25f;
            sig.y = 0;
        }
        signal[segment * symbols_per_segment + offset] = sig;
    } else {
        unsigned offset = i - reserved_index;
        sig = signal[symbols_per_field - reserved_symbols + offset];
        saved_symbols[offset] = sig;        
    }
}

)#");
        _field_sync = compile_kernel(_program, "field_sync");
    }
};