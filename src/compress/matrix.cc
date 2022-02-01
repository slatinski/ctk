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

#include "compress/matrix.h"

namespace ctk { namespace impl {

    auto matrix_size(sensor_count height, measurement_count length) -> sint {
        //assert(0 <= height);
        //assert(0 <= length);

        const sint channels{ height };
        const sint samples{ length };
        return multiply(channels, samples, ok{});
    }

    auto matrix_size(sensor_count height, byte_count length) -> byte_count {
        assert(0 <= height);
        assert(0 <= length);

        const sint channels{ height };
        const auto samples{ as_bits(length, ok{}) };
        return as_bytes(scale(samples, channels, ok{}));
    }


    auto natural_row_order(sensor_count n) -> std::vector<int16_t> {
        assert(0 < n);

        using Int = sint;
        using UInt = typename std::vector<int16_t>::size_type;

        const Int sn{ n };
        const UInt size{ cast(sn, UInt{}, ok{}) };
        std::vector<int16_t> v(size);

        std::iota(begin(v), end(v), int16_t{ 0 });
        return v;
    }


    // TODO
    auto is_valid_row_order(std::vector<int16_t> v) -> bool {
        if (v.empty() || !in_signed_range(v.size(), sint{}, ok{})) {
            return false;
        }

        std::sort(begin(v), end(v));
        return v == natural_row_order(sensor_count{ vsize(v) });
    }



    // reflib
    auto min_data_size(bit_count nexc, bit_count master, reflib) -> encoding_size {
        constexpr const auto two_bytes{ scale(one_byte(), 2, unguarded{}) };

        if (nexc <= two_bytes && master <= two_bytes) {
            return encoding_size::two_bytes;
        }

        assert(nexc <= scale(one_byte(), 4, unguarded{}) && master <= scale(one_byte(), 4, unguarded{}));
        return encoding_size::four_bytes;
    }


    // extended
    auto min_data_size(bit_count nexc, bit_count master, extended) -> encoding_size {
        constexpr const auto two_bytes{ scale(one_byte(), 2, unguarded{}) };
        constexpr const auto four_bytes{ scale(one_byte(), 4, unguarded{}) };

        if (nexc <= one_byte() && master <= one_byte()) {
            return encoding_size::one_byte;
        }

        if (nexc <= two_bytes && master <= two_bytes) {
            return encoding_size::two_bytes;
        }

        if (nexc <= four_bytes && master <= four_bytes) {
            return encoding_size::four_bytes;
        }

        assert(nexc <= scale(one_byte(), 8, unguarded{}) && master <= scale(one_byte(), 8, unguarded{}));
        return encoding_size::eight_bytes;
    }


} /* namespace impl */ } /* namespace ctk */


