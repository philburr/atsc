#pragma once
#include "opencl.h"
#include "common/atsc_parameters.h"
#include "common/oscillator.h"

struct atsc_tune_cl : virtual opencl_base {

tprotected:
    cl_event tune(cl_array<atsc_symbol_type, ATSC_SYMBOLS_PER_FIELD> signal, cl_event event = nullptr) {
#if 1
        size_t count = ATSC_SYMBOLS_PER_FIELD;
        gpuErrchk(clSetKernelArg(_tune_signal, 0, sizeof(cl_mem), &signal.data()));
        gpuErrchk(clSetKernelArg(_tune_signal, 1, sizeof(cl_mem), &_oscillator_table));
        gpuErrchk(clSetKernelArg(_tune_signal, 2, sizeof(_oscillator_increment), &_oscillator_increment));
        return start_kernel(_tune_signal, 1, &count, event);
#else
        return tune_slow(signal, event);
#endif
    }

    cl_event tune_slow(cl_mem signal, cl_event event = nullptr) {
        size_t count = ATSC_SYMBOLS_PER_FIELD;
        gpuErrchk(clSetKernelArg(_tune_signal_slow, 0, sizeof(cl_mem), &signal));
        gpuErrchk(clSetKernelArg(_tune_signal_slow, 1, sizeof(double), &phase_));
        gpuErrchk(clSetKernelArg(_tune_signal_slow, 2, sizeof(double), &phase_increment_));
        auto e = start_kernel(_tune_signal_slow, 1, &count, event);
        phase_ += phase_increment_ * count;
        return e;
    }

protected:
    atsc_tune_cl() : opencl_base() {

        _oscillator_table = cl_alloc_ro(_oscillator.table.size() * sizeof(std::complex<float>));
        to_device(_oscillator_table, _oscillator.table.data(), _oscillator.table.size() * sizeof(std::complex<float>));
        _oscillator_increment = _oscillator.scale;

        phase_ = 0.f;
        phase_increment_ = 2 * M_PI * oscillator_table<void>::frequency / oscillator_table<void>::sampling_frequency;

        compile();
    }

    ~atsc_tune_cl() {
        clReleaseKernel(_tune_signal);
        clReleaseProgram(_program);
    }

private:
    cl_program _program;
    cl_kernel _tune_signal;
    cl_kernel _tune_signal_slow;
    cl_mem _oscillator_table;
    std::complex<float> _oscillator_increment;

    double phase_;
    double phase_increment_;

    oscillator_table<void> _oscillator;

    void compile() {
        _program = compile_program(kernel_prefix + R"#(

static inline float2 complex_mul(float2 a, float2 b)
{
    float2 r;
    r.x = a.x * b.x - a.y * b.y;
    r.y = a.x * b.y + a.y * b.x;
    return r;
}

static inline float2 oscillator(__global float2* otable, float2 adjust, const int i) {
    float2 oscillator_value = otable[i];
    otable[i] = complex_mul(oscillator_value, adjust);
    return oscillator_value;
}

__kernel void tune_signal(__global float2* signal, __global float2* otable, float2 adjust) {
    const int i = get_global_id(0);

    // Fetch the value from the oscillator
    float2 osc_value = oscillator(otable, adjust, i);
    // Mix the signals
    signal[i] = complex_mul(signal[i], osc_value);
}

__kernel void tune_signal_slow(__global float2* out, double phase, double phase_increment) {
    const int i = get_global_id(0);

    phase += phase_increment * i;

    float2 osc_value;
    //float osc_value_x;
    osc_value.y = 0.9f * sin((float)phase);
    osc_value.x = 0.9f * cos((float)phase);

    out[i] = complex_mul(out[i], osc_value);
}

)#");
        _tune_signal = compile_kernel(_program, "tune_signal");
        _tune_signal_slow = compile_kernel(_program, "tune_signal_slow");
    }
};