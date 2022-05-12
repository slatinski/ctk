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

#include <algorithm>
#include <tuple>
#include <cassert>
#include <iostream>

#include "compress/bit_stream.h"
#include "compress/matrix.h" // count_raw3
#include "qcheck.h"
#include "make_block.h" // print_vector


using namespace ctk::impl;
using namespace qcheck;


template<typename T>
using words_bytes = std::tuple<std::vector<T>, std::vector<uint8_t>>;

template<typename T>
struct make_encoded
{
    random_source* _random;

    make_encoded(random_source& r, T)
        : _random{ &r } {
    }

    auto operator()(size_t size) -> words_bytes<T> {
        const auto xs{ gen(size, *_random, std::vector<T>{}) };
        std::vector<bit_count> sizes(xs.size());
        std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

        std::vector<uint8_t> bytes(xs.size() * sizeof(T) * 2);
        std::fill(begin(bytes), end(bytes), uint8_t{ 0 });

        bit_writer writer{ begin(bytes), end(bytes) };
        for (size_t i{ 0 }; i < xs.size(); ++i) {
            writer.write(sizes[i], xs[i]);
        }
        const auto next{ writer.flush() };
        bytes.resize(static_cast<size_t>(std::distance(begin(bytes), next)));

        return std::make_tuple(xs, bytes);
    }
};




template<typename T>
struct property_encoded_size : arguments<std::vector<T>>
{
    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        std::vector<bit_count> sizes(xs.size());
        std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

        const bit_count bits{ std::accumulate(begin(sizes), end(sizes), bit_count{ 0 }) };
        const byte_count expected{ as_bytes(bits) };

        std::vector<uint8_t> bytes(xs.size() * sizeof(T) * 2);
        bit_writer writer{ begin(bytes), end(bytes) };
        for (size_t i{ 0 }; i < xs.size(); ++i) {
            writer.write(sizes[i], xs[i]);
        }

        const auto d{ std::distance(begin(bytes), writer.flush()) };
        const byte_count written{ static_cast<byte_count::value_type>(d) };

        return written == expected;
    }

    virtual auto print(std::ostream& os, const std::vector<T>& xs) const -> std::ostream& override final {
        return print_vector(os, xs);
    }
};


template<typename T>
struct encode_decode : arguments<std::vector<T>>
{
    virtual auto accepts(const std::vector<T>& xs) const -> bool override final {
        return !xs.empty();
    }

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        std::vector<bit_count> sizes(xs.size());
        std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

        std::vector<uint8_t> bytes(xs.size() * sizeof(T) * 2);
        std::fill(begin(bytes), end(bytes), uint8_t{ 0 });
        bit_writer writer{ begin(bytes), end(bytes) };
        for (size_t i{ 0 }; i < xs.size(); ++i) {
            writer.write(sizes[i], xs[i]);
        }
        const auto next{ writer.flush() };

        std::vector<T> ys;
        ys.reserve(xs.size());
        bit_reader reader{ begin(bytes), next };
        for (bit_count n : sizes) {
            ys.push_back(reader.read(n, T{}));
        }

        return ys == xs;
    }

    virtual auto print(std::ostream& os, const std::vector<T>& xs) const -> std::ostream& override final {
        return print_vector(os, xs);
    }
};

template<typename T>
struct decode_encode : arguments<words_bytes<T>>
{
    virtual auto accepts(const words_bytes<T>& args) const -> bool override final {
        const auto& bytes{ std::get<1>(args) };
        return !bytes.empty();
    }

    virtual auto holds(const words_bytes<T>& args) const -> bool override final {
        const auto& xs{ std::get<0>(args) };
        const auto& bytes{ std::get<1>(args) };

        std::vector<bit_count> sizes(xs.size());
        std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

        std::vector<T> ys;
        ys.reserve(xs.size());
        bit_reader reader{ begin(bytes), end(bytes) };
        for (bit_count n : sizes) {
            ys.push_back(reader.read(n, T{}));
        }

        if (reader.current() != end(bytes)) {
            return false;
        }

        std::vector<uint8_t> encoded(ys.size() * sizeof(T) * 2);
        std::fill(begin(encoded), end(encoded), uint8_t{ 0 });
        bit_writer writer{ begin(encoded), end(encoded) };
        for (size_t i{ 0 }; i < ys.size(); ++i) {
            writer.write(sizes[i], ys[i]);
        }
        const auto next{ writer.flush() };
        encoded.resize(static_cast<size_t>(std::distance(begin(encoded), next)));

        return encoded == bytes;
    }

    virtual auto print(std::ostream& os, const words_bytes<T>& args) const -> std::ostream& override final {
        const auto& bytes{ std::get<1>(args) };
        return print_vector(os, bytes);
    }
};



auto main(int, char*[]) -> int {
    random_source r;

    bool ok{ true };

    ok &= check("encoding size 8 bit",  property_encoded_size<uint8_t>{},  make_vectors{ r, uint8_t{} });
    ok &= check("encoding size 16 bit", property_encoded_size<uint16_t>{}, make_vectors{ r, uint16_t{} });
    ok &= check("encoding size 32 bit", property_encoded_size<uint32_t>{}, make_vectors{ r, uint32_t{} });
    ok &= check("encoding size 64 bit", property_encoded_size<uint64_t>{}, make_vectors{ r, uint64_t{} });

    ok &= check("enc/dec 8 bit",  encode_decode<uint8_t>{},  make_vectors{ r, uint8_t{} });
    ok &= check("enc/dec 16 bit", encode_decode<uint16_t>{}, make_vectors{ r, uint16_t{} });
    ok &= check("enc/dec 32 bit", encode_decode<uint32_t>{}, make_vectors{ r, uint32_t{} });
    ok &= check("enc/dec 64 bit", encode_decode<uint64_t>{}, make_vectors{ r, uint64_t{} });

    ok &= check("dec/enc 8 bit",  decode_encode<uint8_t>{},  make_encoded{ r, uint8_t{} });
    ok &= check("dec/enc 16 bit", decode_encode<uint16_t>{}, make_encoded{ r, uint16_t{} });
    ok &= check("dec/enc 32 bit", decode_encode<uint32_t>{}, make_encoded{ r, uint32_t{} });
    ok &= check("dec/enc 64 bit", decode_encode<uint64_t>{}, make_encoded{ r, uint64_t{} });

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

