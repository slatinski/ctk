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

#include "ctk/api_compression.h"
#include "ctk/exception.h"
#include "test/util.h"

namespace ctk { namespace impl { namespace test {

template<typename T, typename Encoder, typename Decoder>
void encode_decode(T, Encoder encode, Decoder decode, ptrdiff_t input_size, random_values& random) {
    std::vector<uint8_t> bytes;
	std::vector<T> input;
	std::vector<T> output;
    input.resize(size_t(input_size));

    try {
        for (T factor : { T{ 1 }, T{ 2 }, T{ 4 }, T{ 8 } }) {
            const T partial_range{ static_cast<T>(std::numeric_limits<T>::max() / factor) };
            T low{ 0 };
            if (std::is_signed<T>::value) {
                low = T(0 - partial_range);
            }
            T high{ partial_range };
            random.fill(low, high, input);

            const auto row_lengths = divisors(input_size);

            for (auto length : row_lengths) {
                const auto height = input_size / length;
                REQUIRE(encode.sensors(height));
                REQUIRE(decode.sensors(height));

                bytes = encode.column_major(input, length);
                REQUIRE(!bytes.empty());
                output = decode.column_major(bytes, length);
                REQUIRE(!output.empty());
                REQUIRE(input == output);

                bytes = encode.row_major(input, length);
                REQUIRE(!bytes.empty());
                output = decode.row_major(bytes, length);
                REQUIRE(!output.empty());
                REQUIRE(input == output);
            }
        }
    }
    catch (const std::bad_alloc& e) {
        std::cerr << ": " << e.what();
    }
    catch (const std::length_error& e) {
        std::cerr << ": " << e.what();
    }
    catch (const ctk::api::v1::ctk_limit& e) {
        std::cerr << ": " << e.what();
    }

    std::cerr << "\n";
}

TEST_CASE("oo interface encode/decode", "[consistency]") {
    using namespace ctk::api::v1;

	random_values random;

	for (ptrdiff_t input_size : { 1, 128, 256, 1024, 2048, 4096 }) {
        std::cout << "input size " << input_size << "\n";

        // reference library format: 4 byte wide signed words
		std::cout << "reflib : int32_t";
		encode_decode(int32_t{}, CompressReflib{}, DecompressReflib{}, input_size, random);

        // extended format: 2, 4 or 8 byte wide signed/unsigned words
		std::cout << "extended: int16_t";
		encode_decode(int16_t{}, CompressInt16{}, DecompressInt16{}, input_size, random);

		std::cout << "extended: int32_t";
		encode_decode(int32_t{}, CompressInt32{}, DecompressInt32{}, input_size, random);

		std::cout << "extended: int64_t";
		encode_decode(int64_t{}, CompressInt64{}, DecompressInt64{}, input_size, random);

		std::cout << "extended: uint16_t";
		encode_decode(uint16_t{}, CompressUInt16{}, DecompressUInt16{}, input_size, random);

		std::cout << "extended: uint32_t";
		encode_decode(uint32_t{}, CompressUInt32{}, DecompressUInt32{}, input_size, random);

		std::cout << "extended: uint64_t";
		encode_decode(uint64_t{}, CompressUInt64{}, DecompressUInt64{}, input_size, random);
	}
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
