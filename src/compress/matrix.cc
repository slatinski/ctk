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

    auto matrix_size(sensor_count electrodes, measurement_count samples) -> sint {
        const dimensions x{ electrodes, samples }; // dimensions guard
        // comparison used in order to prevent the compiler from optimizing out the unused variable x
        if (x.electrodes() != electrodes || x.samples() != samples) {
            return 0;
        }

        const sint channels{ electrodes };
        const sint sample_count{ samples };
        return multiply(channels, sample_count, ok{});
    }

    auto matrix_size(sensor_count electrodes, byte_count samples) -> byte_count {
        const byte_count::value_type i{ samples };
        const measurement_count non_samples{ i };
        const dimensions x{ electrodes, non_samples }; // dimensions guard
        // comparison used in order to prevent the compiler from optimizing out the unused variable x
        if (x.electrodes() != electrodes || x.samples() != non_samples) {
            return byte_count{ 0 };
        }

        const sint channels{ electrodes };
        const auto sample_count{ as_bits(samples, ok{}) };
        return as_bytes(scale(sample_count, channels, ok{}));
    }


    dimensions::dimensions()
    : height{ 0 }
    , length{ 0 } {
    }

    dimensions::dimensions(sensor_count electrodes,  measurement_count samples)
    : height{ electrodes }
    , length{ samples } {
        if (height < 1 || length < 1) {
            std::ostringstream oss;
            oss << "[dimensions, matrix] invalid " << height << " x " << length;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }
    }

    auto dimensions::electrodes() const -> sensor_count {
        return height;
    }

    auto dimensions::samples() const -> measurement_count {
        return length;
    }


    auto natural_row_order(sensor_count n) -> std::vector<int16_t> {
        if (n < 0) {
            std::ostringstream oss;
            oss << "[natural_row_order, matrix] invalid row count " << n;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }

        using Int = sint;
        using UInt = typename std::vector<int16_t>::size_type;

        const Int sn{ n };
        const UInt size{ cast(sn, UInt{}, ok{}) };
        std::vector<int16_t> xs(size);

        std::iota(begin(xs), end(xs), int16_t{ 0 });
        return xs;
    }


    auto is_valid_row_order(std::vector<int16_t> xs) -> bool {
        std::sort(begin(xs), end(xs));
        if (xs != natural_row_order(sensor_count{ vsize(xs) })) {
            ctk_log_error("[is_valid_row_order, matrix] invalid");
            return false;
        }

        return true;
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


