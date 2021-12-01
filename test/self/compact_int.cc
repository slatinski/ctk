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

#include "compress/int.h"

namespace ctk { namespace impl { namespace test {

TEST_CASE("reading and writing of well known data", "[correct]") {
    const unsigned i0{ 0 };
    const std::vector<uint8_t> i0_b1{ write_compact_int(i0, bit_count{ 1 }) };
    const auto [o0, e0] = read_compact_int(begin(i0_b1), end(i0_b1), unsigned{});
    REQUIRE(e0 == end(i0_b1));
    REQUIRE(o0 == i0);

    const unsigned i1{ 1 };
    const std::vector<uint8_t> i1_b1{ write_compact_int(i1, bit_count{ 1 }) };
    const auto [o1, e1] = read_compact_int(begin(i1_b1), end(i1_b1), unsigned{});
    REQUIRE(e1 == end(i1_b1));
    REQUIRE(o1 == i1);

    const unsigned i256{ 256 };
    const std::vector<uint8_t> i256_b9{ write_compact_int(i256, bit_count{ 9 }) };
    const auto [o256, e256] = read_compact_int(begin(i256_b9), end(i256_b9), unsigned{});
    REQUIRE(e256 == end(i256_b9));
    REQUIRE(o256 == i256);

    const unsigned i341{ 341 };
    const std::vector<uint8_t> i341_b9{ write_compact_int(i341, bit_count{ 9 }) };
    const auto [o341, e341] = read_compact_int(begin(i341_b9), end(i341_b9), unsigned{});
    REQUIRE(e341 == end(i341_b9));
    REQUIRE(o341 == i341);

}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
