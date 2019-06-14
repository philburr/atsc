#include "opencl.h"
#include "randomize_cl.h"
#include "interleaver_cl.h"
#include "reed_solomon_cl.h"
#include "trellis_cl.h"
#include "field_sync_cl.h"
#include "tune_cl.h"
#include "filter_cl.h"

struct atsc_opencl : virtual public opencl_base
                   , public atsc_randomize_cl
                   , public atsc_interleaver_cl
                   , public atsc_reed_solomon_cl
                   , public atsc_trellis_cl
                   , public atsc_field_sync_cl
                   , public atsc_tune_cl
                   , public atsc_filter_cl {

    atsc_opencl() 
        : opencl_base() 
        , atsc_randomize_cl()
        , atsc_interleaver_cl()
        , atsc_reed_solomon_cl()
        , atsc_trellis_cl()
        , atsc_field_sync_cl()
        , atsc_tune_cl()
        , atsc_filter_cl()
    {
    }

    ~atsc_opencl() {
    }
};
