#pragma once
#include <memory>
#include <functional>

namespace atsc {

class atsc_encoder {
public:    
    virtual ~atsc_encoder() = default;

    virtual void process(uint8_t* pkt, unsigned count, std::function<void(void*,unsigned)> fn) = 0;

    static std::unique_ptr<atsc_encoder> create();
protected:
    atsc_encoder() = default;
};

}