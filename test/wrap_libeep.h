#pragma once

#include "ctk/api_data.h"
#include "ctk/container/file_reflib.h"
#include <memory>
#include <chrono>

namespace raw3 {
extern "C" {
#include "libcnt/raw3.c"
}
}

namespace ctk { namespace impl { namespace test {

struct libeep
{
    struct raw_free
    {
        void operator()(raw3::raw3_t* p) const {
            raw3::raw3_free(p);
        }
    };


	std::unique_ptr<raw3::raw3_t, raw_free> encoder;
	std::unique_ptr<raw3::raw3_t, raw_free> decoder;
	std::vector<short> chanv;
    int chanc;
    uint64_t epoch_length;


    libeep(measurement_count length, const std::vector<int16_t>& order)
    : chanc{ cast(vsize(order), int{}, ok{}) }
    , epoch_length{ cast(static_cast<sint>(length), uint64_t{}, ok{}) } {
        chanv.resize(order.size());
        std::copy(begin(order), end(order), begin(chanv));

	    encoder.reset(raw3::raw3_init(chanc, chanv.data(), epoch_length));
        if (!encoder) {
            throw std::runtime_error("cannot initialize encoder");
        }

	    decoder.reset(raw3::raw3_init(chanc, chanv.data(), epoch_length));
        if (!decoder) {
            throw std::runtime_error("cannot initialize decoder");
        }
    }


    auto decode(const std::vector<uint8_t>& compressed, int repetitions = 1) -> std::pair<std::vector<raw3::sraw_t>, std::chrono::microseconds> {
        /*
        std::vector<char> bytes is twice as big as neccessary because else asan thends to complain
        ==7985==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x622000068f05 at pc 0x00000055702f bp 0x7ffd00fffac0 sp 0x7ffd00fffab8
READ of size 1 at 0x622000068f05 thread T0
        #0 0x55702e in dehuffman32 (build/clang/release/test/libeep/compress_epochmux+0x55702e)
        #1 0x5595e8 in decompchan (build/clang/release/test/libeep/compress_epochmux+0x5595e8)
        #2 0x55abbe in decompepoch_mux (build/clang/release/test/libeep/compress_epochmux+0x55abbe)
        */
        std::vector<char> bytes(compressed.size() * 2);
        std::fill(begin(bytes), end(bytes), 0);
        std::copy(begin(compressed), end(compressed), begin(bytes));
        const auto byte_data{ bytes.data() };

        std::vector<raw3::sraw_t> output(epoch_length * chanv.size());
        const auto output_data{ output.data() };

        std::chrono::microseconds sum{0};
        int decompressed{0};
        raw3::raw3_t* praw3{ decoder.get() };
        const int length{ cast(epoch_length, int{}, ok{}) };

        for (int i{0}; i < repetitions; ++i) {
            const auto start{ std::chrono::steady_clock::now() };
            decompressed = raw3::decompepoch_mux(praw3, byte_data, length, output_data);
            const auto stop{ std::chrono::steady_clock::now() };
            sum += std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        }

        const size_t decoded{ cast(decompressed, size_t{}, ok{}) };
        if (bytes.size() < decoded) {
            std::ostringstream oss;
            oss << "libeep: decoder memory fault. exceeded allocated memory by " << (decoded - bytes.size()) << " bytes";
            throw std::runtime_error(oss.str());
        }

        return { output, sum };
    }


    auto encode(std::vector<raw3::sraw_t> input, int repetitions = 1) -> std::pair<std::vector<uint8_t>, std::chrono::microseconds> {
        const auto max_size{ max_block_size(begin(input), end(input), reflib{}) };
        std::vector<char> bytes(as_sizet_unchecked(max_size) * 2); // double space because libeep tends to write carelessly. TODO: guard the multiplication
        const auto byte_data{ bytes.data() };
        auto input_data{ input.data() };

        std::chrono::microseconds sum{0};
        int compressed{0};
        raw3::raw3_t* praw3{ encoder.get() };
        const int length{ cast(epoch_length, int{}, ok{}) };

        for (int i{0}; i < repetitions; ++i) {
            const auto start{ std::chrono::steady_clock::now() };
            compressed = raw3::compepoch_mux(praw3, input_data, length, byte_data);
            const auto stop{ std::chrono::steady_clock::now() };
            sum += std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        }

        const size_t encoded{ cast(compressed, size_t{}, ok{}) };

        if (bytes.size() < encoded) {
            std::ostringstream oss;
            oss << "libeep: encoder memory fault. exceeded allocated memory by " << (encoded - bytes.size()) << " bytes";
            throw std::runtime_error(oss.str());
        }

        std::vector<uint8_t> result(encoded);
        std::copy(begin(bytes), begin(bytes) + encoded, begin(result));
        return { result, sum };
    }
};

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */

