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

#include <tuple>

#include "type_wrapper.h"
#include "compress/bit_stream.h"


/*
    A data block consists of a (reflib/extended) header and (compressed/uncompressed) payload.
    
    The uncompressed block consists of a header byte followed by verbatim copy of the input data.

    The compressed block consists of a fixed length header followed by the compressed payload.
    The first value in the compressed payload is verbatim copy of the first element from the input sequence and
    is refered to as master value.
    The master value is followed by tightly packed (with leading zeroes/ones removed) residual data.
    The residual data is derived from the input provided by the application to the functions in matrix.h.
    The functions in this file perform the tight packing/unpacking of residual data into/from the block payload.

    The header has two different formats: reflib and extended.
    The reflib format is used by libeeep (https://sourceforge.net/projects/libeep/) and is kept for compatibility reasons "as is".
    The extended format is INCOMPATIBLE extension which works with all current signed and unsigned integers: 8, 16, 32 and 64 bit wide.

    Data block structure

    1. Header

    1.1. Common (compressed/uncompressed) prefix: encoding scheme

    The bits marked with 'm' encode the compression method:
        - 00b: copy: verbatim copy of the input data
        - 01b: time: stores the difference between adjacent values in one matrix row
        - 10b: time2: stores the difference between adjacent residuals as computed by method time
        - 11b: channel: uses data from the previous matrix row as well

    Reflib format
    | byte 0        |
    |7|6|5|4|3|2|1|0|   bit indices (as seen from C/C++)
    |s| |m|m| | | | |

    The bit marked with 's' encodes the data size:
        - 0: 16-bit data
        - 1: 32-bit data

    Extended format
    | byte 0        |
    |7|6|5|4|3|2|1|0|   bit indices (as seen from C/C++)
    |s|s|m|m| | | | |

    The bit marked with 's' encodes the data size:
        - 00b: 8-bit data
        - 01b: 16-bit data
        - 10b: 32-bit data
        - 11b: 64-bit data

    1.2. Uncompressed block header

    The meaning of the last 4 bits - marked with ' ' in section 1.1 - is not defined in an uncompressed block header.
    The uncompressed block header consists of one byte.

    1.3. Compressed block header
    
    The scheme (section 1.1) is followed by the quantities n and nexc and verbatim copy of the first element in the matrix row

    Reflib format
    4/6 bits    amount of n bits    (4 bits for 16-bit input data, 6 bits for 32-bit input data)
    4/6 bits    amount of nexc bits (4 bits for 16-bit input data, 6 bits for 32-bit input data)
    16/32 bits  master value

    Example: structure of 32-bit compressed header reflib
    | byte 0        | byte 1        | byte 2        | byte 3        | byte 4        |
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|   indices in the input byte stream (as seen from C/C++)
    |s| |m|m|n|n|n|n|n|n|e|e|e|e|e|e| master value (32 bits)
    The bit marked with 's' encodes the data size
    The bits marked with 'm' encode the compression method
    The bits marked with 'n' encode the amount of n bits for this block in 6 bits
    The bits marked with 'e' encode the amount of nexc bits for this block in 6 bits

    Extended format
    3/4/5/6 bits    amount of n bits    (3 bits for 8-bit input data, 4 bits for 16-bit input data, 5 bits for 32-bit input data, 6 bits for 64-bit input data)
    3/4/5/6 bits    amount of nexc bits (3 bits for 8-bit input data, 4 bits for 16-bit input data, 5 bits for 32-bit input data, 6 for 64-bit input data)
    8/16/32/64 bits master value

    Example: structure of 32-bit compressed header extended
    | byte 0        | byte 1        | byte 2        | byte 3        | byte 4        |
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|   indices in the input byte stream (as seen from C/C++)
    |s|s|m|m|m|n|n|n|n|n|e|e|e|e|e| master value (32 bits)
    The bit marked with 's' encodes the data size
    The bits marked with 'm' encode the compression method
    The bits marked with 'n' encode the amount of n bits for this block in 5 bits
    The bits marked with 'e' encode the amount of nexc bits for this block in 5 bits

    2. Block payload

    2.1. Uncompressed block payload
    
    Verbatim copy of the input data starting from the byte after the header byte.

    2.2. Compressed block payload

    2.2.1 Fixed size encoding

    The quantities n and nexc from section 1.3 are equal.
    All entities are encoded in n bit wide groups.

    2.2.2 Variable size encoding

    The quantities n and nexc from section 1.3 are not equal. n < nexc.
    Most entities are encoded in n bit wide groups. Some are encoded in (n + nexc) bit wide groups.
    One n bit sized pattern is designated as special. It is referred to as an "exception marker".
    If an entity is encoded in (n + nexc) bits the first n bits are equal to the exception marker
    and the next nexc bits contain the actual value.

    Example: n = 7, nexc = 10 bits
    | entity i    | entity i + 1| entity i + 2                    | entity i + 3
    | value       | value       | exc. marker | value             | value
    | n bits      | n bits      | n bits      | nexc bits         | n bits
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|...
*/

namespace ctk { namespace impl {

    using IBoolConst = typename std::vector<bool>::const_iterator;


    // each entity stored in the data block payload is encoded as a signum bit followed by one or more data bits.
    static constexpr int nbits_min{ 2 };

    constexpr
    auto pattern_size_min() -> bit_count {
        return bit_count{ nbits_min };
    }


    template<typename T>
    // requirements
    //     - T is signed integral type with two-complement implementation
    constexpr
    auto is_set(T pattern, bit_count n) -> bool {
        assert(0 < n && n <= size_in_bits(pattern));
        const bit_count::value_type sn{ n };
        return (pattern & (T{ 1 } << (sn - 1))) != 0;
    }


    template<typename T>
    // requirements
    //     - T is integral type with two-complement implementation
    auto restore_sign(T pattern, bit_count n) -> T {
        const auto word_size{ size_in_bits(pattern) };
        assert(nbits_min <= n);
        assert(n <= word_size);

        if (n < word_size && is_set(pattern, n)) {
            constexpr const auto mask{ T(~0) };
            return static_cast<T>(pattern | (mask << static_cast<bit_count::value_type>(n)));
        }

        return pattern;
    }


    // size of the method and encoding_size fields
    constexpr
    auto field_width_encoding() -> bit_count {
        return bit_count{ 2 };
    }


    auto field_width_master(encoding_size data_size) -> bit_count;
    auto restore_n(bit_count n, bit_count data_size) -> bit_count;
    auto sizeof_word(encoding_size x) -> unsigned;


    // represents the header structure of the bytestream in the reference library implementation
    // https://sourceforge.net/projects/libeep/
    struct reflib
    {
        static constexpr
        auto decode_size(unsigned pattern) -> encoding_size {
            static_assert(field_width_encoding() == 2);

            if (pattern & 0b10) {
                return encoding_size::four_bytes;
            }
            else {
                return encoding_size::two_bytes;
            }
        }

        static constexpr
        auto encode_size(encoding_size data_size) -> unsigned {
            static_assert(field_width_encoding() == 2);

            if (data_size == encoding_size::four_bytes) {
                return 0b10;
            }

            return 0b00;
        }

        template<typename T>
        static
        auto is_valid_size(T, encoding_size data_size) -> bool {
            static_assert(sizeof(T) == sizeof(uint32_t), "compatibility");

            const unsigned x{ sizeof_word(data_size) };
            return x == sizeof(uint16_t) || x == sizeof(uint32_t);
        }

        // size of the n and nexc bit fields
        static
        auto field_width_n(encoding_size data_size) -> bit_count {
            switch(data_size) {
                case encoding_size::two_bytes: return bit_count{ 4 }; // value interval [0, 15], 0 interpreted as 16
                case encoding_size::four_bytes: return bit_count{ 6 };
                default: throw api::v1::CtkBug{ "reflib::field_width_n: invalid data size" };
            }
        }

        template<typename T>
        static constexpr
        auto as_size(T) -> encoding_size {
            static_assert(sizeof(T) == sizeof(uint32_t), "compatibility");

            return encoding_size::four_bytes;
        }

        template<typename T>
        static
        auto restore_encoding(T master, bit_count n, bit_count nexc, encoding_size data_size) -> std::tuple<T, bit_count, bit_count> {
            if (data_size == encoding_size::four_bytes) {
                // n and nexc are stored in 6 bit wide fields => no need to reinterpret 0 as 32
                assert(field_width_n(data_size) == 6);
                // master is 4 bytes => no need to restore its sign
                static_assert(sizeof(T) == sizeof(uint32_t), "compatibility");

                return { master, n, nexc };
            }

            const auto word_size{ field_width_master(data_size) };
            return { restore_sign(master, word_size), restore_n(n, word_size), restore_n(nexc, word_size) };
        }
    };


    // INCOMPATIBLE implementation of the reference library header structure extended to words with size 1, 2, 4 and 8 bytes.
    // corrected the sizes of the n/nexc bit fields in the 4byte encoding.
    struct extended
    {
        static
        auto decode_size(unsigned pattern) -> encoding_size {
            if (static_cast<unsigned>(encoding_size::eight_bytes) < pattern) {
                static_assert(field_width_encoding() == 2);
                throw api::v1::CtkBug{ "extended::decode_size: 2 bits = 4 possible interpretations" };
            }

            return encoding_size(pattern);
        }

        static
        auto encode_size(encoding_size data_size) -> unsigned {
            if (data_size == encoding_size::length) {
                throw api::v1::CtkBug{ "extended::encode_size: invalid data size" };
            }

            return static_cast<unsigned>(data_size);
        }



        template<typename T>
        static constexpr
        auto is_valid_size(T, encoding_size data_size) -> bool {
            return sizeof_word(data_size) <= sizeof(T);
        }

        // size of the n and nexc bit fields
        static
        auto field_width_n(encoding_size data_size) -> bit_count {
            switch(data_size) {
                case encoding_size::one_byte: return bit_count{ 3 };    // value interval [0, 7],  0 interpreted as 8
                case encoding_size::two_bytes: return bit_count{ 4 };   // value interval [0, 15], 0 interpreted as 16
                case encoding_size::four_bytes: return bit_count{ 5 };  // value interval [0, 31], 0 interpreted as 32
                case encoding_size::eight_bytes: return bit_count{ 6 }; // value interval [0, 63], 0 interpreted as 64
                default: throw api::v1::CtkBug{ "extended::field_width_n: invalid data size" };
            }
        }

        template<typename T>
        static
        auto as_size(T) -> encoding_size {
            static_assert(sizeof(uint8_t) <= sizeof(T) && sizeof(T) <= sizeof(uint64_t));

            switch(sizeof(T)) {
                case sizeof(uint8_t): return encoding_size::one_byte;
                case sizeof(uint16_t): return encoding_size::two_bytes;
                case sizeof(uint32_t): return encoding_size::four_bytes;
                case sizeof(uint64_t): return encoding_size::eight_bytes;
                default: throw api::v1::CtkBug{ "extended::as size: invalid data size" };
            }
        }

        template<typename T>
        static
        auto restore_encoding(T master, bit_count n, bit_count nexc, encoding_size data_size) -> std::tuple<T, bit_count, bit_count> {
            const auto word_size{ field_width_master(data_size) };
            return { restore_sign(master, word_size), restore_n(n, word_size), restore_n(nexc, word_size) };
        }
    };


    auto uncompressed_header_width() -> bit_count;

    template<typename Format>
    auto compressed_header_width(encoding_size data_size, Format) -> bit_count {
        return field_width_encoding()             // encoding data size
             + field_width_encoding()             // method
             + Format::field_width_n(data_size)   // n
             + Format::field_width_n(data_size)   // nexc
             + field_width_master(data_size);     // master value
    }

    

    // bit pattern used as an exception marker for n-bit sized bit groups.
    template<typename T>
    // requirements
    //     - T is integral type with two-complement implementation
    auto exception_marker(bit_count n) -> T {
        assert(0 < n && n <= size_in_bits(T{}));
        const bit_count::value_type sn{ n };
        return static_cast<T>(T{ 1 } << (sn - 1));
    }

    
    template<typename T>
    auto is_exception_marker(T pattern, bit_count n) -> bool {
        return mask_msb(pattern, n) == exception_marker<T>(n);
    }


    // reads/writes an entity from/into the output data block.
    // the entity is encoded either in n bits or in (n + nexc) bits.
    struct entity_variable_width
    {
        const bit_count n;
        const bit_count nexc;

        template<typename IByte, typename T>
        auto encode(bit_writer<IByte>& bits, T pattern, bool is_exceptional) const -> void {
            assert(0 < n && n <= size_in_bits(pattern));
            assert(0 < nexc && nexc <= size_in_bits(pattern));

            if (!is_exceptional) {
                bits.write(n, pattern);
                return;
            }

            bits.write(n, exception_marker<T>(n));
            bits.write(nexc, pattern);
        }

        template<typename IByteConst, typename T>
        auto decode(bit_reader<IByteConst>& bits, T pattern) const -> T {
            assert(0 < n && n <= size_in_bits(pattern));
            assert(0 < nexc && nexc <= size_in_bits(pattern));

            pattern = bits.read(n, pattern);
            if (!is_exception_marker(pattern, n)) {
                return restore_sign(pattern, n);
            }

            pattern = bits.read(nexc, pattern);
            return restore_sign(pattern, nexc);
        }
    };



    // reads/writes an entity from/into the output data block.
    // the entity is encoded in n bits.
    struct entity_fixed_width
    {
        const bit_count n;

        template<typename IByte, typename T>
        auto encode(bit_writer<IByte>& bits, T pattern, bool /* is_exceptional */) const -> void {
            assert(0 < n && n <= size_in_bits(pattern));
            bits.write(n, pattern);
        }

        template<typename IByteConst, typename T>
        // requirements
        //     - T is unsigned integral type with two's complement implementation
        auto decode(bit_reader<IByteConst>& bits, T type_tag) const -> T {
            assert(0 < n && n <= size_in_bits(type_tag));
            return restore_sign(bits.read(n, type_tag), n);
        }
    };




    template<typename IConst, typename IByte, typename Op>
    //  - IConst is ForwardIterator
    //  - ValueType(IConst) is unsigned integral type with two-complement implementation
    //  - Op has an interface identical to entity_fixed_width or entity_variable_width
    auto write_payload(IConst first, IConst last, IBoolConst encoding_map, bit_writer<IByte>& bits, Op op) -> IByte {
        for (/* empty */; first != last; ++first, ++encoding_map) {
            op.encode(bits, *first, *encoding_map);
        }

        return bits.flush();
    }



    template<typename I, typename IByteConst, typename Op>
    //  - I is ForwardIterator
    //  - ValueType(I) is unsigned integral type with two-complement implementation
    //  - Op has an interface identical to entity_fixed_width or entity_variable_width
    auto read_payload(I first, I last, bit_reader<IByteConst>& bits, Op op) -> IByteConst {
        using T = typename std::iterator_traits<I>::value_type;
        constexpr const T type_tag{0};

        for (/* empty */; first != last; ++first) {
            *first = op.decode(bits, type_tag);
        }

        return bits.flush();
    }


    auto is_valid_uncompressed(bit_count n, bit_count nexc, encoding_size data_size) -> bool;
    auto is_valid_compressed(bit_count n, bit_count nexc, encoding_size data_size) -> bool;

    template<typename T, typename Format>
    auto valid_block_encoding(encoding_size data_size, encoding_method m, bit_count n, bit_count nexc, T type_tag, Format) -> bool {
        if (!Format::is_valid_size(type_tag, data_size)) {
            return false;
        }

        if (m == encoding_method::copy) {
            return is_valid_uncompressed(n, nexc, data_size);
        }

        return is_valid_compressed(n, nexc, data_size);
    }


    // compressed header
    template<typename I, typename IByteConst, typename Format>
    // requirements:
    //  - I is ForwardIterator
    //  - ValueType(I) is unsigned integral type with two-complement implementation
    auto read_encoding_compressed(I first, I last, bit_reader<IByteConst>& bits, encoding_size data_size, Format) -> std::tuple<I, bit_count, bit_count> {
        using T = typename std::iterator_traits<I>::value_type;

        if (size_in_bits(T{}) < field_width_master(data_size)) {
            throw api::v1::CtkData{ "read_encoding_compressed, invalid master field width for this data size" };
        }

        if (first == last) {
            throw api::v1::CtkBug{ "read_encoding_compressed, precondition violation: empty output range" };
        }

        const auto n_width{ Format::field_width_n(data_size) };

        const auto un{ bits.read(n_width, unsigned{}) };
        const auto unexc{ bits.read(n_width, unsigned{}) };
        const auto master{ bits.read(field_width_master(data_size), T{}) };

        bit_count n{ static_cast<bit_count::value_type>(un) };
        bit_count nexc{ static_cast<bit_count::value_type>(unexc) };

        std::tie(*first, n, nexc) = Format::restore_encoding(master, n, nexc, data_size);

        return { std::next(first), n, nexc };
    }


    // uncompressed header
    template<typename IByteConst, typename Format>
    auto read_encoding_uncompressed(bit_reader<IByteConst>& bits, encoding_size data_size, Format) -> std::tuple<bit_count, bit_count> {
        constexpr const auto consumed{ field_width_encoding() + field_width_encoding() }; // encoding size + method
        static_assert(consumed <= one_byte());

        bits.read(one_byte() - consumed, unsigned{}); // padding

        // n == nexc == sizeof(T) * 8
        const auto n{ field_width_master(data_size) };
        return { n, n };
    }


    auto decode_method(unsigned) -> encoding_method;
    auto invalid_row_header(encoding_size, encoding_method, bit_count, bit_count, size_t) -> std::string;

    template<typename I, typename IByteConst, typename Format>
    auto read_header(I first, I last, bit_reader<IByteConst>& bits, Format format) -> std::tuple<I, bit_count, bit_count, encoding_method> {
        static_assert(field_width_encoding() == 2);

        const auto usize{ bits.read(field_width_encoding(), unsigned{}) };
        const auto umethod{ bits.read(field_width_encoding(), unsigned{}) };
        assert(usize < 5);   // because field_width_encoding() == 2
        assert(umethod < 5);
        const auto data_size{ Format::decode_size(usize) };
        const auto method{ decode_method(umethod) };

        bit_count n, nexc;
        I next{ first };
        if (method == encoding_method::copy) {
            std::tie(n, nexc) = read_encoding_uncompressed(bits, data_size, format);
        }
        else {
            std::tie(next, n, nexc) = read_encoding_compressed(first, last, bits, data_size, format);
        }

        using T = typename std::iterator_traits<I>::value_type;
        if (!valid_block_encoding(data_size, method, n, nexc, T{}, format)) {
            throw api::v1::CtkData{ invalid_row_header(data_size, method, n, nexc, sizeof(T)) };
        }

        return { next, n, nexc, method};
    }


    // writes a compressed header
    template<typename IConst, typename IByte, typename Format>
    // requirements
    //  - IConst is ForwardIterator
    //  - ValueType(IConst) is unsigned integral type with two-complement implementation
    auto write_header_compressed(IConst first, IBoolConst encoding_map, bit_writer<IByte>& bits, encoding_size data_size, encoding_method m, bit_count n, bit_count nexc, Format) -> std::pair<IConst, IBoolConst> {
        assert(pattern_size_min() <= n);
        assert(n <= size_in_bits(typename std::iterator_traits<IConst>::value_type{}));
        assert(n <= nexc);
        assert(nexc <= size_in_bits(typename std::iterator_traits<IConst>::value_type{}));

        const auto un{ as_sizet_unchecked(n) };
        const auto unexc{ as_sizet_unchecked(nexc) };
        const auto n_width{ Format::field_width_n(data_size) };

        bits.write(field_width_encoding(), Format::encode_size(data_size));
        bits.write(field_width_encoding(), static_cast<unsigned>(m));
        bits.write(n_width, un);
        bits.write(n_width, unexc);
        bits.write(field_width_master(data_size), *first);

        return { std::next(first), std::next(encoding_map) };
    }


    template<typename IConst, typename IByte, typename Format>
    // requirements
    //  - IConst is ForwardIterator
    //  - ValueType(IConst) is unsigned integral type with two-complement implementation
    auto write_header(IConst first, IBoolConst encoding_map, bit_writer<IByte>& bits, encoding_size data_size, encoding_method m, bit_count n, bit_count nexc, Format format) -> std::pair<IConst, IBoolConst> {
        if (m == encoding_method::copy) {
            bits.write(field_width_encoding(), static_cast<unsigned>(data_size));
            bits.write(field_width_encoding(), static_cast<unsigned>(encoding_method::copy));

            constexpr const auto scheme{ field_width_encoding() + field_width_encoding() };
            static_assert(scheme <= one_byte());
            bits.write(one_byte() - scheme, unsigned{0}); // padding

            return { first, encoding_map };
        }

        return write_header_compressed(first, encoding_map, bits, data_size, m, n, nexc, format);
    }



    // method_copy
    template<typename Format, typename T>
    constexpr
    auto max_block_size(measurement_count length, Format, T) -> byte_count {
        const bit_count header{ uncompressed_header_width() };
        const bit_count data{ scale(size_in_bits(T{}), length, ok{}) };
        const bit_count::value_type h{ header };
        const bit_count::value_type d{ data };

        const bit_count bits{ plus(h, d, ok{}) };
        return as_bytes(bits);
    }

    template<typename IConst, typename Format>
    constexpr
    auto max_block_size(IConst first, IConst last, Format format) -> byte_count {
        using T = typename std::iterator_traits<IConst>::value_type;

        using D = decltype(std::distance(first, last));
        const D length{ std::distance(first, last) };
        const auto measurements{ measurement_count{ cast(length, D{}, ok{}) } };
        return max_block_size(measurements, format, T{});
    }


    // interface



    // converts the input words from the interval [ifirst, ilast) into output data block.
    // writes out the bytes into the interval [ofirst, olast).
    // returns an iterator from the output interval: one past the last output byte to which bits were written.
    template<typename IConst, typename IByte, typename Format>
    // requirements
    //  - IConst is ForwardIterator
    //  - ValueType(IConst) is unsigned integral type with two-complement implementation
    //  - IByte is ForwardIterator
    //  - ValueType(IByte) is at least bits_per_byte bits wide
    //  - Format has an interface identical to reflib or extended
    auto encode_block(IConst ifirst, IConst ilast, IBoolConst encoding_map, bit_writer<IByte>& bits, encoding_size data_size, encoding_method method, bit_count n, bit_count nexc, Format format) -> IByte {
        const auto [inext, bnext]{ write_header(ifirst, encoding_map, bits, data_size, method, n, nexc, format) };

        if (n == nexc) {
            return write_payload(inext, ilast, bnext, bits, entity_fixed_width{ n });
        }

        return write_payload(inext, ilast, bnext, bits, entity_variable_width{ n, nexc });
    }


    // decodes [ofirst, olast) words from the input data block into the output word stream.
    // reads the bytes from the input interval [ifirst, ilast).
    // returns an iterator from the input interval: one past the input byte from which bits were consumed.
    template<typename I, typename IByteConst, typename Format>
    // requirements:
    //  - I is ForwardIterator
    //  - ValueType(I) is unsigned integral type with two-complement implementation
    //  - IByte is ForwardIterator
    //  - ValueType(IByte) is at least bits_per_byte bits wide
    //  - Format has an interface identical to reflib or extended
    auto decode_block(bit_reader<IByteConst>& bits, I ofirst, I olast, Format format) -> std::pair<IByteConst, encoding_method> {
        const auto [onext, n, nexc, method]{ read_header(ofirst, olast, bits, format) };

        if (n == nexc) {
            return { read_payload(onext, olast, bits, entity_fixed_width{ n }), method };
        }

        return { read_payload(onext, olast, bits, entity_variable_width{ n, nexc }), method };
    }

} /* namespace impl */ } /* namespace ctk */

