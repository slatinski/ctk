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

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../catch.hpp"

#include <iostream>
#include <numeric>

#include "file/leb128.h"
#include "qcheck.h"


namespace ctk { namespace impl { namespace test {

    struct dwarf_u
    {
        unsigned n;
        std::vector<uint8_t> bytes;
    };

    struct dwarf_s
    {
        int n;
        std::vector<uint8_t> bytes;
    };

    // DWARF Debugging Information Format, Version 4
    // SECTION 7-- DATA REPRESENTATION
    // 7.6 Variable Length Data
    // Figure 22. Examples of unsigned LEB128 encodings
    const std::vector<dwarf_u> example_u{
        { 2, { 2 } },
        { 127, { 127 } },
        { 128, { 0 + 0x80, 1 } },
        { 129, { 1 + 0x80, 1 } },
        { 130, { 2 + 0x80, 1 } },
        { 12857, { 57 + 0x80, 100 } }
    };

    // DWARF Debugging Information Format, Version 4
    // SECTION 7-- DATA REPRESENTATION
    // 7.6 Variable Length Data
    // Figure 23. Examples of signed LEB128 encodings
    const std::vector<dwarf_s> example_s{
        { 2, { 2 } },
        { -2, { 0x7e } },
        { 127, { 127 + 0x80, 0 } },
        { -127, { 1 + 0x80, 0x7f } },
        { 128, { 0 + 0x80, 1 } },
        { -128, { 0 + 0x80, 0x7f } },
        { 129, { 1 + 0x80, 1 } },
        { -129, { 0x7f + 0x80, 0x7e } }
    };



    auto input_consecutive_backward() -> std::vector<int64_t> {
        std::vector<int64_t> xs;
        for (int64_t i{ std::numeric_limits<int64_t>::max() }; i > std::numeric_limits<int64_t>::max() - 4096; --i) {
            xs.push_back(i);
        }
        return xs;
    }

    auto well_known_ints() -> std::vector<int> {
        std::vector<int> xs(example_s.size());
        std::transform(begin(example_s), end(example_s), begin(xs), [](const auto& x) -> int { return x.n; });
        return xs;
    }

    auto well_known_unsigned_ints() -> std::vector<unsigned> {
        std::vector<unsigned> xs(example_u.size());
        std::transform(begin(example_u), end(example_u), begin(xs), [](const auto& x) -> unsigned { return x.n; });
        return xs;
    }

    auto all_int16s() -> std::vector<int16_t> {
        std::vector<int16_t> xs(std::numeric_limits<int16_t>::max() - std::numeric_limits<int16_t>::min());
        std::iota(begin(xs), end(xs), std::numeric_limits<int16_t>::min());
        return xs;
    }

    auto all_uint16s() -> std::vector<uint16_t> {
        std::vector<uint16_t> xs(std::numeric_limits<uint16_t>::max());
        std::iota(begin(xs), end(xs), uint16_t{ 0 });
        return xs;
    }

    auto around_zero() -> std::vector<int32_t> {
        std::vector<int32_t> xs(2048);
        std::iota(begin(xs), end(xs), int32_t{ -1024 });
        return xs;
    }



    template<typename T>
    auto roundtrip(T input) -> void {
        const std::vector<uint8_t> bytes{ encode_leb128_v(input) };
        const T output{ decode_leb128_v(bytes, T{}) };
        REQUIRE(output == input);
    }

    TEST_CASE("single number roundtrip", "[consistency]") {
        std::vector<uint8_t> empty;
        auto [zero, next]{ decode_leb128(begin(empty), end(empty), int{}) };
        REQUIRE(next == end(empty));
        REQUIRE(zero == 0);

        roundtrip(std::numeric_limits<int8_t>::min());
        roundtrip(std::numeric_limits<int16_t>::min());
        roundtrip(std::numeric_limits<int32_t>::min());
        roundtrip(std::numeric_limits<int64_t>::min());
        roundtrip(std::numeric_limits<int8_t>::max());
        roundtrip(std::numeric_limits<int16_t>::max());
        roundtrip(std::numeric_limits<int32_t>::max());
        roundtrip(std::numeric_limits<int64_t>::max());

        roundtrip(std::numeric_limits<uint8_t>::min());
        roundtrip(std::numeric_limits<uint16_t>::min());
        roundtrip(std::numeric_limits<uint32_t>::min());
        roundtrip(std::numeric_limits<uint64_t>::min());
        roundtrip(std::numeric_limits<uint8_t>::max());
        roundtrip(std::numeric_limits<uint16_t>::max());
        roundtrip(std::numeric_limits<uint32_t>::max());
        roundtrip(std::numeric_limits<uint64_t>::max());

        for (int16_t i{ std::numeric_limits<int16_t>::min() }; i < std::numeric_limits<int16_t>::max(); ++i) {
            roundtrip(i);
        }

        for (uint16_t i{ 0 }; i < std::numeric_limits<uint16_t>::max(); ++i) {
            roundtrip(i);
        }
    }

    TEST_CASE("well known representations", "[correct]") {
        for (size_t i{ 0 }; i < example_s.size(); ++i) {
            const std::vector<uint8_t> bytes{ encode_leb128_v(example_s[i].n) };
            REQUIRE(bytes == example_s[i].bytes);
        }

        for (size_t i{ 0 }; i < example_u.size(); ++i) {
            const std::vector<uint8_t> bytes{ encode_leb128_v(example_u[i].n) };
            REQUIRE(bytes == example_u[i].bytes);
        }
    }

    TEST_CASE("invalid input", "[correct]") {
        const std::vector<uint8_t> only_continuation{ 0x80 };
        REQUIRE_THROWS(decode_leb128(begin(only_continuation), end(only_continuation), int16_t{}));

        const std::vector<uint8_t> extra_continuation{ 0x80, 0x80 };
        REQUIRE_THROWS(decode_leb128(begin(extra_continuation), end(extra_continuation), int16_t{}));

        const std::vector<uint8_t> not_enough_output_bits{ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x1 };
        REQUIRE_THROWS(decode_leb128(begin(not_enough_output_bits), end(not_enough_output_bits), int64_t{}));
    }


    // example:
    // the byte sequence { 142, 123 } encodes the signed number -626 (0b10110001110).
    // decoding the sequence into a 8-bit word would yield the signed number -114 (0b10001110) due to truncation.
    // decoding the sequence into a 16-bit or wider signed word would yield the signed number -626.
    // this function takes the simplistic approach to decode the sequence into a wider word and compare the results.
    // this simplistic approach precludes using it for 64-bit wide words.
    template<typename IByte, typename T>
    auto representable_as_t(IByte first, IByte last, T) -> bool {
        static_assert(sizeof(T) < sizeof(int64_t));

        const auto[x, u0]{ decode_leb128(first, last, T{}) };
        const auto[y, u1]{ decode_leb128(first, last, int64_t{}) };
        return x == y;
    }

    // the following one and two byte wide words encode the same value:
    //          01100100
    //  0000000001100100
    // if the byte sequence specifies leading zeroes or ones then it is not the shortest representation for this value.
    template<typename IByte, typename T>
    auto shortest_representation(IByte first, IByte last, T) -> bool {
        if (first == last) {
            return true;
        }
        assert((*(last - 1) & 0x80) == 0);

        const auto[x, u]{ decode_leb128(first, last, T{}) };
        if (x < 0) {
            return *(last - 1) != 127; // leading 1s: not the shortest representation
        }
        else {
            return *(last - 1) != 0; // leading 0s: not the shortest representation
        }
    }


    using namespace qcheck;

    template<typename T>
    struct encode_decode_single : arguments<T>
    {
        explicit encode_decode_single(T/* type tag */) {};

        virtual auto holds(const T& x) const -> bool override final {
            std::vector<uint8_t> bytes(leb128::max_bytes(T{}));
            const auto last{ encode_leb128(x, begin(bytes), end(bytes)) };

            const auto[y, next]{ decode_leb128(begin(bytes), last, T{}) };
            return next == last && x == y;
        }
    };


    template<typename T>
    struct decode_encode_single : arguments<std::vector<uint8_t>>
    {
        explicit decode_encode_single(T/* type tag */) {};

        virtual auto accepts(const std::vector<uint8_t>& xs) const -> bool override final {
            using namespace ctk::impl::leb128;

            const size_t size_max{ max_bytes(T{}) };
            const size_t size_x{ xs.size() };
            const size_t size{ std::min(size_x, size_max) };
            for (size_t i{ 0 }; i < size; ++i) {
                if ((xs[i] & 0x80) == 0x0) { // last byte in the sequence, the continuation bit is not set
                    const auto first{ begin(xs) };
                    const auto last{ first + static_cast<ptrdiff_t>(i) + 1 };
                    const bool representable{ representable_as_t(first, last, T{}) };
                    const bool shortest{ shortest_representation(first, last, T{}) }; // the encoder does not produce leading 0s/1s
                    return representable && shortest;
                }
            }

            return false;
        }

        virtual auto holds(const std::vector<uint8_t>& xs) const -> bool override final {
            const auto[word, last_x]{ decode_leb128(begin(xs), end(xs), T{}) };
            const auto length{ std::distance(begin(xs), last_x) };

            std::vector<uint8_t> ys(static_cast<size_t>(length));
            const auto last_y{ encode_leb128(word, begin(ys), end(ys)) };

            const bool equal_size{ std::distance(begin(xs), last_x) == std::distance(begin(ys), last_y) };
            const bool equal_content{ std::equal(begin(xs), last_x, begin(ys), last_y) };
            return equal_size && equal_content;
        }

        virtual auto classify(const std::vector<uint8_t>& xs) const -> std::string override final {
            const size_t size_max{ ctk::impl::leb128::max_bytes(T{}) };
            const size_t size_x{ xs.size() };
            size_t size{ std::min(size_x, size_max) };
            for (size_t i{ 0 }; i < size; ++i) {
                if ((xs[i] & 0x80) == 0x0) { // last byte in the sequence, the continuation bit is not set
                    size = i + 1;
                    break;
                }
            }
            std::ostringstream oss;
            oss << "length " << size << "/" << size_max << " bytes";
            return oss.str();
        }

        virtual auto print(std::ostream& os, const std::vector<uint8_t>& xs) const -> std::ostream& override final {
            return print_vector(os, xs);
        }

        virtual auto shrink(const std::vector<uint8_t>& xs) const -> std::vector<std::vector<uint8_t>> override final {
            return shrink_vector(xs);
        }
    };


    template<typename T>
    struct encode_decode_multiple : arguments<std::vector<T>>
    {
        explicit encode_decode_multiple(T/* type tag */) {};

        virtual auto holds(const std::vector<T>& xs) const -> bool override final {
            std::vector<uint8_t> bytes(xs.size() * leb128::max_bytes(T{}));
            auto first{ begin(bytes) };
            auto last{ end(bytes) };

            for (T x : xs) {
                first = encode_leb128(x, first, last);
            }
            last = first;
            first = begin(bytes);

            T y;
            std::vector<T> ys;
            ys.reserve(xs.size());
            for (size_t i{ 0 }; i < xs.size(); ++i) {
                std::tie(y, first) = decode_leb128(first, last, T{});
                ys.push_back(y);
            }

            return first == last && xs == ys;
        }
    };


    template<typename T>
    struct encode_decode_multiple_file : arguments<std::vector<T>>
    {
        explicit encode_decode_multiple_file(T/* type tag */) {};

        virtual auto holds(const std::vector<T>& xs) const -> bool override final {
            const std::filesystem::path temporary{ "encode_decode_multiple_file.bin" };
            // writer scope
            {
                file_ptr f{ open_w(temporary) };
                for (T x : xs) {
                    write_leb128(f.get(), x);
                }
            }

            bool success{ true };
            // reader scope
            {
                file_ptr f{ open_r(temporary) };
                for (T x : xs) {
                    const T y{ read_leb128(f.get(), T{}) };
                    if (y != x) {
                        success = false;
                        break;
                    }
                }
            }

            std::filesystem::remove(temporary);
            return success;
        }
    };


    class make_short_vectors
    {
        random_source* _random;
        size_t _size;

    public:

        make_short_vectors(random_source& rnd, size_t size)
        : _random{ &rnd }
        , _size{ size } {
        }

        auto operator()(size_t n) -> std::vector<uint8_t> {
            const size_t size{ gen(_size, *_random, size_t{}) };

            std::vector<uint8_t> xs;
            xs.reserve(size);
            for (size_t i{ 0 }; i < size; ++i) {
                xs.push_back(gen(n, *_random, uint8_t{}));
            }

            return xs;
        }
    };

    TEST_CASE("qcheck", "[consistency]") {
        //random_source r{ 926816044 };
        random_source r;

        const size_t size_i8{ ctk::impl::leb128::max_bytes(int8_t{}) * 8 };
        const size_t size_i16{ ctk::impl::leb128::max_bytes(int16_t{}) * 8 };
        const size_t size_i32{ ctk::impl::leb128::max_bytes(int32_t{}) * 8 };

        REQUIRE(check("enc/dec, single, signed 8 bit", encode_decode_single{ int8_t{} }, make_numbers{ r, int8_t{} }));
        REQUIRE(check("enc/dec, single, signed 16 bit", encode_decode_single{ int16_t{} }, make_numbers{ r, int16_t{} }));
        REQUIRE(check("enc/dec, single, signed 32 bit", encode_decode_single{ int32_t{} }, make_numbers{ r, int32_t{} }));
        REQUIRE(check("enc/dec, single, signed 64 bit", encode_decode_single{ int64_t{} }, make_numbers{ r, int64_t{} }));
        REQUIRE(check("enc/dec, single, unsigned 8 bit", encode_decode_single{ uint8_t{} }, make_numbers{ r, uint8_t{} }));
        REQUIRE(check("enc/dec, single, unsigned 16 bit", encode_decode_single{ uint16_t{} }, make_numbers{ r, uint16_t{} }));
        REQUIRE(check("enc/dec, single, unsigned 32 bit", encode_decode_single{ uint32_t{} }, make_numbers{ r, uint32_t{} }));
        REQUIRE(check("enc/dec, single, unsigned 64 bit", encode_decode_single{ uint64_t{} }, make_numbers{ r, uint64_t{} }));

        REQUIRE(check("dec/end, single, signed 8 bit", decode_encode_single{ int8_t{} }, make_short_vectors{ r, size_i8 }, 800));
        REQUIRE(check("dec/end, single, signed 16 bit", decode_encode_single{ int16_t{} }, make_short_vectors{ r, size_i16 }, 800));
        REQUIRE(check("dec/end, single, signed 32 bit", decode_encode_single{ int32_t{} }, make_short_vectors{ r, size_i32 }, 800));
        REQUIRE(check("dec/end, single, unsigned 8 bit", decode_encode_single{ uint8_t{} }, make_short_vectors{ r, size_i8 }, 800));
        REQUIRE(check("dec/end, single, unsigned 16 bit", decode_encode_single{ uint16_t{} }, make_short_vectors{ r, size_i16 }, 800));
        REQUIRE(check("dec/end, single, unsigned 32 bit", decode_encode_single{ uint32_t{} }, make_short_vectors{ r, size_i32 }, 800));

        REQUIRE(check("enc/dec, multiple, signed 8 bit", encode_decode_multiple{ int8_t{} }, make_vectors{ r, int8_t{} }));
        REQUIRE(check("enc/dec, multiple, signed 16 bit", encode_decode_multiple{ int16_t{} }, make_vectors{ r, int16_t{} }));
        REQUIRE(check("enc/dec, multiple, signed 32 bit", encode_decode_multiple{ int32_t{} }, make_vectors{ r, int32_t{} }));
        REQUIRE(check("enc/dec, multiple, signed 64 bit", encode_decode_multiple{ int64_t{} }, make_vectors{ r, int64_t{} }));
        REQUIRE(check("enc/dec, multiple, unsigned 8 bit", encode_decode_multiple{ uint8_t{} }, make_vectors{ r, uint8_t{} }));
        REQUIRE(check("enc/dec, multiple, unsigned 16 bit", encode_decode_multiple{ uint16_t{} }, make_vectors{ r, uint16_t{} }));
        REQUIRE(check("enc/dec, multiple, unsigned 32 bit", encode_decode_multiple{ uint32_t{} }, make_vectors{ r, uint32_t{} }));
        REQUIRE(check("enc/dec, multiple, unsigned 64 bit", encode_decode_multiple{ uint64_t{} }, make_vectors{ r, uint64_t{} }));

        // missing decode_encode_multiple signed/unsigned
        // missing decode_encode_single_file signed/unsigned
        // missing decode_encode_multiple_file signed/unsigned

        REQUIRE(check("enc/dec file, multiple, signed 8 bit", encode_decode_multiple_file{ int8_t{} }, make_vectors{ r, int8_t{} }));
        REQUIRE(check("enc/dec file, multiple, signed 16 bit", encode_decode_multiple_file{ int16_t{} }, make_vectors{ r, int16_t{} }));
        REQUIRE(check("enc/dec file, multiple, signed 32 bit", encode_decode_multiple_file{ int32_t{} }, make_vectors{ r, int32_t{} }));
        REQUIRE(check("enc/dec file, multiple, signed 64 bit", encode_decode_multiple_file{ int64_t{} }, make_vectors{ r, int64_t{} }));
        REQUIRE(check("enc/dec file, multiple, unsigned 8 bit", encode_decode_multiple_file{ uint8_t{} }, make_vectors{ r, uint8_t{} }));
        REQUIRE(check("enc/dec file, multiple, unsigned 16 bit", encode_decode_multiple_file{ uint16_t{} }, make_vectors{ r, uint16_t{} }));
        REQUIRE(check("enc/dec file, multiple, unsigned 32 bit", encode_decode_multiple_file{ uint32_t{} }, make_vectors{ r, uint32_t{} }));
        REQUIRE(check("enc/dec file, multiple, unsigned 64 bit", encode_decode_multiple_file{ uint64_t{} }, make_vectors{ r, uint64_t{} }));

        // fixed inputs
        encode_decode_multiple encdec_i32{ int32_t{} };
        REQUIRE(encdec_i32.holds(around_zero()));

        encode_decode_multiple encdec_i64{ int64_t{} };
        REQUIRE(encdec_i64.holds(input_consecutive_backward()));

        encode_decode_multiple encdec_i{ int{} };
        REQUIRE(encdec_i.holds(well_known_ints()));

        encode_decode_multiple encdec_u{ unsigned{} };
        REQUIRE(encdec_u.holds(well_known_unsigned_ints()));

        encode_decode_multiple encdec_all_i16{ int16_t{} };
        REQUIRE(encdec_all_i16.holds(all_int16s()));

        encode_decode_multiple encdec_all_u16{ uint16_t{} };
        REQUIRE(encdec_all_u16.holds(all_uint16s()));

        encode_decode_multiple_file encdec_i32_file{ int32_t{} };
        REQUIRE(encdec_i32_file.holds(around_zero()));

        encode_decode_multiple_file encdec_i64_file{ int64_t{} };
        REQUIRE(encdec_i64_file.holds(input_consecutive_backward()));

        encode_decode_multiple_file encdec_i_file{ int{} };
        REQUIRE(encdec_i_file.holds(well_known_ints()));

        encode_decode_multiple_file encdec_u_file{ unsigned{} };
        REQUIRE(encdec_u_file.holds(well_known_unsigned_ints()));

        encode_decode_multiple_file encdec_all_i16_file{ int16_t{} };
        REQUIRE(encdec_all_i16_file.holds(all_int16s()));

        encode_decode_multiple_file encdec_all_u16_file{ uint16_t{} };
        REQUIRE(encdec_all_u16_file.holds(all_uint16s()));
    }

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
