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

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../catch.hpp"

#include <iostream>
#include <numeric>

#include "container/leb128.h"

namespace ctk { namespace impl { namespace test {

    struct dwarf_u
    {
        unsigned n;
        std::vector<uint8_t> bytes;
    };

    struct dwarf_s
    {
        int n;
        std::vector<uint8_t> bytes;
    };

    // DWARF Debugging Information Format, Version 4
    // SECTION 7-- DATA REPRESENTATION
    // 7.6 Variable Length Data
    // Figure 22. Examples of unsigned LEB128 encodings
    const std::vector<dwarf_u> example_u{
        { 2, { 2 } },
        { 127, { 127 } },
        { 128, { 0 + 0x80, 1 } },
        { 129, { 1 + 0x80, 1 } },
        { 130, { 2 + 0x80, 1 } },
        { 12857, { 57 + 0x80, 100 } }
    };

    // DWARF Debugging Information Format, Version 4
    // SECTION 7-- DATA REPRESENTATION
    // 7.6 Variable Length Data
    // Figure 23. Examples of signed LEB128 encodings
    const std::vector<dwarf_s> example_s{
        { 2, { 2 } },
        { -2, { 0x7e } },
        { 127, { 127 + 0x80, 0 } },
        { -127, { 1 + 0x80, 0x7f } },
        { 128, { 0 + 0x80, 1 } },
        { -128, { 0 + 0x80, 0x7f } },
        { 129, { 1 + 0x80, 1 } },
        { -129, { 0x7f + 0x80, 0x7e } }
    };



    auto input_consecutive_backward() -> std::vector<int64_t> {
        std::vector<int64_t> xs;
        for (int64_t i{ std::numeric_limits<int64_t>::max() }; i > std::numeric_limits<int64_t>::max() - 4096; --i) {
            xs.push_back(i);
        }
        return xs;
    }

    auto well_known_ints() -> std::vector<int> {
        std::vector<int> xs(example_s.size());
        std::transform(begin(example_s), end(example_s), begin(xs), [](const auto& x) -> int { return x.n; });
        return xs;
    }

    auto well_known_unsigned_ints() -> std::vector<unsigned> {
        std::vector<unsigned> xs(example_u.size());
        std::transform(begin(example_u), end(example_u), begin(xs), [](const auto& x) -> unsigned { return x.n; });
        return xs;
    }

    auto all_int16s() -> std::vector<int16_t> {
        std::vector<int16_t> xs(std::numeric_limits<int16_t>::max() - std::numeric_limits<int16_t>::min());
        std::iota(begin(xs), end(xs), std::numeric_limits<int16_t>::min());
        return xs;
    }

    auto all_uint16s() -> std::vector<uint16_t> {
        std::vector<uint16_t> xs(std::numeric_limits<uint16_t>::max());
        std::iota(begin(xs), end(xs), uint16_t{ 0 });
        return xs;
    }

    auto around_zero() -> std::vector<int32_t> {
        std::vector<int32_t> xs(2048);
        std::iota(begin(xs), end(xs), int32_t{ -1024 });
        return xs;
    }



    template<typename T>
    auto roundtrip(T input) -> void {
        const std::vector<uint8_t> bytes{ encode_leb128_v(input) };
        const T output{ decode_leb128_v(bytes, T{}) };
        REQUIRE(output == input);
    }

    TEST_CASE("single number roundtrip", "[consistency]") {
        std::vector<uint8_t> empty;
        auto [zero, next]{ decode_leb128(begin(empty), end(empty), int{}) };
        REQUIRE(next == end(empty));
        REQUIRE(zero == 0);

        roundtrip(std::numeric_limits<int8_t>::min());
        roundtrip(std::numeric_limits<int16_t>::min());
        roundtrip(std::numeric_limits<int32_t>::min());
        roundtrip(std::numeric_limits<int64_t>::min());
        roundtrip(std::numeric_limits<int8_t>::max());
        roundtrip(std::numeric_limits<int16_t>::max());
        roundtrip(std::numeric_limits<int32_t>::max());
        roundtrip(std::numeric_limits<int64_t>::max());

        roundtrip(std::numeric_limits<uint8_t>::min());
        roundtrip(std::numeric_limits<uint16_t>::min());
        roundtrip(std::numeric_limits<uint32_t>::min());
        roundtrip(std::numeric_limits<uint64_t>::min());
        roundtrip(std::numeric_limits<uint8_t>::max());
        roundtrip(std::numeric_limits<uint16_t>::max());
        roundtrip(std::numeric_limits<uint32_t>::max());
        roundtrip(std::numeric_limits<uint64_t>::max());

        for (int16_t i{ std::numeric_limits<int16_t>::min() }; i < std::numeric_limits<int16_t>::max(); ++i) {
            roundtrip(i);
        }

        for (uint16_t i{ 0 }; i < std::numeric_limits<uint16_t>::max(); ++i) {
            roundtrip(i);
        }
    }

    TEST_CASE("well known representations", "[correct]") {
        for (size_t i{ 0 }; i < example_s.size(); ++i) {
            const std::vector<uint8_t> bytes{ encode_leb128_v(example_s[i].n) };
            REQUIRE(bytes == example_s[i].bytes);
        }

        for (size_t i{ 0 }; i < example_u.size(); ++i) {
            const std::vector<uint8_t> bytes{ encode_leb128_v(example_u[i].n) };
            REQUIRE(bytes == example_u[i].bytes);
        }
    }


    template<typename T>
    auto roundtrip_multiple(const std::vector<T>& xs) -> void {
        std::vector<uint8_t> bytes(xs.size() * leb128::max_bytes(T{}));
        auto first{ begin(bytes) };
        auto last{ end(bytes) };

        for (T x : xs) {
            first = encode_leb128(x, first, last);
        }
        last = first;
        first = begin(bytes);

        T y;
        for (T x : xs) {
            std::tie(y, first) = decode_leb128(first, last, T{});
            REQUIRE(y == x);
        }

        REQUIRE(first == last);
    }

    TEST_CASE("multiple numbers", "[consistency]") {
        roundtrip_multiple(around_zero());
        roundtrip_multiple(input_consecutive_backward());
        roundtrip_multiple(well_known_ints());
        roundtrip_multiple(well_known_unsigned_ints());
    }

    TEST_CASE("invalid input", "[correct]") {
        const std::vector<uint8_t> only_continuation{ 0x80 };
        REQUIRE_THROWS(decode_leb128(begin(only_continuation), end(only_continuation), int16_t{}));

        const std::vector<uint8_t> extra_continuation{ 0x80, 0x80 };
        REQUIRE_THROWS(decode_leb128(begin(extra_continuation), end(extra_continuation), int16_t{}));

        const std::vector<uint8_t> not_enough_output_bits{ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x1 };
        REQUIRE_THROWS(decode_leb128(begin(not_enough_output_bits), end(not_enough_output_bits), int64_t{}));
    }


    template<typename T>
    auto roundtrip_file(const std::vector<T>& xs) -> void {
        const std::filesystem::path temporary{ "leb128.bin" };
        {
            file_ptr f{ open_w(temporary) };
            for (T x : xs) {
                write_leb128(f.get(), x);
            }
        }
        {
            file_ptr f{ open_r(temporary) };
            for (T x : xs) {
                const T y{ read_leb128(f.get(), T{}) };
                REQUIRE(y == x);
            }
        }

        std::filesystem::remove(temporary);
    }


    TEST_CASE("file io", "[consistency]") {
        roundtrip_file(well_known_ints());
        roundtrip_file(well_known_unsigned_ints());
        roundtrip_file(input_consecutive_backward());
        roundtrip_file(all_int16s());
        roundtrip_file(all_uint16s());
    }

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
