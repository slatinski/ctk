/*
Copyright 2015-2021 Velko Hristov
This file is part of CntToolKit.
SPDX-License-Identifier: LGPL-3.0+

CntToolKit is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CntToolKit is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with CntToolKit.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <tuple>
#include <algorithm>

#include "compress/bit_stream.h"
#include "qcheck.h"


using namespace ctk::impl;
using namespace qcheck;


template<typename T>
auto make_bytes(size_t uncompressed, T/* type tag */) -> std::vector<uint8_t> {
    constexpr const size_t nexc{ sizeof(T) * 8 };
    const size_t bits{ 80 + (nexc + nexc) * uncompressed };
    const size_t size{ bits / 8 + 1 };

    std::vector<uint8_t> xs(size);
    std::fill(begin(xs), end(xs), 0);
    return xs;
}

template<typename T>
auto make_bytes(const std::vector<T>& xs) -> std::vector<uint8_t> {
    return make_bytes(xs.size(), T{});
}


struct block_param
{
    unsigned data_size;
    unsigned method;
    unsigned n;
    unsigned nexc;
};

template<typename T, typename Format>
auto generate_block_parameters(random_source& rnd, T, Format) -> block_param {
    // generates encoding data size
    // interval [0, 3] one of { one_byte, two_bytes, four_bytes, eight_bytes }
    unsigned size{ gen(static_cast<size_t>(Format::as_size(T{})), rnd, unsigned{}) };
    const auto data_size{ Format::decode_size(size) };
    const bit_count bits{ field_width_master(data_size) };
    const bit_count::value_type max_size{ bits };
    const unsigned size_in_bits{ static_cast<unsigned>(max_size) };

    // generates encoding method.
    // interval [0, 3] - one of { copy, time, time2, chan }
    unsigned method{ gen(3, rnd, unsigned{}) };

    // generates values for n and nexc bits, such that 2 <= n && n <= nexc && nexc <= sizeof(encoding data size) * 8.
    // interval [2, sizeof(encoding data size) * 8]
    unsigned n{ choose(2U, size_in_bits, rnd) };
    unsigned nexc{ choose(2U, size_in_bits, rnd) };
    if (nexc < n) {
        std::swap(n, nexc);
    }

    // method copy
    if (method == 0) {
        n = size_in_bits;
        nexc = size_in_bits;
    }

    return { size, method, n, nexc };
}

template<typename T, typename Format>
auto encode_uncompressed(const block_param& param, const std::vector<T>& uncompressed, const std::vector<bool> encoding_map, Format) -> std::tuple<std::vector<uint8_t>, std::vector<bool>, encoding_size, bit_count, bit_count, size_t> {
    if (uncompressed.empty()) {
        return std::make_tuple(std::vector<uint8_t>{}, std::vector<bool>{}, encoding_size::two_bytes, bit_count{ 2 }, bit_count{ 2 }, 0);
    }
    std::vector<uint8_t> xs{ make_bytes(uncompressed) };
    bit_writer bits{ begin(xs), end(xs) };

    const auto data_size{ Format::decode_size(param.data_size) };
    const auto n_width{ Format::field_width_n(data_size) };
    const bit_count nbit{ param.n };
    const bit_count nexcbit{ param.nexc };
    const T marker{ exception_marker<T>(nbit) };

    bits.write(field_width_encoding(), Format::encode_size(data_size));
    bits.write(field_width_encoding(), param.method);
    if (param.method == static_cast<unsigned>(encoding_method::copy)) {
        bits.write(bit_count{ 4 }, unsigned{ 0 });

        for (T x : uncompressed) {
            bits.write(field_width_master(data_size), x);
        }
    }
    else {
        bits.write(n_width, param.n);
        bits.write(n_width, param.nexc);
        bits.write(field_width_master(data_size), uncompressed[0]);

        if (param.n == param.nexc) {
            for (size_t i{ 1 }; i < uncompressed.size(); ++i) {
                bits.write(nbit, uncompressed[i]);
            }
        }
        else {
            for (size_t i{ 1 }; i < uncompressed.size(); ++i) {
                if (encoding_map[i]) {
                    bits.write(nbit, marker);
                    bits.write(nexcbit, uncompressed[i]);
                }
                else {
                    bits.write(nbit, uncompressed[i]);
                }
            }
        }
    }
    xs.resize(static_cast<size_t>(std::distance(begin(xs), bits.flush())));

    return std::make_tuple(xs, encoding_map, data_size, nbit, nexcbit, uncompressed.size());
}

template<typename T, typename Format>
auto generate_encoded_block(size_t size, random_source& rnd, T, Format) -> std::tuple<std::vector<uint8_t>, std::vector<bool>, encoding_size, bit_count, bit_count, size_t> {
    const auto param{ generate_block_parameters(rnd, T{}, Format{}) };
    const bit_count nbit{ param.n };

    // interprets size to mean the number of words used to generte the compresed byte stream.
    std::vector<bool> encoding_map; // true - exception (n + nexc bits), false - regular (n bits)
    encoding_map.reserve(size);
    for (size_t i{ 0 }; i < size; ++i) {
        encoding_map.push_back(gen(size, rnd, bool{}));
    }

    std::vector<T> xs;
    xs.reserve(size);
    if (param.n == param.nexc) {
        for (size_t i{ 0 }; i < size; ++i) {
            xs.push_back(gen(size, rnd, T{}));
        }
    }
    else {
        for (size_t i{ 0 }; i < size; ++i) {
            T x{ gen(size, rnd, T{}) };
            if (!encoding_map[i]) {
                while (is_exception_marker(x, nbit)) {
                    x = gen(size, rnd, T{});
                }
            }
            xs.push_back(x);
        }
    }

    return encode_uncompressed(param, xs, encoding_map, Format{});
}

