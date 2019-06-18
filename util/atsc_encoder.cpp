#include <cstdio>
#include <cstring>
#include <memory>
#include <functional>
#include <immintrin.h>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <cxxopts.hpp>

#include "atsc/atsc.h"
#include "common/atsc_parameters.h"
#include "defaults.h"

template<typename T>
using unique_freeable_ptr = std::unique_ptr<T,std::function<void(T*)>>;

#define XSTR(x) STR(x)
#define STR(x) #x

int main(int argc, char* argv[]) {

    cxxopts::Options options(argv[0], "ATSC SDR Modulator");
    options.add_options()
        ("d, driver", "SoapySDR Driver String", cxxopts::value<std::string>()->default_value(XSTR(DEFAULT_DRIVER_NAME)))
        ("f, frequency", "Transmit Frequency", cxxopts::value<float>()->default_value(XSTR(DEFAULT_FREQUENCY))) 
        ("g, gain", "Gain (dB)", cxxopts::value<float>()->default_value(XSTR(DEFAULT_GAIN)))
        ("positional", "filename", cxxopts::value<std::vector<std::string>>())
        ;
    options.parse_positional("positional");

    float frequency;
    float gain;
    std::string filename;
    std::string driver;

    try {
        auto result = options.parse(argc, argv);
        driver = result["driver"].as<std::string>();
        frequency = result["frequency"].as<float>();
        gain = result["gain"].as<float>();
        filename = result["positional"].as<std::vector<std::string>>()[0];
    }
    catch(...) {
        options.positional_help("filename");
        fprintf(stderr, "%s", options.help().c_str());
        exit(0);
    }

    auto input = unique_freeable_ptr<FILE>();
    if (filename == "-") {
        input = unique_freeable_ptr<FILE>(stdin, [](FILE*){});
    } else {
        input = unique_freeable_ptr<FILE>(fopen(filename.c_str(), "rb"), [](FILE* fp) { fclose(fp); });
    }
    if (!input) {
        fprintf(stderr, "Failed to read input file\n");
        return 0;
    }

    try {
        auto driver_str = std::string("driver=") + driver;
        auto device = unique_freeable_ptr<SoapySDR::Device>(SoapySDR::Device::make(driver_str), [](SoapySDR::Device* d) { SoapySDR::Device::unmake(d); });
        auto encoder = atsc_encoder::create();
        if (!encoder) {
            printf("Failed to create encoder\n");
            return 0;
        }

        device->setBandwidth(SOAPY_SDR_TX, 0, 6e6);
        device->setSampleRate(SOAPY_SDR_TX, 0, 4500000.0 / 286 * 684);
        device->setFrequencyCorrection(SOAPY_SDR_TX, 0, 0);
        device->setFrequency(SOAPY_SDR_TX, 0, frequency);
        device->setGain(SOAPY_SDR_TX, 0, gain);

        auto stream = device->setupStream(SOAPY_SDR_TX, SOAPY_SDR_CF32, {0});
        auto samples = device->getStreamMTU(stream);

        int16_t* result = (int16_t*)_mm_malloc(sizeof(int16_t) * ATSC_SYMBOLS_PER_FIELD, 32);


        auto in = std::make_unique<atsc_field_mpeg2>();
        uint8_t* in_data = in.get()->data();
        while (fread(in_data, ATSC_DATA_SEGMENTS * ATSC_MPEG2_BYTES, 1, input.get()) == 1) {
            encoder->process(in_data, ATSC_DATA_SEGMENTS, [device = device.get(), stream, samples, result](void* data, unsigned sz) {

                int flags = 0;
                (void)sz;

                std::complex<float> *xfer = (std::complex<float>*)data;
                auto remaining = size_t(ATSC_SYMBOLS_PER_FIELD);
                while (remaining > 0) {
                    auto transfer = std::min(remaining, samples);

                    void* buffers[] = { xfer };
                    device->writeStream(stream, buffers, transfer, flags);

                    remaining -= transfer;
                    xfer += transfer;
                }
            });
        }

        device->closeStream(stream);
        _mm_free(result);

    } catch (std::runtime_error& e) {
        printf("No hardware matching '%s' found\n", driver.c_str());
    }

    return 0;
}