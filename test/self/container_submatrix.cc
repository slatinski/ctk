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

#include "container/file_reflib.h"

#include <vector>

using namespace ctk::impl;


namespace ctk { namespace impl { namespace test {


TEST_CASE("first_i last_i", "[concistency]") {
    const ptrdiff_t height{ 3 };
    const ptrdiff_t length{ 7 };
    std::vector<int> input {
        11, 12, 13, 14, 15, 16, 17,
        21, 22, 23, 24, 25, 26, 27,
        31, 32, 33, 34, 35, 36, 37
    };
    const auto first{ begin(input) };
    const auto last{ end(input) };

    using I = decltype(begin(input));
    buf_win<I> w_input{ first, last, height, length }; 

    REQUIRE_THROWS(first_i(w_input, length));
    REQUIRE_THROWS(first_i(w_input, -1));

    REQUIRE_THROWS(last_i(w_input, 0, length + 1));
    REQUIRE_THROWS(last_i(w_input, 1, length));
    REQUIRE_THROWS(last_i(w_input, -1, 1));
    REQUIRE_THROWS(last_i(w_input, 1, -1));

    ptrdiff_t offset{ 0 };
    REQUIRE(*first_i(w_input, offset) == 11);
    REQUIRE(*last_i(w_input, offset, 1) == 12);
    REQUIRE(*last_i(w_input, offset, 2) == 13);
    REQUIRE(*last_i(w_input, offset, 3) == 14);
    REQUIRE(*last_i(w_input, offset, 4) == 15);
    REQUIRE(*last_i(w_input, offset, 5) == 16);
    REQUIRE(*last_i(w_input, offset, 6) == 17);
    REQUIRE(*last_i(w_input, offset, 7) == 21);
    REQUIRE_THROWS(last_i(w_input, offset, 8));

    offset = 1;
    REQUIRE(*first_i(w_input, offset) == 12);
    REQUIRE(*last_i(w_input, offset, 1) == 13);
    REQUIRE(*last_i(w_input, offset, 2) == 14);
    REQUIRE(*last_i(w_input, offset, 3) == 15);
    REQUIRE(*last_i(w_input, offset, 4) == 16);
    REQUIRE(*last_i(w_input, offset, 5) == 17);
    REQUIRE(*last_i(w_input, offset, 6) == 21);
    REQUIRE_THROWS(last_i(w_input, offset, 7));

    offset = 2;
    REQUIRE(*first_i(w_input, offset) == 13);
    REQUIRE(*last_i(w_input, offset, 1) == 14);
    REQUIRE(*last_i(w_input, offset, 2) == 15);
    REQUIRE(*last_i(w_input, offset, 3) == 16);
    REQUIRE(*last_i(w_input, offset, 4) == 17);
    REQUIRE(*last_i(w_input, offset, 5) == 21);
    REQUIRE_THROWS(last_i(w_input, offset, 6));

    offset = 3;
    REQUIRE(*first_i(w_input, offset) == 14);
    REQUIRE(*last_i(w_input, offset, 1) == 15);
    REQUIRE(*last_i(w_input, offset, 2) == 16);
    REQUIRE(*last_i(w_input, offset, 3) == 17);
    REQUIRE(*last_i(w_input, offset, 4) == 21);
    REQUIRE_THROWS(last_i(w_input, offset, 5));

    offset = 4;
    REQUIRE(*first_i(w_input, offset) == 15);
    REQUIRE(*last_i(w_input, offset, 1) == 16);
    REQUIRE(*last_i(w_input, offset, 2) == 17);
    REQUIRE(*last_i(w_input, offset, 3) == 21);
    REQUIRE_THROWS(last_i(w_input, offset, 4));

    offset = 5;
    REQUIRE(*first_i(w_input, offset) == 16);
    REQUIRE(*last_i(w_input, offset, 1) == 17);
    REQUIRE(*last_i(w_input, offset, 2) == 21);
    REQUIRE_THROWS(last_i(w_input, offset, 3));

    offset = 6;
    REQUIRE(*first_i(w_input, offset) == 17);
    REQUIRE(*last_i(w_input, offset, 1) == 21);
    REQUIRE_THROWS(last_i(w_input, offset, 2));

    offset = 7;
    REQUIRE_THROWS(first_i(w_input, offset));
    REQUIRE_THROWS(first_i(w_input, -1));
}


TEST_CASE("submatrix", "[consistency]") {
    const ptrdiff_t heigth{ 3 };
    const ptrdiff_t in_length{ 4 };
    std::vector<int> input {
        11, 12, 13, 14,
        21, 22, 23, 24,
        31, 32, 33, 34
    };
    const auto first{ begin(input) };
    const auto last{ end(input) };

    const ptrdiff_t out_length{ 3 };
    std::vector<int> output(heigth * out_length);
    auto fout{ begin(output) };
    auto lout{ end(output) };
    auto fzero{ fout };
    auto fincompatible{ fout };

    using I = decltype(begin(input));
    using IOut = decltype(begin(output));

    buf_win<I> w_input{ first, last, heigth, in_length };
    buf_win<IOut> w_output{ fout, lout, heigth , out_length};
    buf_win<I> w_zero{ fzero, fzero, heigth, 0 };
    buf_win<IOut> w_incompatible{ fincompatible, fincompatible + out_length * 2, 2, out_length}; // 2 rows instead of 3

    REQUIRE(submatrix(0, w_incompatible, 0, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_incompatible, 0, w_output, 1) == fout); // amount == 0
    REQUIRE(submatrix(0, w_incompatible, 1, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_incompatible, 1, w_output, 1) == fout); // amount == 0
    REQUIRE_THROWS(submatrix(1, w_incompatible, 0, w_output, 0));
    REQUIRE_THROWS(submatrix(1, w_incompatible, 0, w_output, 1));
    REQUIRE_THROWS(submatrix(1, w_incompatible, 1, w_output, 0));
    REQUIRE_THROWS(submatrix(1, w_incompatible, 1, w_output, 1));

    REQUIRE(submatrix(0, w_input, 0, w_incompatible, 0) == fincompatible); // amount == 0
    REQUIRE(submatrix(0, w_input, 0, w_incompatible, 1) == fincompatible); // amount == 0
    REQUIRE(submatrix(0, w_input, 1, w_incompatible, 0) == fincompatible); // amount == 0
    REQUIRE(submatrix(0, w_input, 1, w_incompatible, 1) == fincompatible); // amount == 0
    REQUIRE_THROWS(submatrix(1, w_input, 0, w_incompatible, 0));
    REQUIRE_THROWS(submatrix(1, w_input, 0, w_incompatible, 1));
    REQUIRE_THROWS(submatrix(1, w_input, 1, w_incompatible, 0));
    REQUIRE_THROWS(submatrix(1, w_input, 1, w_incompatible, 1));

    REQUIRE(submatrix(0, w_zero, 0, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_zero, 0, w_output, 1) == fout); // amount == 0
    REQUIRE(submatrix(0, w_zero, 1, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_zero, 1, w_output, 1) == fout); // amount == 0
    REQUIRE_THROWS(submatrix(1, w_zero, 0, w_output, 0)); // input length < input offset + amount
    REQUIRE_THROWS(submatrix(1, w_zero, 0, w_output, 1)); // input length < input offset + amount
    REQUIRE_THROWS(submatrix(1, w_zero, 1, w_output, 0)); // input length < input offset + amount
    REQUIRE_THROWS(submatrix(1, w_zero, 1, w_output, 1)); // input length < input offset + amount

    REQUIRE(submatrix(0, w_input, 0, w_zero, 0) == fzero); // amount == 0
    REQUIRE(submatrix(0, w_input, 0, w_zero, 1) == fzero); // amount == 0
    REQUIRE(submatrix(0, w_input, 1, w_zero, 0) == fzero); // amount == 0
    REQUIRE(submatrix(0, w_input, 1, w_zero, 1) == fzero); // amount == 0
    REQUIRE_THROWS(submatrix(1, w_input, 0, w_zero, 0)); // output length < output offset + amount
    REQUIRE_THROWS(submatrix(1, w_input, 0, w_zero, 1)); // output length < output offset + amount
    REQUIRE_THROWS(submatrix(1, w_input, 1, w_zero, 0)); // output length < output offset + amount
    REQUIRE_THROWS(submatrix(1, w_input, 1, w_zero, 1)); // output length < output offset + amount

    REQUIRE(submatrix(0, w_input, 0, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 1, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 2, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 3, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 4, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 5, w_output, 0) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 0, w_output, 1) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 0, w_output, 2) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 0, w_output, 3) == fout); // amount == 0
    REQUIRE(submatrix(0, w_input, 0, w_output, 4) == fout); // amount == 0
    REQUIRE_THROWS(submatrix(4, w_input, 0, w_output, 0)); // output length < amount
    REQUIRE_THROWS(submatrix(5, w_input, 0, w_output, 0)); // input length < amount
    REQUIRE_THROWS(submatrix(1, w_input, 4, w_output, 0)); // input length < input offset + amount
    REQUIRE_THROWS(submatrix(1, w_input, 0, w_output, 3)); // output length < output offset + amount
    REQUIRE_THROWS(submatrix(-1, w_input, 0, w_output, 0)); // negative amount
    REQUIRE_THROWS(submatrix(1, w_input, -1, w_output, 0)); // negative input offset
    REQUIRE_THROWS(submatrix(1, w_input, 0, w_output, -1)); // negative output offset

    ptrdiff_t amount{0};
    ptrdiff_t i_offset{0};
    ptrdiff_t o_offset{0};

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 0;
    o_offset = 0;
    const std::vector<int> expected_0 {
        11, 0, 0,
        21, 0, 0,
        31, 0, 0
    };
    const auto r0{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_0);
    REQUIRE(r0 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 1;
    o_offset = 0;
    const std::vector<int> expected_1 {
        12, 0, 0,
        22, 0, 0,
        32, 0, 0
    };
    const auto r1{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_1);
    REQUIRE(r1 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 2;
    o_offset = 0;
    const std::vector<int> expected_2 {
        13, 0, 0,
        23, 0, 0,
        33, 0, 0
    };
    const auto r2{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_2);
    REQUIRE(r2 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 3;
    o_offset = 0;
    const std::vector<int> expected_3 {
        14, 0, 0,
        24, 0, 0,
        34, 0, 0
    };
    const auto r3{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_3);
    REQUIRE(r3 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 2;
    i_offset = 0;
    o_offset = 0;
    const std::vector<int> expected_4 {
        11, 12, 0,
        21, 22, 0,
        31, 32, 0
    };
    const auto r4{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_4);
    REQUIRE(r4 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 2;
    i_offset = 1;
    o_offset = 0;
    const std::vector<int> expected_5 {
        12, 13, 0,
        22, 23, 0,
        32, 33, 0
    };
    const auto r5{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_5);
    REQUIRE(r5 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 2;
    i_offset = 2;
    o_offset = 0;
    const std::vector<int> expected_6 {
        13, 14, 0,
        23, 24, 0,
        33, 34, 0
    };
    const auto r6{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_6);
    REQUIRE(r6 == last_i(w_output, o_offset, amount));

    REQUIRE_THROWS(submatrix(amount + 1, w_input, i_offset, w_output, o_offset));
    REQUIRE_THROWS(submatrix(amount, w_input, i_offset + 1, w_output, o_offset));

    std::fill(fout, lout, 0);
    amount = 3;
    i_offset = 0;
    o_offset = 0;
    const std::vector<int> expected_7 {
        11, 12, 13,
        21, 22, 23,
        31, 32, 33
    };
    const auto r7{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_7);
    REQUIRE(r7 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 3;
    i_offset = 1;
    o_offset = 0;
    const std::vector<int> expected_8 {
        12, 13, 14,
        22, 23, 24,
        32, 33, 34
    };
    const auto r8{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_8);
    REQUIRE(r8 == last_i(w_output, o_offset, amount));

    REQUIRE_THROWS(submatrix(amount + 1, w_input, i_offset, w_output, o_offset));
    REQUIRE_THROWS(submatrix(amount, w_input, i_offset + 1, w_output, o_offset));
    REQUIRE_THROWS(submatrix(amount, w_input, i_offset, w_output, o_offset + 1));


    std::fill(fout, lout, 0);
    amount = 2;
    i_offset = 0;
    o_offset = 1;
    const std::vector<int> expected_9 {
        0, 11, 12,
        0, 21, 22,
        0, 31, 32
    };
    const auto r9{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_9);
    REQUIRE(r9 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 2;
    i_offset = 1;
    o_offset = 1;
    const std::vector<int> expected_10 {
        0, 12, 13,
        0, 22, 23,
        0, 32, 33
    };
    const auto r10{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_10);
    REQUIRE(r10 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 2;
    i_offset = 2;
    o_offset = 1;
    const std::vector<int> expected_11 {
        0, 13, 14,
        0, 23, 24,
        0, 33, 34
    };
    const auto r11{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_11);
    REQUIRE(r11 == last_i(w_output, o_offset, amount));

    REQUIRE_THROWS(submatrix(amount + 1, w_input, i_offset, w_output, o_offset));
    REQUIRE_THROWS(submatrix(amount, w_input, i_offset + 1, w_output, o_offset));
    REQUIRE_THROWS(submatrix(amount, w_input, i_offset, w_output, o_offset + 1));

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 0;
    o_offset = 2;
    const std::vector<int> expected_12 {
        0, 0, 11,
        0, 0, 21,
        0, 0, 31
    };
    const auto r12{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_12);
    REQUIRE(r12 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 1;
    o_offset = 2;
    const std::vector<int> expected_13 {
        0, 0, 12,
        0, 0, 22,
        0, 0, 32
    };
    const auto r13{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_13);
    REQUIRE(r13 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 2;
    o_offset = 2;
    const std::vector<int> expected_14 {
        0, 0, 13,
        0, 0, 23,
        0, 0, 33
    };
    const auto r14{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_14);
    REQUIRE(r14 == last_i(w_output, o_offset, amount));

    std::fill(fout, lout, 0);
    amount = 1;
    i_offset = 3;
    o_offset = 2;
    const std::vector<int> expected_15 {
        0, 0, 14,
        0, 0, 24,
        0, 0, 34
    };
    const auto r15{ submatrix(amount, w_input, i_offset, w_output, o_offset) };
    REQUIRE(output == expected_15);
    REQUIRE(r15 == last_i(w_output, o_offset, amount));

    REQUIRE_THROWS(submatrix(amount + 1, w_input, i_offset, w_output, o_offset));
    REQUIRE_THROWS(submatrix(amount, w_input, i_offset + 1, w_output, o_offset));
    REQUIRE_THROWS(submatrix(amount, w_input, i_offset, w_output, o_offset + 1));
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
