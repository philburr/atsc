#pragma once
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.hpp>
#include <vector>
#include <array>

#ifdef BUILD_UNITTEST
#define tprivate public
#define tprotected public
#else
#define tprivate private
#define tprotected protected
#endif

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
static inline void gpuAssert(cl_int code, const char *file, int line, bool abort=true)
{
    if (code != CL_SUCCESS)
    {
        fprintf(stderr,"GPUassert: %d %s:%d\n", code, file, line);
        if (abort) exit(code);
    }
}

template<typename T, size_t N>
struct cl_array {
    using type = T;

    explicit cl_array(cl_mem mem) : mem_(mem) {}

    constexpr size_t size() {
        return N;
    }
    cl_mem& data() {
        return mem_;
    }

private:
    cl_mem mem_;
};

struct opencl_base {
protected:

    opencl_base() 
        : local_group_size{256, 1, 1}
    {
        clGetPlatformIDs(1, &platform, NULL);

        cl_uint deviceIdCount;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceIdCount);
        deviceids.resize(deviceIdCount);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, deviceIdCount, &deviceids[0], NULL);

        cl_int err;
        cl_context_properties props[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
        ctx = clCreateContext( props, deviceids.size(), &deviceids[0], NULL, NULL, &err ); gpuErrchk(err);

        deviceid = deviceids[0];
#ifdef BUILD_UNITTEST
        q = clCreateCommandQueue( ctx, deviceid, CL_QUEUE_PROFILING_ENABLE, &err ); gpuErrchk(err);
#else
        q = clCreateCommandQueue( ctx, deviceid, 0, &err ); gpuErrchk(err);
#endif        

        kernel_prefix = std::string(R"#(
#pragma OPENCL EXTENSION cl_intel_printf : enable
#include <stdint.h>
)#");

    }

    ~opencl_base() {
        clReleaseCommandQueue(q);
        clReleaseContext(ctx);
    }

    cl_program compile_program(const std::string& content)
    {
        const char* sources[] = { content.data() };
        size_t lengths[] = { content.size() };

        cl_int err;
        cl_program program = clCreateProgramWithSource(ctx, 1, sources, lengths, &err);
        gpuErrchk(err);

        err = clBuildProgram(program, 1, &deviceid, nullptr, nullptr, nullptr);
        if (err == CL_BUILD_PROGRAM_FAILURE) {
            size_t len;
            clGetProgramBuildInfo(program, deviceid, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
            char* log = new char[len];
            clGetProgramBuildInfo(program, deviceid, CL_PROGRAM_BUILD_LOG, len, log, NULL);
            printf("%s\n", log);
            delete log;
        }
        gpuErrchk(err);
        return program;
    }

    cl_kernel compile_kernel(cl_program program, const std::string& name)
    {
        cl_int err;
        auto kernel = clCreateKernel(program, name.c_str(), &err);
        gpuErrchk(err);
        return kernel;
    }

    cl_event start_kernel(cl_kernel        kernel,
                          cl_uint          work_dim,
                          const size_t *   global_work_size,
                          cl_event         wait_event) {
        cl_event event;
        if (wait_event) {
            gpuErrchk(clEnqueueNDRangeKernel(q, kernel, work_dim, NULL, global_work_size, &local_group_size[0], 1, &wait_event, &event));
            clReleaseEvent(wait_event);
        } else {
            gpuErrchk(clEnqueueNDRangeKernel(q, kernel, work_dim, NULL, global_work_size, &local_group_size[0], 0, NULL, &event));
        }
#ifdef BUILD_UNITTEST
        clWaitForEvents(1, &event);
        clFinish(q);

        cl_ulong time_start;
        cl_ulong time_end;

        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

        double nanoSeconds = time_end-time_start;
        printf("OpenCl Execution time is: %0.6f microeconds \n",nanoSeconds / 1000.0);
#endif
        return event;
    }

tprotected:
    void to_device(cl_mem dest, const void* src, size_t sz) {
        cl_int err = clEnqueueWriteBuffer(q, dest, CL_TRUE, 0, sz, src, 0, NULL, NULL);
        gpuErrchk(err);
    }

    void to_host(void* dest, cl_mem src, size_t sz, cl_event wait_event = nullptr) {
        if (wait_event) {
            clEnqueueReadBuffer(q, src, CL_TRUE, 0, sz, dest, 1, &wait_event, NULL);
            clReleaseEvent(wait_event);
        } else {
            clEnqueueReadBuffer(q, src, CL_TRUE, 0, sz, dest, 0, NULL, NULL);
        }
    }

    template<typename T, size_t SZ>
    cl_array<T, SZ> cl_alloc_arr() {
        return cl_array<T, SZ>(cl_alloc(sizeof(T) * SZ));
    }

    template<typename T>
    auto cl_alloc_arr() {
        using type = std::tuple_element_t<0, T>;
        return cl_array<type, std::tuple_size<T>::value>(cl_alloc(sizeof(type) * std::tuple_size<T>::value));
    }

    cl_mem cl_alloc(size_t sz) {
        int err;
        cl_mem mem = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sz, NULL, &err); gpuErrchk(err);
        return mem;
    }

    cl_mem cl_alloc_ro(size_t sz) {
        int err;
        cl_mem mem = clCreateBuffer(ctx, CL_MEM_READ_ONLY, sz, NULL, &err); gpuErrchk(err);
        return mem;
    }

    void cl_free(cl_mem mem) {
        clReleaseMemObject(mem);
    }

    template<typename T, size_t SZ>
    void cl_free(cl_array<T, SZ> mem) {
        return cl_free(mem.data());
    }

    void cl_memset(cl_mem mem, size_t sz) {
        char fill = 0;
        clEnqueueFillBuffer(q, mem, &fill, 1, 0, sz, 0, NULL, NULL);
    }


protected:
    cl_platform_id platform;
    std::vector<cl_device_id> deviceids;
    cl_context ctx;
    cl_command_queue q;
    cl_device_id deviceid;

    size_t local_group_size[3];

    std::string kernel_prefix;
};