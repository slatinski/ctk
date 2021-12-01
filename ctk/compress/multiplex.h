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

#include <vector>
#include <algorithm>
#include <cstring>

#include "arithmetic.h"

namespace ctk { namespace impl {


    struct client2storage
    {
        template<typename IConst, typename I>
        constexpr
        auto operator()(IConst client, I storage) const -> void {
            using T = typename std::iterator_traits<I>::value_type;
            using U = typename std::iterator_traits<IConst>::value_type;
            static_assert(sizeof(T) == sizeof(U));

            memcpy(&(*storage), &(*client), sizeof(T));
        }
    };


    struct storage2client
    {
        template<typename I, typename IConst>
        constexpr
        auto operator()(I client, IConst storage) const -> void {
            using T = typename std::iterator_traits<I>::value_type;
            using U = typename std::iterator_traits<IConst>::value_type;
            static_assert(sizeof(T) == sizeof(U));

            memcpy(&(*client), &(*storage), sizeof(T));
        }
    };

    // client: column-major first order
    // storage: row-major first order
    template<typename I1, typename I2, typename AssignOp>
    // requirements
    //     - I1 is random access iterator and ValueType(I1) is convertible to ValueType(I2)
    //     - I2 is random access iterator and ValueType(I1) is convertible to ValueType(I2)
    //     - AssignOp has an interface identical to the interface of storage2client or client2storage
    auto transpose(I1 client, I2 storage, const std::vector<int16_t>& row_order, measurement_count length, AssignOp assign) -> void {
        const sint i_heigth{ cast(row_order.size(), sint{}, guarded{}) };
        const sint i_length{ length };
        sint column{ 0 };

        for (int16_t row : row_order) {
            sint x{ 0 };
            sint y{ 0 };

            for (/* empty */; x < i_length; ++x, y += i_heigth) {
                assign(client + row + y, storage + column + x);
            }

            column += i_length;
        }
    }



    // interface 

    struct column_major2row_major
    {
        // converts a column-major order matrix into a row-major order matrix, according to the specified row order
        template<typename IConst, typename I>
        auto from_client(IConst client, I storage, const std::vector<int16_t>& row_order, measurement_count length) const -> void {
            transpose(client, storage, row_order, length, client2storage{});
        }

        // converts a row-major order matrix into a column-major order matrix, according to the specified row order
        template<typename IConst, typename I>
        auto to_client(IConst storage, I client, const std::vector<int16_t>& row_order, measurement_count length) const -> void {
            transpose(client, storage, row_order, length, storage2client{});
        }
    };


    // these functions could be implemented as
    // from_client() { copy(client, storage) }
    // to_client() { copy(storage, client) }
    // if natural row order could be assumed: 0, 1, 2, 3, ...
    struct row_major2row_major
    {
        // copies a row-major order matrix into another row-major order matrix, according to the specified row order
        template<typename IConst, typename I>
        auto from_client(IConst client, I storage, const std::vector<int16_t>& row_order, measurement_count length) const -> void {
            using T = typename std::iterator_traits<I>::value_type;
            using U = typename std::iterator_traits<IConst>::value_type;
            static_assert(sizeof(T) == sizeof(U));

            for (int16_t row : row_order) {
                const auto row_begin{ row * static_cast<sint>(length) };
                const auto row_end{ row_begin + static_cast<sint>(length) };

                storage = std::copy(client + row_begin, client + row_end, storage);
            }
        }

        // copies a row-major order matrix into another row-major order matrix, according to the specified row order
        template<typename IConst, typename I>
        auto to_client(IConst storage, I client, const std::vector<int16_t>& row_order, measurement_count length) const -> void {
            using T = typename std::iterator_traits<I>::value_type;
            using U = typename std::iterator_traits<IConst>::value_type;
            static_assert(sizeof(T) == sizeof(U));

            sint row_begin{ 0 };
            const sint i_length{ length };

            for (int16_t row : row_order) {
                static_assert(sizeof(int16_t) <= sizeof(sint));
                const sint dest{ multiply(static_cast<sint>(row), i_length, guarded{}) };
                const sint row_end{ plus(row_begin, i_length, guarded{}) };
                //dest = row * i_length;
                //row_end = row_begin + i_length;

                std::copy(storage + row_begin, storage + row_end, client + dest);

                row_begin = row_end;
            }
        }
    };


} /* namespace impl */ } /* namespace ctk */

