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

#include "compress/multiplex.h"

namespace ctk { namespace impl { namespace test {


template<typename Multiplex>
auto mux_demux(const std::vector<int>& client, const std::vector<int>& storage, std::vector<int>& buffer, const std::vector<int16_t>& row_order, sint row_length, Multiplex multiplex) -> void {
    multiplex.from_client(begin(client), begin(buffer), row_order, measurement_count{ row_length });
    REQUIRE(buffer == storage);

    multiplex.to_client(begin(storage), begin(buffer), row_order, measurement_count{ row_length });
    REQUIRE(buffer == client);
}


TEST_CASE("cnt matrix multiplex/demultiplex", "[correct]") {
    const sint measurements{ 4 };
    const std::vector<int> column_major = {
        11, 21, 31,
        12, 22, 32,
        13, 23, 33,
        14, 24, 34 };
    const std::vector<int> row_major = {
        11, 12, 13, 14,
        21, 22, 23, 24,
        31, 32, 33, 34 };
    std::vector<int> buffer(column_major.size());
    constexpr const column_major2row_major transpose;
    constexpr const row_major2row_major copy;

    std::cout << "demultiplex/multiplex: channel order 0 1 2\n";
    // order: 0 1 2
    // input  [11 21 31] [12 22 32] [13 23 33] [14 24 34]
    // output [11 12 13 14] [21 22 23 24] [31 32 33 34]
    const std::vector<int16_t> row_order_012 = { 0, 1, 2 };
    mux_demux(column_major, row_major, buffer, row_order_012, measurements, transpose);

    // order: 0 1 2
    // input  [11 12 13 14] [21 22 23 24] [31 32 33 34]
    // output [11 12 13 14] [21 22 23 24] [31 32 33 34]
    mux_demux(row_major, row_major, buffer, row_order_012, measurements, copy);

    std::cout << "demultiplex/multiplex: channel order 0 2 1\n";
    // order: 0 2 1
    // input  [11 21 31] [12 22 32] [13 23 33] [14 24 34]
    // output [11 12 13 14] [31 32 33 34] [21 22 23 24]
    const std::vector<int16_t> row_order_021 = { 0, 2, 1 };
    const std::vector<int> row_major_021 = {
        11, 12, 13, 14,
        31, 32, 33, 34,
        21, 22, 23, 24 };
    mux_demux(column_major, row_major_021, buffer, row_order_021, measurements, transpose);

    // order: 0 2 1
    // input  [11 12 13 14] [21 22 23 24] [31 32 33 34]
    // output [11 12 13 14] [31 32 33 34] [21 22 23 24]
    mux_demux(row_major, row_major_021, buffer, row_order_021, measurements, copy);

    std::cout << "demultiplex/multiplex: channel order 1 0 2\n";
    // order: 1 0 2
    // input  [11 21 31] [12 22 32] [13 23 33] [14 24 34]
    // output [21 22 23 24] [11 12 13 14] [31 32 33 34]
    const std::vector<int16_t> row_order_102 = { 1, 0, 2 };
    const std::vector<int> row_major_102 = {
        21, 22, 23, 24,
        11, 12, 13, 14,
        31, 32, 33, 34 };
    mux_demux(column_major, row_major_102, buffer, row_order_102, measurements, transpose);

    // order: 1 0 2
    // input  [11 12 13 14] [21 22 23 24] [31 32 33 34]
    // output [21 22 23 24] [11 12 13 14] [31 32 33 34]
    mux_demux(row_major, row_major_102, buffer, row_order_102, measurements, copy);

    std::cout << "demultiplex/multiplex: channel order 1 2 0\n";
    // order: 1 2 0
    // input  [11 21 31] [12 22 32] [13 23 33] [14 24 34]
    // output [21 22 23 24] [31 32 33 34] [11 12 13 14]
    const std::vector<int16_t> row_order_120 = { 1, 2, 0 };
    const std::vector<int> row_major_120 = {
        21, 22, 23, 24,
        31, 32, 33, 34,
        11, 12, 13, 14 };
    mux_demux(column_major, row_major_120, buffer, row_order_120, measurements, transpose);

    // order: 1 2 0
    // input  [11 12 13 14] [21 22 23 24] [31 32 33 34]
    // output [21 22 23 24] [31 32 33 34] [11 12 13 14]
    mux_demux(row_major, row_major_120, buffer, row_order_120, measurements, copy);

    std::cout << "demultiplex/multiplex: channel order 2 0 1\n";
    // order: 2 0 1
    // input  [11 21 31] [12 22 32] [13 23 33] [14 24 34]
    // output [31 32 33 34] [11 12 13 14] [21 22 23 24]
    const std::vector<int16_t> row_order_201 = { 2, 0, 1 };
    const std::vector<int> row_major_201 = {
        31, 32, 33, 34,
        11, 12, 13, 14,
        21, 22, 23, 24 };
    mux_demux(column_major, row_major_201, buffer, row_order_201, measurements, transpose);

    // order: 2 0 1
    // input  [11 12 13 14] [21 22 23 24] [31 32 33 34]
    // output [31 32 33 34] [11 12 13 14] [21 22 23 24]
    mux_demux(row_major, row_major_201, buffer, row_order_201, measurements, copy);

    std::cout << "demultiplex/multiplex: channel order 2 1 0\n";
    // order: 2 1 0
    // input  [11 21 31] [12 22 32] [13 23 33] [14 24 34]
    // output [31 32 33 34] [21 22 23 24] [11 12 13 14]
    const std::vector<int16_t> row_order_210 = { 2, 1, 0 };
    const std::vector<int> row_major_210 = {
        31, 32, 33, 34,
        21, 22, 23, 24,
        11, 12, 13, 14 };
    mux_demux(column_major, row_major_210, buffer, row_order_210, measurements, transpose);

    // order: 2 1 0
    // input  [11 12 13 14] [21 22 23 24] [31 32 33 34]
    // output [31 32 33 34] [21 22 23 24] [11 12 13 14]
    mux_demux(row_major, row_major_210, buffer, row_order_210, measurements, copy);
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */

