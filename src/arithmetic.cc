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

#include "arithmetic.h"

namespace ctk { namespace impl {

    auto as_sizet(sint x) -> size_t {
        return cast(x, size_t{}, ok{});
    }

    auto as_sizet(sensor_count x) -> size_t {
        const sensor_count::value_type ix{ x };
        return as_sizet(ix);
    }

    auto as_sizet(measurement_count x) -> size_t {
        const measurement_count::value_type ix{ x };
        return as_sizet(ix);
    }

    auto as_sizet(epoch_count x) -> size_t {
        const epoch_count::value_type ix{ x };
        return as_sizet(ix);
    }

    auto as_sizet(bit_count x) -> size_t {
        const bit_count::value_type ix{ x };
        return as_sizet(ix);
    }

    auto as_sizet(byte_count x) -> size_t {
        const byte_count::value_type ix{ x };
        return as_sizet(ix);
    }

    auto as_sizet_unchecked(sint x) -> size_t {
        assert(0 <= x);
        return static_cast<size_t>(x);
    }

    auto as_sizet_unchecked(sensor_count x) -> size_t {
        const sensor_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    auto as_sizet_unchecked(measurement_count x) -> size_t {
        const measurement_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    auto as_sizet_unchecked(epoch_count x) -> size_t {
        const epoch_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    auto as_sizet_unchecked(bit_count x) -> size_t {
        const bit_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    auto as_sizet_unchecked(byte_count x) -> size_t {
        const byte_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

} /* namespace impl */ } /* namespace ctk */

