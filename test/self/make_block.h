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

#include "compress/matrix.h" // count_raw3
#include "qcheck.h"


using namespace ctk::impl;
using namespace qcheck;


template<typename T>
auto make_bytes(const std::vector<T>& xs) -> std::vector<uint8_t> {
    constexpr const size_t nexc{ sizeof(T) * 8 };
    const size_t bits{ 80 + (nexc + nexc) * xs.size() };
    const size_t size{ bits / 8 + 1 };

    std::vector<uint8_t> bytes(size);
    std::fill(begin(bytes), end(bytes), 0);
    return bytes;
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

    // generates values for n and nexc bits, such as 2 <= n && n <= nexc && nexc <= sizeof(encoding data size) * 8.
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
auto generate_encoded(size_t size, const block_param& param, random_source& rnd, T, Format) -> std::tuple<std::vector<uint8_t>, size_t> {
    // interprets size to mean the number of words used to generte the compresed byte stream.
    if (size == 0) {
        return std::make_tuple(std::vector<uint8_t>{}, 0);
    }
    const size_t max_x{ static_cast<size_t>(std::pow(2, param.nexc - 1/* signum */) - 1) };
    std::vector<T> xs;
    xs.reserve(size);
    for (size_t i{ 0 }; i < size; ++i) {
        xs.push_back(gen(max_x, rnd, T{}));
    }

    // size of each input measured in bits
    std::vector<bit_count> sizes(xs.size());
    std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

    const bit_count n{ static_cast<bit_count::value_type>(param.n) };
    const bit_count nexc{ static_cast<bit_count::value_type>(param.nexc) };
    assert(true == std::accumulate(begin(sizes), end(sizes), true, [nexc](bool a, bit_count s) -> bool { return a && s <= nexc; }));

    // encoding map for variable width encoding
    std::vector<bool> encoding_map(xs.size());
    std::transform(begin(xs), end(xs), begin(sizes), begin(encoding_map), is_exception{ n });
    auto emap{ begin(encoding_map) };

    auto bytes{ make_bytes(xs) };
    bit_writer writer{ begin(bytes), end(bytes) };

    const auto data_size{ Format::decode_size(param.data_size) };
    const encoding_method method{ static_cast<int>(param.method) };
    const bit_count::value_type nbit{ param.n };
    const bit_count::value_type nexcbit{ param.nexc };

    const auto last{ encode_block(begin(xs), end(xs), emap, writer, data_size, method, bit_count{ nbit }, bit_count{ nexcbit }, Format{}) };
    const auto first{ begin(bytes) };
    bytes.resize(static_cast<size_t>(std::distance(first, last)));

    return std::make_tuple(bytes, size);
}


template<typename T, typename Format>
auto generate_block(size_t size, random_source& rnd, T, Format) -> std::tuple<std::vector<uint8_t>, size_t> {
    const auto param{ generate_block_parameters(rnd, T{}, Format{}) };
    return generate_encoded(size, param, rnd, T{}, Format{});
}

