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
#include "file/io.h"


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
        template<typename T, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto encode_byte(T x, L leb) -> std::tuple<uint8_t, T, bool> {
            static_assert(std::is_integral<T>::value);

            T byte{ seven_bits(x) };
            x >>= 7;

            if (leb.is_last(x, byte)) {
                return { static_cast<uint8_t>(byte & 0xff), x, false }; // no more
            }

            byte |= static_cast<T>(continuation_bit);
            return { static_cast<uint8_t>(byte & 0xff), x, true }; // more
        }


        // based on the pseudo code presented in
        // DWARF Debugging Information Format, Version 4
        // Appendix C -- Variable Length Data: Encoding/Decoding (informative)
        // Figure 46. Algorithm to decode an unsigned LEB128 number
        // Figure 47. Algorithm to decode a signed LEB128 number
        template<typename T, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto decode_byte(uint8_t x, T acc, unsigned shift, L leb) -> std::tuple<T, unsigned, bool> {
            static_assert(std::is_integral<T>::value);

            constexpr unsigned size{ sizeof(T) * 8 };
            if (size <= shift) {
                throw api::v1::ctk_data{ "leb128::decode_byte: invalid encoding" };
            }

            const T byte{ static_cast<T>(x) };

            acc |= static_cast<T>(seven_bits(byte) << shift);
            shift += 7;

            if (continuation_bit_set(byte)) {
                return { acc, shift, true }; // more
            }

            if (leb.extend_sign(shift, size, byte)) {
                acc |= static_cast<T>(-(1 << shift));
            }

            return { acc, shift, false }; // no more
        }


        template<typename T, typename IByte, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto encode(T x, IByte first, IByte last, L leb) -> IByte {
            using B = typename std::iterator_traits<IByte>::value_type;
            static_assert(sizeof(B) == 1);
            static_assert(std::is_unsigned<B>::value);
            static_assert(std::is_integral<T>::value);

            bool more{ first != last };

            for (/* empty */; first != last; ++first) {
                if (!more) {
                    break;
                }

                std::tie(*first, x, more) = encode_byte(x, leb);
            }

            if (more) {
                throw api::v1::ctk_bug{ "leb128::encode: insufficient output buffer" };
            }

            return first;
        }



        template<typename T, typename IByte, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto decode(IByte first, IByte last, T x, L leb) -> std::pair<T, IByte> {
            using B = typename std::iterator_traits<IByte>::value_type;
            static_assert(sizeof(B) == 1);
            static_assert(std::is_unsigned<B>::value);
            static_assert(std::is_integral<T>::value);

            x = 0;
            unsigned shift{ 0 };
            bool more{ first != last };

            for (/* empty */; first != last; ++first) {
                if (!more) {
                    break;
                }

                std::tie(x, shift, more) = decode_byte(*first, x, shift, leb);
            }

            if (more) {
                throw api::v1::ctk_data{ "leb128::decode: invalid encoding" };
            }

            return { x, first };
        }


        template<typename T>
        auto max_bytes(T/* type tag */) -> unsigned {
            return static_cast<unsigned>(std::ceil((sizeof(T) * 8.0) / 7.0));
        }


        template<typename T, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto to_file(FILE* f, T x, L leb) -> void {
            bool more{ true };
            uint8_t byte{ 0 };
            const unsigned max_size{ max_bytes(T{}) };

            for (unsigned i{ 0 }; i < max_size; ++i) {
                std::tie(byte, x, more) = encode_byte(x, leb);
                write(f, byte);
                if (!more) {
                    break;
                }
            }

            assert(!more);
        }


        template<typename T, typename L>
        // requirements:
        // - L has an interface identical to sleb or uleb
        auto from_file(FILE* f, T x, L leb) -> T {
            x = 0;
            unsigned shift{ 0 };
            bool more{ true };
            const unsigned max_size{ max_bytes(T{}) };

            for (unsigned i{ 0 }; i < max_size; ++i) {
                std::tie(x, shift, more) = decode_byte(read(f, uint8_t{}), x, shift, leb);
                if (!more) {
                    break;
                }
            }

            if (more) {
                throw api::v1::ctk_data{ "leb128::from_file: invalid encoding" };
            }

            return x;
        }


        // helper for calling encode() with the correct sleb/uleb argument
        template<bool/* is signed? */>
        struct encode_sequence;

        template<>
        struct encode_sequence<true/* signed */>
        {
            template<typename T, typename IByte>
            static
            auto write(T x, IByte first, IByte last) -> IByte {
                static_assert(std::is_signed<T>::value);
                return encode(x, first, last, sleb{});
            }
        };

        template<>
        struct encode_sequence<false/* unsigned */>
        {
            template<typename T, typename IByte>
            static
            auto write(T x, IByte first, IByte last) -> IByte {
                static_assert(std::is_unsigned<T>::value);
                return encode(x, first, last, uleb{});
            }
        };


        // helper for calling decode() with the correct sleb/uleb argument
        template<bool/* is signed? */>
        struct decode_sequence;

        template<>
        struct decode_sequence<true/* signed */>
        {
            template<typename T, typename IByte>
            static
            auto read(IByte first, IByte last, T type_tag) -> std::pair<T, IByte> {
                static_assert(std::is_signed<T>::value);
                return decode(first, last, type_tag, sleb{});
            }
        };

        template<>
        struct decode_sequence<false/* unsigned */>
        {
            template<typename T, typename IByte>
            static
            auto read(IByte first, IByte last, T type_tag) -> std::pair<T, IByte> {
                static_assert(std::is_unsigned<T>::value);
                return decode(first, last, type_tag, uleb{});
            }
        };


        // helper for calling to_file() with the correct sleb/uleb argument
        template<bool/* is signed? */>
        struct write_to_file;

        template<>
        struct write_to_file<true/* signed */>
        {
            template<typename T>
            static
            auto write(FILE* f, T x) -> void {
                static_assert(std::is_signed<T>::value);
                to_file(f, x, sleb{});
            }
        };

        template<>
        struct write_to_file<false/* unsigned */>
        {
            template<typename T>
            static
            auto write(FILE* f, T x) -> void {
                static_assert(std::is_unsigned<T>::value);
                to_file(f, x, uleb{});
            }
        };


        // helper for calling from_file() with the correct sleb/uleb argument
        template<bool/* is signed? */>
        struct read_from_file;

        template<>
        struct read_from_file<true/* signed */>
        {
            template<typename T>
            static
            auto read(FILE* f, T type_tag) -> T {
                static_assert(std::is_signed<T>::value);
                return from_file(f, type_tag, sleb{});
            }
        };

        template<>
        struct read_from_file<false/* unsigned */>
        {
            template<typename T>
            static
            auto read(FILE* f, T type_tag) -> T {
                static_assert(std::is_unsigned<T>::value);
                return from_file(f, type_tag, uleb{});
            }
        };

    } // namespace leb128



    template<typename T, typename IByte>
    auto encode_leb128(T x, IByte first, IByte last) -> IByte {
        using op = leb128::encode_sequence<std::is_signed<T>::value>;
        return op::write(x, first, last);
    }


    template<typename T, typename IByte>
    auto decode_leb128(IByte first, IByte last, T type_tag) -> std::pair<T, IByte> {
        using op = leb128::decode_sequence<std::is_signed<T>::value>;
        return op::read(first, last, type_tag);
    }


    template<typename T>
    auto encode_leb128_v(T x) -> std::vector<uint8_t> {
        static_assert(sizeof(T) <= 8);

        const unsigned max_size{ leb128::max_bytes(x) };
        std::vector<uint8_t> xs(max_size);
        const auto first{ begin(xs) };
        const auto last{ end(xs) };
        const auto next{ encode_leb128(x, first, last) };
        xs.resize(static_cast<size_t>(std::distance(first, next)));
        return xs;
    }

    template<typename T>
    auto decode_leb128_v(const std::vector<uint8_t>& xs, T type_tag) -> T {
        const auto first{ begin(xs) };
        const auto last{ end(xs) };
        const auto [x, next]{ decode_leb128(first, last, type_tag) };
        if (next != last) {
            throw api::v1::ctk_data{ "decode_leb128_v: invalid encoding" };
        }
        return x;
    }


    template<typename T>
    auto write_leb128(FILE* f, T x) -> void {
        using op = leb128::write_to_file<std::is_signed<T>::value>;
        op::write(f, x);
    }

    template<typename T>
    auto read_leb128(FILE* f, T type_tag) -> T {
        using op = leb128::read_from_file<std::is_signed<T>::value>;
        return op::read(f, type_tag);
    }

} /* namespace impl */ } /* namespace ctk */

