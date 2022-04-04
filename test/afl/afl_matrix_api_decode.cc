#include <cstdio>
#include <cassert>
#include <iostream>

#include "api_compression.h"

struct close_file
{
    auto operator()(FILE* f) const -> void {
        if (!f) return;
        if (std::fclose(f)) {
            std::cerr << "close_file: failed\n";
        }
    }
};
using file_ptr = std::unique_ptr<FILE, close_file>;


// generates a seed file for the fuzzer
auto generate_input_file(const std::string& fname) -> void {
    constexpr const int64_t height{ 2 };
    constexpr const int64_t length{ 3 };
    const std::vector<int32_t> matrix{ 1, 2, 3, 4, 5, 6 };
    ctk::api::v1::CompressInt32 encoder;
    encoder.Sensors(height);
    const std::vector<uint8_t> bytes{ encoder.ColumnMajor(matrix, length) };

    file_ptr f{ fopen(fname.c_str(), "wb") };
    if (!f) {
        throw std::runtime_error("cannot write to file");
    }

    if (std::fwrite(&height, sizeof(height), 1, f.get()) != 1) {
        throw std::runtime_error("cannot write out height");
    }

    if (std::fwrite(&length, sizeof(length), 1, f.get()) != 1) {
        throw std::runtime_error("cannot write out max_length");
    }

    if (std::fwrite(&length, sizeof(length), 1, f.get()) != 1) {
        throw std::runtime_error("cannot write out length");
    }

    if (std::fwrite(bytes.data(), sizeof(uint8_t), bytes.size(), f.get()) != bytes.size()) {
        throw std::runtime_error("cannot write out bytes");
    }

    std::cerr << "afl-fuzz input file written\n";
}


auto main(int argc, char* argv[]) -> int {
	if (argc < 2) {
        std::cerr << "missing argument: file name\n";
        return 1;
    }
    // generates a seed file for the fuzzer
    //generate_input_file(argv[1]);
    //return 0;

    std::vector<uint8_t> bytes;
    int64_t height{ 0 };
    int64_t max_length{ 0 };
    int64_t length{ 0 };

    try {
        { // scope for the file i/o
            file_ptr f{ fopen(argv[1], "rb") };
            if (!f) {
                return 1;
            }

            if (std::fread(&height, sizeof(height), 1, f.get()) != 1) {
                return 1;
            }

            if (std::fread(&max_length, sizeof(max_length), 1, f.get()) != 1) {
                return 1;
            }

            if (std::fread(&length, sizeof(length), 1, f.get()) != 1) {
                return 1;
            }

            uint8_t x;
            while (std::fread(&x, sizeof(x), 1, f.get()) == 1) {
                bytes.push_back(x);
            }
        }

        ctk::api::v1::DecompressInt32 decoder;
        if (!decoder.Sensors(height)) {
            return 1;
        }

        const std::vector<int32_t> v1{ decoder.ColumnMajor(bytes, length) };
        const std::vector<int32_t> v2{ decoder.RowMajor(bytes, length) };
        assert(v1.size() == v2.size());
        return !v1.empty() && !v2.empty();
    }
    catch (const std::bad_alloc&) {
        return 1;
    }
    catch (const std::length_error&) {
        return 1;
    }

    return 1;
}


