#include <cstdio>
#include <cassert>
#include <iostream>

#include "api_compression.h"
#include "exception.h"

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
    catch (const std::bad_alloc&) {
        return 1;
    }
    catch (const std::length_error&) {
        return 1;
    }

    try {
        ctk::api::v1::CompressInt32 encoder;
        if (!encoder.Sensors(height)) {
            return 1;
        }

        const std::vector<uint8_t> bytes_column_major{ encoder.ColumnMajor(input, length) };
        const std::vector<uint8_t> bytes_row_major{ encoder.RowMajor(input, length) };
        assert((bytes_column_major.empty() && bytes_row_major.empty()) || (!bytes_column_major.empty() && !bytes_row_major.empty()));
        if (bytes_column_major.empty()) {
            return 0;
        }
        assert(!input.empty());


        ctk::api::v1::DecompressInt32 decoder;
        assert(decoder.Sensors(height));

        const std::vector<int32_t> output_column_major{ decoder.ColumnMajor(bytes_column_major, length) };
        const std::vector<int32_t> output_row_major{ decoder.RowMajor(bytes_row_major, length) };
        assert(output_column_major == input);
        assert(output_row_major == input);

        // deterministic
        const std::vector<uint8_t> reencoded_column_major{ encoder.ColumnMajor(output_column_major, length) };
        const std::vector<uint8_t> reencoded_row_major{ encoder.RowMajor(output_row_major, length) };
        assert(reencoded_column_major == bytes_column_major);
        assert(reencoded_row_major == bytes_row_major);
        return 0;
    }
    catch (const std::bad_alloc&) {
        return 1;
    }
    catch (const std::length_error&) {
        return 1;
    }
    catch (const ctk::api::v1::CtkLimit&) {
        return 1;
    }
    catch (const ctk::api::v1::CtkData&) {
        return 1;
    }

    return 1;
}


