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

#include <cassert>
#include <vector>
#include <variant>
#include <algorithm>
#include <array>

#include "arithmetic.h"


/*
    Represents a sequence of bytes as a sequence of bits.
    The contents of every byte is interpreted from the most significant down to the least significant bit.
    The information is consumed in bit groups. A bit group may span over several adjacent bytes.

    Example: if 10 bits have to be read from or written to the byte stream, then the affected bits will be:

    a) Placement of the 10 bits in the byte stream:
    | byte N        | byte N + 1    | byte N + 2    |   input byte stream (decoder case) or output byte stream (encoder case)
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|   bit indices (as seen from C/C++)
    |9|8|7|6|5|4|3|2|1|0|                           |   10 bits extracted from/written to the byte stream

    b) Placement of the 10 bits in the input word (encoder case) or output word (decoder case):
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|   bit indices (as seen from C/C++)
    |           |9|8|7|6|5|4|3|2|1|0|   two byte word with the bits from the stream placed at their actual position in the word

    Processing 3 more bits would look like this:

    a) Placement of the 3 bits in the byte stream:
    | byte N        | byte N + 1    | byte N + 2    |   input byte stream (decoder case) or output byte stream (encoder case)
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|   bit indices (as seen from C/C++)
    |                   |2|1|0|                     |   3 more bits extracted from/written to the byte stream

    b) Placement of the 3 bits in the input word (encoder case) or output word (decoder case):
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|   bit indices (as seen from C/C++)
    |                         |2|1|0|   two byte word with the bits from the stream placed at their actual position in the word

*/


namespace ctk { namespace impl {

    template<typename T>
    struct pos_size
    {
        static constexpr const size_t value = sizeof(T) * bits_per_byte + 1;
    };

    // creates a bit mask for selecting the n least significant bits.
    template<typename T>
    static
    auto init_posmask() -> std::array<T, pos_size<T>::value> {
        static_assert(std::is_unsigned<T>::value, "signed integer overflow");

        auto result{ std::array<T, pos_size<T>::value>{} };
        const auto first{ begin(result) };
        const auto last{ first + pos_size<T>::value - 2 }; // the last 2 elements cannot be generated using the main algorithm

        // fills the vector with the result of the function f(n) = pow(2, n) - 1, n >= 0
        // 00000000
        // 00000001
        // 00000011
        // 00000111
        // ...
        T x{ 1 };
        std::generate(first, last, [&x]() { const auto result{ T(x - 1) }; x = static_cast<T>(x + x); return result; });

        using ST = typename std::make_signed<T>::type;

        result[pos_size<T>::value - 2] = (static_cast<T>(std::numeric_limits<ST>::max()));
        result[pos_size<T>::value - 1] = static_cast<T>(~0);

        return result;
    }

    inline const std::array<uint8_t,  pos_size<uint8_t>::value>  posmask8{  init_posmask<uint8_t>()  };
    inline const std::array<uint16_t, pos_size<uint16_t>::value> posmask16{ init_posmask<uint16_t>() };
    inline const std::array<uint32_t, pos_size<uint32_t>::value> posmask32{ init_posmask<uint32_t>() };
    inline const std::array<uint64_t, pos_size<uint64_t>::value> posmask64{ init_posmask<uint64_t>() };


    template<size_t>
    struct select_posmask;

    template<>
    struct select_posmask<sizeof(uint8_t)>
    {
        static constexpr
        auto table() -> const std::array<uint8_t, pos_size<uint8_t>::value>& {
            return posmask8;
        }
    };

    template<>
    struct select_posmask<sizeof(uint16_t)>
    {
        static constexpr
        auto table() -> const std::array<uint16_t, pos_size<uint16_t>::value>& {
            return posmask16;
        }
    };

    template<>
    struct select_posmask<sizeof(uint32_t)>
    {
        static constexpr
        auto table() -> const std::array<uint32_t, pos_size<uint32_t>::value>& {
            return posmask32;
        }
    };

    template<>
    struct select_posmask<sizeof(uint64_t)>
    {
        static constexpr
        auto table() -> const std::array<uint64_t, pos_size<uint64_t>::value>& {
            return posmask64;
        }
    };


    template<typename T>
    // requirements
    //     - T is integral type with two-complement implementation
    constexpr
    auto mask_msb(T bit_pattern, bit_count n) -> T {
        using mask_type = select_posmask<sizeof(T)>;

        constexpr const auto& posmask{ mask_type::table() };
        const auto i{ as_sizet_unchecked(n) };

        return static_cast<T>(bit_pattern & posmask[i]);
    }


    // represents a sequence of bytes as a sequence of bits.
    struct bit_stream
    {
        using A = uint64_t;     // accumulator type

        A accumulator;
        bit_count total;        // amount of bits available in the whole byte sequence
        bit_count available;    // amount of bits available in the accumulator

        template<typename I>
        bit_stream(I first, I last)
            : accumulator{ 0 }
            , total{ size_in_bits(first, last, ok{}) }
            , available{ 0 } {
        }
    };

    template<typename IByteConst>
    struct bit_stream_reader
    {
        IByteConst current; // input stream
        bit_stream common;

        bit_stream_reader(IByteConst first, IByteConst last)
            : current{ first }
            , common{ first, last } {
            if (current == last) {
                throw api::v1::ctk_limit{ "bit_stream_reader: empty input" };
            }
            assert(common.total != 0);

            common.available = one_byte();
            common.accumulator = *current;
            ++current;
        }
    };

    template<typename IByte>
    struct bit_stream_writer
    {
        IByte current; // output stream
        bit_stream common;

        // NB: the range [first, last) should be zero-initialized by the caller
        bit_stream_writer(IByte first, IByte last)
            : current{ first }
            , common{ first, last } {
        }
    };

    // transfers n bits from the input bit stream into the output word.
    // stores the leftover bits from the last consumed input byte in the accumulator
    template<typename IByteConst, typename T>
    // requirements
    // T is unsigned integral
    auto read_n(bit_stream_reader<IByteConst>& stream, bit_count n, T /*type tag*/) -> T {
        using A = typename bit_stream::A;

        static_assert(sizeof(T) < sizeof(A), "A should be at least one byte wider");
        static_assert(std::is_unsigned<T>::value);
        static_assert(std::is_unsigned<A>::value);
        assert(0 <= n);
        assert(n <= size_in_bits<T>());
        assert(stream.common.available <= one_byte());

        if (stream.common.total < n) {
            throw api::v1::ctk_data{ "read_n: not enough bits" };
        }

        while (stream.common.available < n) {
            stream.common.accumulator = (stream.common.accumulator << bits_per_byte) | *stream.current;

            ++stream.current;
            stream.common.available += one_byte();
        }
        assert(n <= stream.common.available);

        const bit_count::value_type shift{ stream.common.available - n };
        stream.common.available -= n;
        stream.common.total -= n;

        return mask_msb(static_cast<T>(stream.common.accumulator >> shift), n);
    }



    // transfers the n least significant bits from the input word into the output bit stream.
    template<typename IByte, typename T>
    // requirements
    // T is unsigned integral
    auto write_n(bit_stream_writer<IByte>& stream, bit_count n, T input) -> void {
        using A = typename bit_stream::A;

        static_assert(sizeof(T) < sizeof(A), "A should be at least one byte wider");
        static_assert(std::is_unsigned<T>::value);
        static_assert(std::is_unsigned<A>::value);
        assert(0 <= n);
        assert(n <= size_in_bits<T>());
        assert(stream.common.available <= one_byte());

        if (stream.common.total < n) {
            throw api::v1::ctk_data{ "write_n: not enough bits" };
        }

        const bit_count::value_type sn{ n };
        stream.common.accumulator = (stream.common.accumulator << sn) | mask_msb(input, n);
        stream.common.available += n;
        stream.common.total -= n;

        while (one_byte() < stream.common.available) {
            const bit_count::value_type shift{ stream.common.available - one_byte() };

            *stream.current = static_cast<uint8_t>(mask_msb(stream.common.accumulator >> shift, one_byte()));

            ++stream.current;
            stream.common.available -= one_byte();
        }
    }


    template<typename IByteConst>
    auto flush_reader(bit_stream_reader<IByteConst>& stream) -> IByteConst {
        assert(stream.common.available <= one_byte()); // read_n never leaves more than bits_per_byte bits in the accumulator

        stream.common.total -= stream.common.available;
        stream.common.available = bit_count{ 0 };
        return stream.current;
    }

    template<typename IByte>
    auto flush_writer(bit_stream_writer<IByte>& stream) -> IByte {
        assert(stream.common.available <= one_byte()); // write_n never leaves more than bits_per_byte bits in the accumulator

        if (stream.common.available == 0) {
            return stream.current;
        }

        const auto leftover{ one_byte() - stream.common.available };
        const bit_count::value_type shift{ leftover };
        *stream.current = static_cast<uint8_t>(stream.common.accumulator << shift);
        ++stream.current;
        stream.common.total -= leftover;
        stream.common.available = bit_count{ 0 };

        return stream.current;
    }


    template<typename IByteConst>
    auto read_64(bit_stream_reader<IByteConst>& stream, bit_count n) -> uint64_t {
        constexpr const bit_count half_size{ size_in_bits<uint32_t>() };
        constexpr const uint32_t half_type{ 0 };

        if (n <= half_size) {
            return read_n(stream, n, half_type);
        }

        const uint32_t hw{ read_n(stream, n - half_size, half_type) };
        const uint32_t low_word{ read_n(stream, half_size, half_type) };

        const uint64_t high_word{ hw };
        constexpr const auto shift{ as_sizet_unchecked(half_size) };
        return (high_word << shift) | low_word;
    }


    template<typename IByte>
    auto write_64(bit_stream_writer<IByte>& stream, bit_count n, uint64_t input) -> void {
        constexpr const bit_count half_size{ size_in_bits<uint32_t>() };
        const auto low_word{ static_cast<uint32_t>(mask_msb(input, half_size)) };

        if (n <= half_size) {
            write_n(stream, n, low_word);
            return;
        }

        constexpr const auto shift{ as_sizet_unchecked(half_size) };
        const auto high_word{ static_cast<uint32_t>(input >> shift) };

        write_n(stream, n - half_size, high_word);
        write_n(stream, half_size, low_word);
    }



  
    // interface

    template<typename IByteConst>
    class bit_reader final
    {
        impl::bit_stream_reader<IByteConst> stream;

    public:

        bit_reader(IByteConst first, IByteConst last)
            : stream{ first, last } {
        }

        template<typename T>
        auto read(bit_count n, T type_tag) -> T {
            return impl::read_n(stream, n, type_tag);
        }

        auto read(bit_count n, uint64_t /* type tag */) -> uint64_t {
            return impl::read_64(stream, n);
        }

        auto flush() -> IByteConst {
            return impl::flush_reader(stream);
        }

        auto current() const -> IByteConst {
            return stream.current;
        }

        auto count() const -> bit_count {
            return stream.common.total;
        }
    };



    template<typename IByte>
    class bit_writer final
    {
        impl::bit_stream_writer<IByte> stream;

    public:

        // NB: the range [first, last) should be zero-initialized by the caller
        bit_writer(IByte first, IByte last)
            : stream{ first, last } {
        }

        template<typename T>
        auto write(bit_count n, T input) -> void {
            write_n(stream, n, input);
        }

        auto write(bit_count n, uint64_t input) -> void {
            write_64(stream, n, input);
        }

        auto flush() -> IByte {
            return flush_writer(stream);
        }

        auto current() const -> IByte {
            return stream.current;
        }

        auto count() const -> bit_count {
            return stream.common.total;
        }
    };
    

} /* namespace impl */ } /* namespace ctk */

