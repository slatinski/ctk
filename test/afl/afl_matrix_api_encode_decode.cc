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

    file_ptr f{ fopen(fname.c_str(), "wb") };
    if (!f) {
        throw std::runtime_error("cannot write to file");
    }

    if (std::fwrite(&height, sizeof(height), 1, f.get()) != 1) {
        throw std::runtime_error("cannot write out height");
    }

    if (std::fwrite(&length, sizeof(length), 1, f.get()) != 1) {
        throw std::runtime_error("cannot write out length");
    }

    if (std::fwrite(matrix.data(), sizeof(int32_t), matrix.size(), f.get()) != matrix.size()) {
        throw std::runtime_error("cannot write out the matrix");
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

    std::vector<int32_t> input;
    int64_t height{ 0 };
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

            if (std::fread(&length, sizeof(length), 1, f.get()) != 1) {
                return 1;
            }

            int32_t x;
            while (std::fread(&x, sizeof(x), 1, f.get()) == 1) {
                input.push_back(x);
            }
        }

        ctk::api::v1::CompressInt32 encoder;
        if (!encoder.sensors(height, std::nothrow)) {
            return 1;
        }

        const std::vector<uint8_t> bytes_column_major{ encoder.column_major(input, length, std::nothrow) };
        const std::vector<uint8_t> bytes_row_major{ encoder.row_major(input, length, std::nothrow) };
        assert((bytes_column_major.empty() && bytes_row_major.empty()) || (!bytes_column_major.empty() && !bytes_row_major.empty()));
        if (bytes_column_major.empty()) {
            return 0;
        }
        assert(!input.empty());


        ctk::api::v1::DecompressInt32 decoder;
        assert(decoder.sensors(height));

        const std::vector<int32_t> output_column_major{ decoder.column_major(bytes_column_major, length) };
        const std::vector<int32_t> output_row_major{ decoder.row_major(bytes_row_major, length) };
        assert(output_column_major.size() == output_row_major.size());
        assert(output_column_major.size() == input.size());

        const size_t size{ input.size() };
        for (size_t i = 0; i < size; ++i) {
            assert(output_column_major[i] == input[i]);
            assert(output_row_major[i] == input[i]);
        }

        return 0;
    }
    catch (const std::bad_alloc&) {
        return 1;
    }
    catch (const std::length_error&) {
        return 1;
    }

    return 1;
}


