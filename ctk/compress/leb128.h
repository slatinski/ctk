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

#include <limits>
#include <vector>

#include "exception.h"


namespace ctk { namespace impl {

    namespace leb128 {

        constexpr const int continuation_bit{ 0x80 };
        constexpr const int signum_bit{ 0x40 };


        template<typename T>
        constexpr
        auto continuation_bit_set(T x) -> bool {
            return (x & continuation_bit) == continuation_bit;
        }

        template<typename T>
        constexpr
        auto signum_bit_set(T x) -> bool {
            return (x & signum_bit) == signum_bit;
        }

        template<typename T>
        constexpr
        auto seven_bits(T x) -> T {
            return static_cast<T>(x & 0x7f);
        }


        struct uleb
        {
            // encode: is this the last byte to be emitted?
            // true if the input x consists of leading zeroes only.
            template<typename T>
            constexpr
            auto is_last(T x, T /* byte */) const -> bool {
                return x == 0;
            }

            // used by decode() to decide if the result shall be sign extended
            template<typename T>
            constexpr
            auto extend_sign(size_t /* shift */, size_t /* size */, T /* byte */) const -> bool {
                return false;
            }
        };

        struct sleb
        {
            // encode: is this the last byte to be emitted?
            // true if the input x consists either of leading zeroes (positive input) or of leading ones (negative input) only.
            template<typename T>
            constexpr
            auto is_last(T x, T byte) const -> bool {
                return (x == 0 && !signum_bit_set(byte)) || (x == -1 && signum_bit_set(byte));
            }

            // used by decode() to decide if the result shall be sign extended
            template<typename T>
            constexpr
            auto extend_sign(size_t shift, size_t size, T byte) const -> bool {
                return (shift < size) && signum_bit_set(byte);
            }
        };


        // based on the pseudo code presented in
        // DWARF Debugging Information Format, Version 4
        // Appendix C -- Variable Length Data: Encoding/Decoding (informative)
        // Figure 44. Algorithm to encode an unsigned integer
        // Figure 45. Algorithm to encode a signed integer
        template<typename T, typename IByte, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto encode(T x, IByte first, IByte last, L leb) -> IByte {
            using B = typename std::iterator_traits<IByte>::value_type;
            static_assert(sizeof(B) == 1);
            static_assert(std::is_unsigned<B>::value);
            static_assert(std::is_integral<T>::value);

            bool more{ true };
            T byte;

             while (more && first != last) {
                byte = seven_bits(x);
                x >>= 7;

                if (leb.is_last(x, byte)) {
                    more = false;
                }
                else {
                    byte |= static_cast<T>(continuation_bit);
                }

                *first = static_cast<B>(byte);
                ++first;
            }

            if (more) {
                throw api::v1::ctk_bug{ "leb128::encode: insufficient output buffer" };
            }

            return first;
        }


        template<typename T>
        struct lebstate
        {
            T x;
            size_t shift;

            lebstate()
            : x{ 0 }
            , shift{ 0 } {
            }
        };

        // based on the pseudo code presented in
        // DWARF Debugging Information Format, Version 4
        // Appendix C -- Variable Length Data: Encoding/Decoding (informative)
        // Figure 46. Algorithm to decode an unsigned LEB128 number
        // Figure 47. Algorithm to decode a signed LEB128 number
        template<typename T, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto decode_byte(uint8_t input, lebstate<T>& state, L leb) -> bool {
            static_assert(std::is_integral<T>::value);

            constexpr const size_t size{ sizeof(T) * 8 };
            if (size <= state.shift) {
                throw api::v1::ctk_data{ "leb128::decode_byte: invalid encoding" };
            }

            const T byte{ static_cast<T>(input) };

            state.x |= static_cast<T>(seven_bits(byte) << state.shift);
            state.shift += 7;

            if (continuation_bit_set(byte)) {
                return true; // more
            }

            if (leb.extend_sign(state.shift, size, byte)) {
                state.x |= static_cast<T>(-(1 << state.shift));
            }

            return false; // no more
        }


        // based on the pseudo code presented in
        // DWARF Debugging Information Format, Version 4
        // Appendix C -- Variable Length Data: Encoding/Decoding (informative)
        // Figure 46. Algorithm to decode an unsigned LEB128 number
        // Figure 47. Algorithm to decode a signed LEB128 number
        template<typename T, typename IByte, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto decode(IByte first, IByte last, T/*type tag*/, L leb) -> std::pair<T, IByte> {
            using B = typename std::iterator_traits<IByte>::value_type;
            static_assert(sizeof(B) == 1);
            static_assert(std::is_unsigned<B>::value);
            static_assert(std::is_integral<T>::value);
            static_assert(sizeof(T) <= 8);

            lebstate<T> state;
            bool more{ first != last };

            while(more && first != last) {
                more = decode_byte(*first, state, leb);
                ++first;
            }

            if (more) {
                throw api::v1::ctk_data{ "leb128::decode: invalid encoding" };
            }

            return { state.x, first };
        }


    } // namespace leb128


    template<typename T, typename IByte>
    auto encode_uleb128(T x, IByte first, IByte last) -> IByte {
        static_assert(std::is_unsigned<T>::value);

        return leb128::encode(x, first, last, leb128::uleb{});
    }

    template<typename T, typename IByte>
    auto encode_sleb128(T x, IByte first, IByte last) -> IByte {
        static_assert(std::is_signed<T>::value);

        return leb128::encode(x, first, last, leb128::sleb{});
    }

    template<typename T, typename IByte>
    auto decode_uleb128(IByte first, IByte last, T type_tag) -> std::pair<T, IByte> {
        static_assert(std::is_unsigned<T>::value);

        return leb128::decode(first, last, type_tag, leb128::uleb{});
    }

    template<typename T, typename IByte>
    auto decode_sleb128(IByte first, IByte last, T type_tag) -> std::pair<T, IByte> {
        static_assert(std::is_signed<T>::value);

        return leb128::decode(first, last, type_tag, leb128::sleb{});
    }


    template<typename T>
    auto encode_sleb128_v(T x) -> std::vector<uint8_t> {
        static_assert(sizeof(T) <= 8);

        std::vector<uint8_t> xs(sizeof(T) + 2 /* sizeof(T) <= 8 */);
        const auto first{ begin(xs) };
        const auto last{ end(xs) };
        const auto next{ encode_sleb128(x, first, last) };
        xs.resize(static_cast<size_t>(std::distance(first, next)));
        return xs;
    }

    template<typename T>
    auto decode_sleb128_v(const std::vector<uint8_t>& xs, T type_tag) -> T {
        const auto first{ begin(xs) };
        const auto last{ end(xs) };
        const auto [x, next]{ decode_sleb128(first, last, type_tag) };
        if (next != last) {
            throw api::v1::ctk_data{ "decode_sleb128_v: invalid encoding" };
        }
        return x;
    }


    template<typename T>
    auto encode_uleb128_v(T x) -> std::vector<uint8_t> {
        static_assert(sizeof(T) <= 8);

        std::vector<uint8_t> xs(sizeof(T) + 2 /* sizeof(T) <= 8 */);
        const auto first{ begin(xs) };
        const auto last{ end(xs) };
        const auto next{ encode_uleb128(x, first, last) };
        xs.resize(static_cast<size_t>(std::distance(first, next)));
        return xs;
    }

    template<typename T>
    auto decode_uleb128_v(const std::vector<uint8_t>& xs, T type_tag) -> T {
        const auto first{ begin(xs) };
        const auto last{ end(xs) };
        const auto [x, next]{ decode_uleb128(first, last, type_tag) };
        if (next != last) {
            throw api::v1::ctk_data{ "decode_uleb128_v: invalid encoding" };
        }
        return x;
    }


} /* namespace impl */ } /* namespace ctk */

