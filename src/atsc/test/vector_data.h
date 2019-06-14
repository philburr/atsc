#include <cstdio>
#include <memory>
#include <array>
#include <string>
#include <filesystem>
#include <random>

extern std::filesystem::path test_directory;

template<typename T>
auto random_vector_data()
    -> std::unique_ptr<T>
{
    using data_type = std::tuple_element_t<0, T>;
    auto data = std::make_unique<T>();

    auto eng = std::default_random_engine();
    if constexpr(std::is_integral_v<data_type>) {
        auto rnd = std::uniform_int_distribution<data_type>();
        for (size_t i = 0; i < data->size(); i++) {
            (*data)[i] = rnd(eng);
        }
        return data;
    } else if constexpr(std::is_floating_point_v<data_type>) {
        auto rnd = std::uniform_real_distribution<data_type>(-1, 1);
        for (size_t i = 0; i < data->size(); i++) {
            (*data)[i] = rnd(eng);
        }
        return data;
    } else {
        auto rnd = std::uniform_real_distribution<float>(-1, 1);
        for (size_t i = 0; i < data->size(); i++) {
            (*data)[i] = std::complex<float>(rnd(eng), rnd(eng));
        }
        return data;
    }
}

template<typename T>
auto load_vector_data(const std::string& filename) 
    -> std::unique_ptr<T>
{
    auto full_path = test_directory / "data" / filename;

    FILE* fp = fopen(full_path.c_str(), "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to load test data: %s\n", full_path.c_str());
        return nullptr;
    }

    auto r = std::make_unique<T>();
    if (r == nullptr) {
        fclose(fp);
        return nullptr;
    }

    auto sz = fread(r->data(), sizeof(std::tuple_element_t<0, T>) * std::tuple_size_v<T>, 1, fp);
    if (sz != 1) {
        fclose(fp);
        return nullptr;
    }

    fclose(fp);
    return r;
}

template<typename T, size_t N>
void save_vector_data(const std::string& filename, std::array<T, N>* data)
{
    auto full_path = test_directory / "data" / filename;

    FILE* fp = fopen(full_path.c_str(), "wb");
    if (fp == NULL) {
        return;
    }

    fwrite(data->data(), sizeof(T) * N, 1, fp);
    fclose(fp);
}