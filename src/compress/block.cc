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

#include "compress/block.h"

#include <sstream>

#include "type_wrapper.h"

namespace ctk { namespace impl {

    auto sizeof_word(encoding_size x) -> unsigned {
        static_assert(field_width_encoding() == 2);

        switch(x) {
            case encoding_size::one_byte: return sizeof(uint8_t);
            case encoding_size::two_bytes: return sizeof(uint16_t);
            case encoding_size::four_bytes: return sizeof(uint32_t);
            case encoding_size::eight_bytes: return sizeof(uint64_t);
            default: {
                const std::string e{ "[sizeof_word, block] 2 bits = 4 possible interpretations" };
                throw api::v1::CtkBug{ e };
            }
        }
    }


    auto field_width_master(encoding_size data_size) -> bit_count {
        using Int = byte_count::value_type;

        constexpr const unguarded guard;
        return as_bits(byte_count{ cast(sizeof_word(data_size), Int{}, guard) }, guard);
    }


    auto decode_method(unsigned pattern) -> encoding_method {
        static_assert(field_width_encoding() == 2);

        if (static_cast<unsigned>(encoding_method::chan) < pattern) {
            const std::string e{ "[decode_method, block] 2 bits = 4 possible interpretations" };
            throw api::v1::CtkBug{ e };
        }

        return encoding_method(pattern);
    }


    auto invalid_row_header(encoding_size data_size, encoding_method method, bit_count n, bit_count nexc, size_t word_size) -> std::string {
        assert(static_cast<unsigned>(data_size) < static_cast<unsigned>(encoding_size::length));
        assert(static_cast<unsigned>(method) < static_cast<unsigned>(encoding_method::length));

        std::ostringstream oss;
        oss << "[invalid row header, block] encoding data size " << data_size
            << ", method " << method
            << ", n " << n
            << ", nexc " << nexc
            << ", target data size " << word_size << " byte" << (word_size == 1 ? "" : "s");
        return oss.str();
    }


    auto restore_n(bit_count n, bit_count word_size) -> bit_count {
        return n == 0 ? word_size : n;
    }


    auto uncompressed_header_width() -> bit_count {
        constexpr const auto scheme{ field_width_encoding() + field_width_encoding() };
        static_assert(scheme <= one_byte());
        return one_byte();
    }


    auto is_valid_uncompressed(bit_count n, bit_count nexc, encoding_size data_size) -> bool {
        if(n != nexc) {
            // n and nexc are constructed by the implementation
            std::ostringstream oss;
            oss << "[is_valid_uncompressed, block] n != nexc, " << n << " != " << nexc;
            const auto e{ oss.str() };
            throw api::v1::CtkBug{ e };
        }

        const auto sizeof_t{ field_width_master(data_size) };
        if (n != sizeof_t) {
            return false;
        }

        return true;
    }


    auto is_valid_compressed(bit_count n, bit_count nexc, encoding_size data_size) -> bool {
        constexpr const auto lowest{ pattern_size_min() };
        // not possible with this encoding scheme
        if (n < lowest || nexc < lowest) {
            return false;
        }

        // not possible with this encoding scheme
        if (nexc < n) {
            return false;
        }

        const auto sizeof_t{ field_width_master(data_size) };
        if (sizeof_t < nexc) {
            return false;
        }

        return true;
    }


} /* namespace impl */ } /* namespace ctk */

