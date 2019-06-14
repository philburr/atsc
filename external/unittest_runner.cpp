#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"
#include <filesystem>
#include <string>
#include <cstdio>

std::filesystem::path test_directory;

int main( int argc, char* argv[] ) {
    auto path = std::filesystem::path(std::string(argv[0]));
    if (path.is_absolute()) {
        test_directory = path.parent_path();
    } else {
        auto abs_path = std::filesystem::current_path();
        abs_path /= path;
        test_directory = abs_path.parent_path();
    }
  return Catch::Session().run( argc, argv );
}