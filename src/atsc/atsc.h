#pragma once
#include <memory>

class atsc_encoder {
public:    
    virtual ~atsc_encoder() = default;

    void process(uint8_t* pkt, unsigned count, std::function<void(void*,unsigned)> fn);

    static std::unique_ptr<atsc_encoder> create();
protected:
    atsc_encoder() = default;
};

