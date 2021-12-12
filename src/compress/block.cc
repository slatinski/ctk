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


#include <ostream>

#include "compress/block.h"

namespace ctk { namespace impl {

    auto sizeof_word(encoding_size x) -> unsigned {
        static_assert(field_width_encoding() == 2);

        switch(x) {
            case encoding_size::one_byte: return sizeof(uint8_t);
            case encoding_size::two_bytes: return sizeof(uint16_t);
            case encoding_size::four_bytes: return sizeof(uint32_t);
            case encoding_size::eight_bytes: return sizeof(uint64_t);
            default: throw api::v1::ctk_bug{ "sizeof_word: 2 bits = 4 possible interpretations" };
        }
    }


    auto field_width_master(encoding_size encoding_size) -> bit_count {
        constexpr const unguarded guard;
        return as_bits(byte_count{ cast(sizeof_word(encoding_size), sint{}, guard) }, guard);
    }


    auto decode_method(unsigned pattern) -> encoding_method {
        if (static_cast<unsigned>(encoding_method::chan) < pattern) {
            static_assert(field_width_encoding() == 2);
            throw api::v1::ctk_bug{ "decode_method: 2 bits = 4 possible interpretations" };
        }

        return encoding_method(pattern);
    }


    auto invalid_row_header(encoding_size data_size, encoding_method method, bit_count n, bit_count nexc, size_t word_size) -> std::string {
        std::ostringstream oss;
        oss << "invalid epoch header: encoding data size " << data_size
            << ", method " << method
            << ", n " << n
            << ", nexc " << nexc
            << ", target data size " << word_size << " byte(s)";

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
        // constructed by the implementation
        if(n != nexc) {
            throw api::v1::ctk_bug{ "is_valid_uncompressed: constructed by the implementation" };
        }

        return n == field_width_master(data_size);
    }


    auto is_valid_compressed(bit_count n, bit_count nexc, encoding_size encoding_size) -> bool {
        // not possible with this encoding scheme
        if (n < pattern_size_min() || nexc < pattern_size_min()) {
            return false;
        }

        // not possible with this encoding scheme
        if (nexc < n) {
            return false;
        }

        // does it fit in a word?
        return nexc <= field_width_master(encoding_size);
    }


} /* namespace impl */ } /* namespace ctk */

