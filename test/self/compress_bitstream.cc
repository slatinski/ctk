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

#include <cstdint>
#include <numeric>
#include <algorithm>
#include <tuple>
#include <cassert>
#include <iostream>

#include "compress/bit_stream.h"
#include "qcheck.h"
#include "compress/matrix.h" // count_raw3

namespace ctk { namespace impl { namespace test {

using bit_group = std::pair<bit_count::value_type, uintmax_t>;


template<typename T>
bool representable(const std::vector<bit_group>& groups, T /* type tag */) {
    static_assert(std::is_unsigned<T>::value);
    static_assert(sizeof(T) <= sizeof(uintmax_t));

	const auto first{ std::begin(groups) };
	const auto last{ std::end(groups) };

	return std::none_of(first, last, [](const bit_group& x) { return std::numeric_limits<T>::max() < x.second; });
}



template<typename Reader, typename I, typename T>
void read_bit_groups(Reader reader, const std::vector<bit_group>& groups, I last, T type_tag) {
	uintmax_t expected{ 0 };
    bit_count count{ 0 };

	for (const auto& group : groups) {
		count = bit_count{ group.first };
		expected = static_cast<T>(group.second); // established by representable()

		REQUIRE(expected == reader.read(count, type_tag));
		reader.read(bit_count{ 0 }, type_tag);
	}
	reader.read(bit_count{ 0 }, type_tag);

	REQUIRE(reader.flush() == last);

	// reading one more bit should fail because the whole bit sequence is presumably already consumed
	REQUIRE_THROWS(reader.read(bit_count{ 1 }, type_tag));
}


void test_bit_reader(const std::vector<uint8_t>& input, const std::vector<bit_group>& groups) {
	const auto first{ std::begin(input) };
	const auto last{ std::end(input) };

	if (representable(groups, uint8_t{})) {
		read_bit_groups(bit_reader{first, last}, groups, last, uint8_t{});
	}

	if (representable(groups, uint16_t{})) {
		read_bit_groups(bit_reader{first, last}, groups, last, uint16_t{});
	}

	if (representable(groups, uint32_t{})) {
		read_bit_groups(bit_reader{first, last}, groups, last, uint32_t{});
	}

	if (representable(groups, uint64_t{})) {
		read_bit_groups(bit_reader{first, last}, groups, last, uint64_t{});
	}
}


template<typename Writer, typename I, typename T>
void write_bit_groups(Writer writer, const std::vector<bit_group>& groups, I last, T /* type tag */) {
	T input{ 0 };
	bit_count count{ 0 };

	for (const auto& group : groups) {
		count = bit_count{ group.first };
		input = static_cast<T>(group.second); // the cast is ok because the input groups were scanned with the function representable()

		writer.write(count, input);
		writer.write(bit_count{ 0 }, input);
	}
	writer.write(bit_count{ 0 }, input);

	REQUIRE(writer.flush() == last);

	// writing one more bit should fail because the whole bit sequence is presumably already consumed
	REQUIRE_THROWS(writer.write(bit_count{ 1 }, input));
}



void test_bit_writer(const std::vector<uint8_t>& expected, const std::vector<bit_group>& groups) {
	std::vector<uint8_t> output;
	output.resize(expected.size());
	std::fill(std::begin(output), std::end(output), uint8_t{ 0 });
	REQUIRE(output != expected); // else the comparisons below would not test anything

	const auto first{ std::begin(output) };
	const auto last{ std::end(output) };

	if (representable(groups, uint8_t{})) {
		write_bit_groups(bit_writer(first, last), groups, last, uint8_t{});
		REQUIRE(output == expected);
	}

	if (representable(groups, uint16_t{})) {
		std::fill(first, last, uint8_t{ 0 }); // makes sure output != expected
		write_bit_groups(bit_writer(first, last), groups, last, uint16_t{});
		REQUIRE(output == expected);
	}

	if (representable(groups, uint32_t{})) {
		std::fill(first, last, uint8_t{ 0 }); // makes sure output != expected
		write_bit_groups(bit_writer(first, last), groups, last, uint32_t{});
		REQUIRE(output == expected);
	}

	if (representable(groups, uint64_t{})) {
		std::fill(first, last, uint8_t{ 0 }); // makes sure output != expected
		write_bit_groups(bit_writer(first, last), groups, last, uint64_t{});
		REQUIRE(output == expected);
	}
}




TEST_CASE("reading and writing of well known data", "[concistency]") {
    using Int = bit_count::value_type;

	// 8 byte (64 bit sequence) consisting only of alternating ones and zeroes
	const std::vector<uint8_t> input{ 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	const auto bits{ static_cast<Int>(size_in_bits(begin(input), end(input), unguarded{})) };

	// reading the sequence in groups of 2 bits:
	// 32 groups of 2 bits each. each group is expected to contain the value b10.
	std::vector<bit_group> groups2;
	for (Int i{ 0 }; i < bits / 2; ++i) {
		groups2.push_back({ 2, 0x2 });
	}
    test_bit_reader(input, groups2);
	test_bit_writer(input, groups2);

	// reading the sequence in groups of 3 bits:
	// 20 groups of 3 bits each. the groups are expected to contain the alternating values b101 and b010.
	// remainder: a group of 4 bits containing the value b1010.
	std::vector<bit_group> groups3;
	for (Int i{ 0 }; i < bits / 6; ++i) {
		groups3.push_back({ 3, 0x5 });
		groups3.push_back({ 3, 0x2 });
	}
	groups3.push_back({ 4, 0xA });
	test_bit_reader(input, groups3);
	test_bit_writer(input, groups3);

	// reading the sequence in groups of 4 bits:
	// 16 groups of 4 bits each. each group is expected to contain the value b1010.
	std::vector<bit_group> groups4;
	for (Int i{ 0 }; i < bits / 4; ++i) {
		groups4.push_back({ 4, 0xA });
	}
	test_bit_reader(input, groups4);
	test_bit_writer(input, groups4);

	// reading the sequence in groups of 5 bits:
	// 12 groups of 5 bits each. the groups are expected to contain the alternating values b10101 and b01010.
	// remainder: a group of 4 bits containing the value b1010.
	std::vector<bit_group> groups5;
	for (Int i{ 0 }; i < bits / 10; ++i) {
		groups5.push_back({ 5, 0x15 });
		groups5.push_back({ 5, 0xA });
	}
	groups5.push_back({ 4, 0xA });
	test_bit_reader(input, groups5);
	test_bit_writer(input, groups5);

	// reading the sequence in groups of 6 bits:
	// 10 groups of 6 bits each. each group is expected to contain the value b101010.
	// remainder: a group of 4 bits containing the value b1010.
	std::vector<bit_group> groups6;
	for (Int i{ 0 }; i < bits / 6; ++i) {
		groups6.push_back({ 6, 0x2A });
	}
	groups6.push_back({ 4, 0xA });
	test_bit_reader(input, groups6);
	test_bit_writer(input, groups6);

	// reading the sequence in groups of 7 bits:
	// 8 groups of 7 bits each. the groups are expected to contain the alternating values b1010101 and b0101010.
	// remainder: a group of 8 bits containing the value b10101010.
	std::vector<bit_group> groups7;
	for (Int i{ 0 }; i < bits / 14; ++i) {
		groups7.push_back({ 7, 0x55 });
		groups7.push_back({ 7, 0x2A });
	}
	groups7.push_back({ 8, 0xAA });
	test_bit_reader(input, groups7);
	test_bit_writer(input, groups7);

	// reading the sequence in groups of 8 bits:
	// every byte consitst of 1 groups of 8 bits, every group should have value b10101010
	std::vector<bit_group> groups8;
	for (Int i{ 0 }; i < bits / 8; ++i) {
		groups8.push_back({ 8, 0xAA });
	}
	test_bit_reader(input, groups8);
	test_bit_writer(input, groups8);

	// reading the sequence in groups of 9 bits:
	// 8 groups of 7 bits each. the groups are expected to contain the alternating values b101010101 and b010101010.
	// remainder: a group of 8 bits containing the value b10101010 and a group of 2 bits containing the value b10.
	std::vector<bit_group> groups9;
	for (Int i{ 0 }; i < bits / 18; ++i) {
		groups9.push_back({ 9, 0x155 });
		groups9.push_back({ 9, 0xAA });
	}
	groups9.push_back({ 8, 0xAA });
	groups9.push_back({ 2, 0x2 });
	test_bit_reader(input, groups9);
	test_bit_writer(input, groups9);

	// reading the sequence in groups of 10 bits:
	// 6 groups of 10 bits each. each group is expected to contain the value b1010101010.
	// remainder: a group of 4 bits containing the value b1010.
	std::vector<bit_group> groups10;
	for (Int i{ 0 }; i < bits / 10; ++i) {
		groups10.push_back({ 10, 0x2AA });
	}
	groups10.push_back({ 4, 0xA });
	test_bit_reader(input, groups10);
	test_bit_writer(input, groups10);

	// reading the sequence in groups of 11 bits:
	// 5 groups of 11 bits each. the groups are expected to contain the alternating values b10101010101 and b01010101010.
	// remainder: a group of 9 bits containing the value b010101010.
	std::vector<bit_group> groups11;
	for (Int i{ 0 }; i < bits / 22; ++i) {
		groups11.push_back({ 11, 0x555 });
		groups11.push_back({ 11, 0x2AA });
	}
	groups11.push_back({ 11, 0x555 });
	groups11.push_back({ 9, 0xAA });
	test_bit_reader(input, groups11);
	test_bit_writer(input, groups11);

	// reading the sequence in groups of 12 bits:
	// 5 groups of 12 bits each. each group is expected to contain the value b101010101010.
	// remainder: a group of 4 bits containing the value b1010.
	std::vector<bit_group> groups12;
	for (Int i{ 0 }; i < bits / 12; ++i) {
		groups12.push_back({ 12, 0xAAA });
	}
	groups12.push_back({ 4, 0xA });
	test_bit_reader(input, groups12);
	test_bit_writer(input, groups12);

	// reading the sequence in groups of 13 bits:
	// 4 groups of 13 bits each. the groups are expected to contain the alternating values b1010101010101 and b0101010101010.
	// remainder: a group of 12 bits containing the value b101010101010.
	std::vector<bit_group> groups13;
	for (Int i{ 0 }; i < bits / 26; ++i) {
		groups13.push_back({ 13, 0x1555 });
		groups13.push_back({ 13, 0xAAA });
	}
	groups13.push_back({ 12, 0xAAA });
	test_bit_reader(input, groups13);
	test_bit_writer(input, groups13);

	// reading the sequence in groups of 14 bits:
	// 4 groups of 14 bits each. each group is expected to contain the value b10101010101010.
	// remainder: a group of 8 bits containing the value b10101010.
	std::vector<bit_group> groups14;
	for (Int i{ 0 }; i < bits / 14; ++i) {
		groups14.push_back({ 14, 0x2AAA });
	}
	groups14.push_back({ 8, 0xAA });
	test_bit_reader(input, groups14);
	test_bit_writer(input, groups14);

	// reading the sequence in groups of 15 bits:
	// 4 groups of 15 bits each. the groups are expected to contain the alternating values b101010101010101 and b010101010101010.
	// remainder: a group of 4 bits containing the value b1010.
	std::vector<bit_group> groups15;
	for (Int i{ 0 }; i < bits / 30; ++i) {
		groups15.push_back({ 15, 0x5555 });
		groups15.push_back({ 15, 0x2AAA });
	}
	groups15.push_back({ 4, 0xA });
	test_bit_reader(input, groups15);
	test_bit_writer(input, groups15);

	// reading the sequence in groups of 16 bits:
	// 4 groups of 16 bits each. each group is expected to contain the value b1010101010101010.
	std::vector<bit_group> groups16;
	for (Int i{ 0 }; i < bits / 16; ++i) {
		groups16.push_back({ 16, 0xAAAA });
	}
	test_bit_reader(input, groups16);
	test_bit_writer(input, groups16);

	// reading the sequence in groups of 17 bits:
	// 3 groups of 17 bits each. the groups are expected to contain the alternating values b10101010101010101 and b01010101010101010.
	// remainder: a group of 13 bits containing the value b0101010101010.
	std::vector<bit_group> groups17;
	for (Int i{ 0 }; i < bits / 34; ++i) {
		groups17.push_back({ 17, 0x15555 });
		groups17.push_back({ 17, 0xAAAA });
	}
	groups17.push_back({ 17, 0x15555 });
	groups17.push_back({ 13, 0xAAA });
	test_bit_reader(input, groups17);
	test_bit_writer(input, groups17);

	// reading the sequence in groups of 18 bits:
	// 3 groups of 18 bits each. each group is expected to contain the value b101010101010101010.
	// remainder: a group of 10 bits containing the value b1010101010.
	std::vector<bit_group> groups18;
	for (Int i{ 0 }; i < bits / 18; ++i) {
		groups18.push_back({ 18, 0x2AAAA });
	}
	groups18.push_back({ 10, 0x2AA });
	test_bit_reader(input, groups18);
	test_bit_writer(input, groups18);

	// reading the sequence in groups of 19 bits:
	// 3 groups of 19 bits each. the groups are expected to contain the alternating values b1010101010101010101 and b0101010101010101010.
	// remainder: a group of 7 bits containing the value b0101010101010.
	std::vector<bit_group> groups19;
	groups19.push_back({ 19, 0x55555 });
	groups19.push_back({ 19, 0x2AAAA });
	groups19.push_back({ 19, 0x55555 });
	groups19.push_back({ 7, 0x2A });
	test_bit_reader(input, groups19);
	test_bit_writer(input, groups19);

	// reading the sequence in groups of 23 bits:
	// 2 groups of 23 bits each. the groups are expected to contain the alternating values b1010101010101010101 and b0101010101010101010.
	// remainder: a group of 7 bits containing the value b0101010101010.
	std::vector<bit_group> groups23;
	groups23.push_back({ 23, 0x555555 });
	groups23.push_back({ 23, 0x2AAAAA });
	groups23.push_back({ 18, 0x2AAAA });
	test_bit_reader(input, groups23);
	test_bit_writer(input, groups23);

	// reading the sequence in groups of 24 bits:
	// 2 groups of 24 bits each. each group is expected to contain the value b101010101010101010101010.
	// remainder: a group of 16 bits containing the value b1010101010101010.
	std::vector<bit_group> groups24;
	groups24.push_back({ 24, 0xAAAAAA });
	groups24.push_back({ 24, 0xAAAAAA });
	groups24.push_back({ 16, 0xAAAA });
	test_bit_reader(input, groups24);
	test_bit_writer(input, groups24);

	// reading the sequence in groups of 25 bits:
	// 2 groups of 25 bits each. each group is expected to contain the value b101010101010101010101010.
	// remainder: a group of 14 bits containing the value b10101010101010.
	std::vector<bit_group> groups25;
	groups25.push_back({ 25, 0x1555555 });
	groups25.push_back({ 25, 0xAAAAAA });
	groups25.push_back({ 14, 0x2AAA });
	test_bit_reader(input, groups25);
	test_bit_writer(input, groups25);

	// reading the sequence in groups of 31 bits:
	// 2 groups of 31 bits each. the groups are expected to contain the alternating values b1010101010101010101010101010101 and b0101010101010101010101010101010.
	// remainder: a group of 2 bits containing the value b10.
	std::vector<bit_group> groups31;
	groups31.push_back({ 31, 0x55555555 });
	groups31.push_back({ 31, 0x2AAAAAAA });
	groups31.push_back({ 2, 0x2 });
	test_bit_reader(input, groups31);
	test_bit_writer(input, groups31);

	// reading the sequence in groups of 32 bits.
	// 2 groups of 32 bits each. each group is expected to contain the value b10101010101010101010101010101010.
	std::vector<bit_group> groups32;
	groups32.push_back({ 32, 0xAAAAAAAA });
	groups32.push_back({ 32, 0xAAAAAAAA });
	test_bit_reader(input, groups32);
	test_bit_writer(input, groups32);

	// reading the sequence in groups of 31 bits:
	// 1 group of 33 bits. the group is expected to contain the value b101010101010101010101010101010101.
	// remainder: a group of 31 bits containing the value b0101010101010101010101010101010.
	std::vector<bit_group> groups33;
	groups33.push_back({ 33, 0x155555555 });
	groups33.push_back({ 31, 0x2AAAAAAA });
	test_bit_reader(input, groups33);
	test_bit_writer(input, groups33);

	// reading the sequence in groups of 64 bits.
	std::vector<bit_group> groups64;
	groups64.push_back({ 64, 0xAAAAAAAAAAAAAAAA });
	test_bit_reader(input, groups64);
	test_bit_writer(input, groups64);
}


TEST_CASE("writer flush test", "[concistency]") {
	std::vector<uint8_t> output;
	output.resize(8); // 8 bytes x 8 bits = 64 bits available for writing
	const auto first{ std::begin(output) };
	const auto last{ std::end(output) };
	const auto range{ size_in_bits(first, last, unguarded{}) + bit_count{ 1 } }; // 65

	const uint64_t word64{ 0xAAAAAAAAAAAAAAAA };

	// 1) after writing 0 bits the function flush() shall return "first" because none of the output bytes were consumed (no bits were written to the output)
	bit_writer writer_zero{ first, last };
	writer_zero.write(bit_count{ 0 }, word64);
	REQUIRE(writer_zero.flush() == first);

	for (bit_count i{ 1 }; i < range; ++i) {
		// after writing from 1 up to 8 bits the first byte (index 0) is consumed (bits were written to it). the next writable byte shall be the second byte (index 1)
		// after writing from 9 up to 16 bits the first two bytes (indices 0 and 1) are consumed (bits were written to them). the next writable byte shall be the third byte (index 2)
		// after writing from 17 up to 24 bits the first three bytes (indices 0, 1 and 2) are consumed (bits were written to them). the next writable byte shall be the fourth byte (index 3)
		// after writing from 25 up to 32 bits the first four bytes (indices 0, 1, 2 and 3) are consumed (bits were written to them). the next writable byte shall be the fifth byte (index 4)
		// ...
		const auto li{ static_cast<bit_count::value_type>((i - bit_count{ 1 }) / one_byte() + bit_count{ 1 }) };
		const ptrdiff_t last_index{ cast(li, ptrdiff_t{}, ok{}) };

		bit_writer writer{ first, last };
		writer.write(i, word64);
		const auto next{ writer.flush() };
		REQUIRE(next == first + last_index);
	}
}


using namespace qcheck;
template<typename T>
using words_bytes = std::tuple<std::vector<T>, std::vector<uint8_t>>;

template<typename T>
struct make_encoded
{
    random_source* _random;

    make_encoded(random_source& r, T)
        : _random{ &r } {
    }

    auto operator()(size_t size) -> words_bytes<T> {
        const auto xs{ gen(size, *_random, std::vector<T>{}) };
        std::vector<bit_count> sizes(xs.size());
        std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

        std::vector<uint8_t> bytes(xs.size() * sizeof(T) * 2);
        std::fill(begin(bytes), end(bytes), uint8_t{ 0 });

        bit_writer writer{ begin(bytes), end(bytes) };
        for (size_t i{ 0 }; i < xs.size(); ++i) {
            writer.write(sizes[i], xs[i]);
        }
        const auto next{ writer.flush() };
        bytes.resize(static_cast<size_t>(std::distance(begin(bytes), next)));

        return std::make_tuple(xs, bytes);
    }
};




template<typename T>
struct property_encoded_size : arguments<std::vector<T>>
{
    explicit property_encoded_size(T/* type tag */) {}

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        std::vector<bit_count> sizes(xs.size());
        std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

        const bit_count bits{ std::accumulate(begin(sizes), end(sizes), bit_count{ 0 }) };
        const byte_count expected{ as_bytes(bits) };

        std::vector<uint8_t> bytes(xs.size() * sizeof(T) * 2);
        bit_writer writer{ begin(bytes), end(bytes) };
        for (size_t i{ 0 }; i < xs.size(); ++i) {
            writer.write(sizes[i], xs[i]);
        }

        const auto d{ std::distance(begin(bytes), writer.flush()) };
        const byte_count written{ static_cast<byte_count::value_type>(d) };

        return written == expected;
    }

    virtual auto print(std::ostream& os, const std::vector<T>& xs) const -> std::ostream& override final {
        return print_vector(os, xs);
    }
};


template<typename T>
struct encode_decode : arguments<std::vector<T>>
{
    explicit encode_decode(T/* type tag */) {}

    virtual auto accepts(const std::vector<T>& xs) const -> bool override final {
        return !xs.empty(); // the bit_reader constructor does not accept empty input
    }

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        const bit_count size{ size_in_bits(T{}) };

        std::vector<uint8_t> bytes(xs.size() * sizeof(T));
        std::fill(begin(bytes), end(bytes), uint8_t{ 0 });
        bit_writer writer{ begin(bytes), end(bytes) };
        for (T x : xs) {
            writer.write(size, x);
        }
        const auto next{ writer.flush() };

        std::vector<T> ys;
        ys.reserve(xs.size());
        bit_reader reader{ begin(bytes), next };
        for (size_t i{ 0 }; i < xs.size(); ++i) {
            ys.push_back(reader.read(size, T{}));
        }

        return ys == xs;
    }

    virtual auto print(std::ostream& os, const std::vector<T>& xs) const -> std::ostream& override final {
        return print_vector(os, xs);
    }

    virtual auto shrink(const std::vector<T>& xs) const -> std::vector<std::vector<T>> override final {
        return shrink_vector(xs);
    }
};

template<typename T>
struct decode_encode : arguments<words_bytes<T>>
{
    explicit decode_encode(T/* type tag */) {}

    virtual auto accepts(const words_bytes<T>& args) const -> bool override final {
        const auto& bytes{ std::get<1>(args) };
        return !bytes.empty();
    }

    virtual auto holds(const words_bytes<T>& args) const -> bool override final {
        const auto& xs{ std::get<0>(args) };
        const auto& bytes{ std::get<1>(args) };

        std::vector<bit_count> sizes(xs.size());
        std::transform(begin(xs), end(xs), begin(sizes), count_raw3{});

        std::vector<T> ys;
        ys.reserve(xs.size());
        bit_reader reader{ begin(bytes), end(bytes) };
        for (bit_count n : sizes) {
            ys.push_back(reader.read(n, T{}));
        }

        if (reader.current() != end(bytes)) {
            return false;
        }

        std::vector<uint8_t> encoded(ys.size() * sizeof(T) * 2);
        std::fill(begin(encoded), end(encoded), uint8_t{ 0 });
        bit_writer writer{ begin(encoded), end(encoded) };
        for (size_t i{ 0 }; i < ys.size(); ++i) {
            writer.write(sizes[i], ys[i]);
        }
        const auto next{ writer.flush() };
        encoded.resize(static_cast<size_t>(std::distance(begin(encoded), next)));

        return encoded == bytes;
    }

    virtual auto print(std::ostream& os, const words_bytes<T>& args) const -> std::ostream& override final {
        const auto& bytes{ std::get<1>(args) };
        return print_vector(os, bytes);
    }
};



TEST_CASE("qcheck", "[concistency]") {
    random_source r;
    //random_source r{ 1368344820 };

    REQUIRE(check("encoding size 8 bit",  property_encoded_size{ uint8_t{} },  make_vectors{ r, uint8_t{} }));
    REQUIRE(check("encoding size 16 bit", property_encoded_size{ uint16_t{} }, make_vectors{ r, uint16_t{} }));
    REQUIRE(check("encoding size 32 bit", property_encoded_size{ uint32_t{} }, make_vectors{ r, uint32_t{} }));
    REQUIRE(check("encoding size 64 bit", property_encoded_size{ uint64_t{} }, make_vectors{ r, uint64_t{} }));

    REQUIRE(check("enc/dec 8 bit",  encode_decode{ uint8_t{} },  make_vectors{ r, uint8_t{} }));
    REQUIRE(check("enc/dec 16 bit", encode_decode{ uint16_t{} }, make_vectors{ r, uint16_t{} }));
    REQUIRE(check("enc/dec 32 bit", encode_decode{ uint32_t{} }, make_vectors{ r, uint32_t{} }));
    REQUIRE(check("enc/dec 64 bit", encode_decode{ uint64_t{} }, make_vectors{ r, uint64_t{} }));

    REQUIRE(check("dec/enc 8 bit",  decode_encode{ uint8_t{} },  make_encoded{ r, uint8_t{} }));
    REQUIRE(check("dec/enc 16 bit", decode_encode{ uint16_t{} }, make_encoded{ r, uint16_t{} }));
    REQUIRE(check("dec/enc 32 bit", decode_encode{ uint32_t{} }, make_encoded{ r, uint32_t{} }));
    REQUIRE(check("dec/enc 64 bit", decode_encode{ uint64_t{} }, make_encoded{ r, uint64_t{} }));
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
