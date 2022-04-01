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

#include "type_wrapper.h"

#include <sstream>

#include "exception.h"

namespace ctk { namespace impl {

    auto operator<<(std::ostream& os, const encoding_size& x) -> std::ostream& {
        switch(x) {
            case encoding_size::one_byte: os << "1 byte"; break;
            case encoding_size::two_bytes: os << "2 bytes"; break;
            case encoding_size::four_bytes: os << "4 bytes"; break;
            case encoding_size::eight_bytes: os << "8 bytes"; break;
            default: {
                std::ostringstream oss;
                oss << "[operator<<(encoding_size), type_wrapper] invalid size " << static_cast<int>(x);
                const auto e{ oss.str() };
                throw ctk::api::v1::CtkBug{ e };
            }
        }
        return os;
    }

    auto operator<<(std::ostream& os, const encoding_method& x) -> std::ostream& {
        switch(x) {
            case encoding_method::copy: os << "copy"; break;
            case encoding_method::time: os << "time"; break;
            case encoding_method::time2: os << "time2"; break;
            case encoding_method::chan: os << "chan"; break;
            default: {
                std::ostringstream oss;
                oss << "[operator<<(encoding_method), type_wrapper] invalid size " << static_cast<int>(x);
                const auto e{ oss.str() };
                throw ctk::api::v1::CtkBug{ e };
            }
        }
        return os;
    }

} /* namespace impl */ } /* namespace ctk */

