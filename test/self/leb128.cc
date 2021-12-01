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

#include "compress/leb128.h"

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


    template<typename T>
    auto roundtrip_unsigned(T input) -> void {
        const std::vector<uint8_t> bytes{ encode_uleb128_v(input) };
        const T output{ decode_uleb128_v(bytes, T{}) };
        REQUIRE(output == input);
    }

    TEST_CASE("unsigned single number roundtrip", "[consistency]") {
        roundtrip_unsigned(std::numeric_limits<uint8_t>::min());
        roundtrip_unsigned(std::numeric_limits<uint16_t>::min());
        roundtrip_unsigned(std::numeric_limits<uint32_t>::min());
        roundtrip_unsigned(std::numeric_limits<uint64_t>::min());

        roundtrip_unsigned(std::numeric_limits<uint8_t>::max());
        roundtrip_unsigned(std::numeric_limits<uint16_t>::max());
        roundtrip_unsigned(std::numeric_limits<uint32_t>::max());
        roundtrip_unsigned(std::numeric_limits<uint64_t>::max());

        for (uint16_t i{ 0 }; i < std::numeric_limits<uint16_t>::max(); ++i) {
            roundtrip_unsigned(i);
        }

        for (size_t i{ 0 }; i < example_u.size(); ++i) {
            const std::vector<uint8_t> bytes{ encode_uleb128_v(example_u[i].n) };
            REQUIRE(bytes == example_u[i].bytes);
        }
    }


    template<typename T>
    auto roundtrip_signed(T input) -> void {
        const std::vector<uint8_t> bytes{ encode_sleb128_v(input) };
        const T output{ decode_sleb128_v(bytes, T{}) };
        REQUIRE(output == input);
    }

    TEST_CASE("signed single number roundtrip", "[consistency]") {
        std::vector<uint8_t> empty;
        auto [zero, next]{ leb128::decode(begin(empty), end(empty), 12, leb128::sleb{}) };
        REQUIRE(next == end(empty));
        REQUIRE(zero == 0);

        roundtrip_signed(std::numeric_limits<int8_t>::min());
        roundtrip_signed(std::numeric_limits<int16_t>::min());
        roundtrip_signed(std::numeric_limits<int32_t>::min());
        roundtrip_signed(std::numeric_limits<int64_t>::min());

        roundtrip_signed(std::numeric_limits<int8_t>::max());
        roundtrip_signed(std::numeric_limits<int16_t>::max());
        roundtrip_signed(std::numeric_limits<int32_t>::max());
        roundtrip_signed(std::numeric_limits<int64_t>::max());

        for (int16_t i{ std::numeric_limits<int16_t>::min() }; i < std::numeric_limits<int16_t>::max(); ++i) {
            roundtrip_signed(i);
        }

        for (size_t i{ 0 }; i < example_s.size(); ++i) {
            const std::vector<uint8_t> bytes{ encode_sleb128_v(example_s[i].n) };
            REQUIRE(bytes == example_s[i].bytes);
        }
    }

    template<typename T>
    auto multiple_signed(const std::vector<T>& xs) -> void {
        std::vector<uint8_t> bytes(xs.size() * (sizeof(T) + 2 /* sizeof(T) <= 8 */));
        auto first{ begin(bytes) };
        auto last{ end(bytes) };

        for (T x : xs) {
            first = encode_sleb128(x, first, last);
        }
        last = first;
        first = begin(bytes);

        T y;
        for (T x : xs) {
            std::tie(y, first) = decode_sleb128(first, last, T{});
            REQUIRE(y == x);
        }

        REQUIRE(first == last);
    }

    template<typename T>
    auto multiple_unsigned(const std::vector<T>& xs) -> void {
        std::vector<uint8_t> bytes(xs.size() * (sizeof(T) + 2 /* sizeof(T) <= 8 */));
        auto first{ begin(bytes) };
        auto last{ end(bytes) };

        for (T x : xs) {
            first = encode_uleb128(x, first, last);
        }
        last = first;
        first = begin(bytes);

        T y;
        for (T x : xs) {
            std::tie(y, first) = decode_uleb128(first, last, T{});
            REQUIRE(y == x);
        }

        REQUIRE(first == last);
    }


    TEST_CASE("multiple numbers", "[consistency]") {
        std::vector<int32_t> consecutive_s(26);
        std::iota(begin(consecutive_s), end(consecutive_s), -12);
        multiple_signed(consecutive_s);

        std::vector<int> well_known_s(example_s.size());
        std::transform(begin(example_s), end(example_s), begin(well_known_s), [](const auto& x) -> int { return x.n; });
        multiple_signed(well_known_s);

        std::vector<unsigned> well_known_u(example_u.size());
        std::transform(begin(example_u), end(example_u), begin(well_known_u), [](const auto& x) -> unsigned { return x.n; });
        multiple_unsigned(well_known_u);
    }

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
