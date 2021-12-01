#pragma once

#include "api_data.h"
#include "container/file_reflib.h"
#include <vector>
#include <chrono>
#include <sstream>

namespace ctk { namespace impl { namespace test {

template<typename T, typename Format>
struct libthis
{
    measurement_count length;
    std::vector<int16_t> order;
    matrix_decoder_general<T, Format> decoder;
    matrix_encoder_general<T, Format> encoder;


    libthis(measurement_count length, const std::vector<int16_t>& order, T, Format)
    : length{ length }
    , order{ order }
    {
        if (length < 1) {
            throw std::runtime_error("invalid epoch length");
        }

        decoder.row_order(order);
        encoder.row_order(order);
    }


    auto decode(const std::vector<uint8_t>& compressed, int repetitions = 1) -> std::pair<std::vector<T>, std::chrono::microseconds> {
        const auto start{ std::chrono::steady_clock::now() };

        std::vector<T> output(as_sizet_unchecked(length) * order.size());
        const auto first{ begin(output) };
        const auto last{ end(output) };
        const auto first_byte{ begin(compressed) };
        const auto last_byte{ end(compressed) };
        auto next_byte{ begin(compressed) };
        dma_array transfer{ first, last, column_major2row_major{} };

        for (int x{ 0 }; x < repetitions; ++x) {
            decoder(transfer, first_byte, last_byte, length);
        }

        const auto allocated{ std::distance(first_byte, last_byte) };
        const auto consumed{ std::distance(first_byte, next_byte) };
        if (allocated < consumed) {
            std::ostringstream oss;
            oss << "libthis: decoder memory fault. exceeded allocated memory by " << (consumed - allocated) << " bytes";
            throw std::runtime_error(oss.str());
        }

        const auto stop{ std::chrono::steady_clock::now() };
        std::chrono::microseconds sum{ std::chrono::duration_cast<std::chrono::microseconds>(stop - start) };

        return { output, sum };
    }


    auto encode(const std::vector<T>& input, int repetitions = 1) -> std::pair<std::vector<uint8_t>, std::chrono::microseconds> {
        const auto start{ std::chrono::steady_clock::now() };

        const auto first{ begin(input) };
        const auto last{ end(input) };
        const dma_array transfer{ first, last, column_major2row_major{} };

        const auto max_size{ max_encoded_size({ sensor_count{ vsize(order) }, length }, Format{}, T{}) };
        std::vector<uint8_t> bytes(as_sizet_unchecked(max_size));
        const auto first_byte{ begin(bytes) };
        const auto last_byte{ end(bytes) };
        auto next_byte{ begin(bytes) };

        for (int x{ 0 }; x < repetitions; ++x) {
            next_byte = encoder(transfer, length, first_byte, last_byte);
        }

        const auto output_size{ cast(std::distance(first_byte, next_byte), size_t{}, ok{}) };
        if (bytes.size() < output_size) {
            std::ostringstream oss;
            oss << "libthis: encoder memory fault. exceeded allocated memory by " << (output_size - bytes.size()) << " bytes";
            throw std::runtime_error(oss.str());
        }

        bytes.resize(output_size);

        const auto stop{ std::chrono::steady_clock::now() };
        std::chrono::microseconds sum{ std::chrono::duration_cast<std::chrono::microseconds>(stop - start) };
        return { bytes, sum };
    }
};

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */

