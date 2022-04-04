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

    using namespace ctk::api::v1;


TEST_CASE("matrix dimensions", "[correct]") {
    using T = CompressReflib::value_type;

    std::vector<int16_t> order;
    std::vector<T> input;
    std::vector<uint8_t> empty;
    std::vector<uint8_t> bytes;

    // the empty input handling is inconsistent because i can not decide how to proceed.
    // encoding the empty input might have either of these valid outcomes:
    //  - empty output
    //  - valid block header followed by zero values
    //    (method copy, data size 4 bytes, n 2 bits, nexc 2 bits) is a good single byte candidate.

    // 1) encoder
    CompressReflib encode;

    REQUIRE_THROWS(encode.Sensors(-1));
    REQUIRE_THROWS(encode.Sensors(std::numeric_limits<int64_t>::max()));
    REQUIRE(encode.Sensors(0));
    REQUIRE(encode.Sensors(order));

    // 1.1) empty input, no sensors: something out of nothing

    // negative
    REQUIRE(encode.ColumnMajor(input, -1).empty());
    REQUIRE(encode.RowMajor(input, -1).empty());

    // nothing out of nothing
    REQUIRE(encode.ColumnMajor(input, 0).empty());
    REQUIRE(encode.RowMajor(input, 0).empty());

    // too much
    REQUIRE(encode.ColumnMajor(input, 1).empty());
    REQUIRE(encode.RowMajor(input, 1).empty());
    REQUIRE(encode.ColumnMajor(input, std::numeric_limits<int64_t>::max()).empty());
    REQUIRE(encode.RowMajor(input, std::numeric_limits<int64_t>::max()).empty());


    // 1.2) input, no sensors: something out of nothing
    input.push_back(1024);

    // negative
    REQUIRE_THROWS(encode.ColumnMajor(input, -1));
    REQUIRE_THROWS(encode.RowMajor(input, -1));

    // nothing out of nothing
    REQUIRE_THROWS(encode.ColumnMajor(input, 0));
    REQUIRE_THROWS(encode.RowMajor(input, 0));

    // too much
    REQUIRE_THROWS(encode.ColumnMajor(input, 1));
    REQUIRE_THROWS(encode.RowMajor(input, 1));
    REQUIRE_THROWS(encode.ColumnMajor(input, std::numeric_limits<int64_t>::max()));
    REQUIRE_THROWS(encode.RowMajor(input, std::numeric_limits<int64_t>::max()));

    // 1.3) empty input, sensors: something out of nothing
    input.clear();
    REQUIRE(encode.Sensors(1));

    // negative
    REQUIRE(encode.ColumnMajor(input, -1).empty());
    REQUIRE(encode.RowMajor(input, -1).empty());

    // nothing out of nothing
    REQUIRE(encode.ColumnMajor(input, 0).empty());
    REQUIRE(encode.RowMajor(input, 0).empty());

    // too much
    REQUIRE(encode.ColumnMajor(input, 1).empty());
    REQUIRE(encode.RowMajor(input, 1).empty());
    REQUIRE(encode.ColumnMajor(input, std::numeric_limits<int64_t>::max()).empty());
    REQUIRE(encode.RowMajor(input, std::numeric_limits<int64_t>::max()).empty());


    // 1.4) input, sensors: something out of something
    REQUIRE(encode.Sensors(1));
    input.push_back(1024);

    // negative
    REQUIRE_THROWS(encode.ColumnMajor(input, -1));
    REQUIRE_THROWS(encode.RowMajor(input, -1));

    // nothing out of something
    REQUIRE_THROWS(encode.ColumnMajor(input, 0));
    REQUIRE_THROWS(encode.RowMajor(input, 0));

    // something out of something
    REQUIRE(encode.ColumnMajor(input, 1).empty() == false);
    REQUIRE(encode.RowMajor(input, 1).empty() == false);

    bytes = encode.ColumnMajor(input, 1);

    // too much
    REQUIRE_THROWS(encode.ColumnMajor(input, 2));
    REQUIRE_THROWS(encode.RowMajor(input, 2));


    // 2) decoder
    DecompressReflib decode;

    REQUIRE_THROWS(decode.Sensors(-1));
    REQUIRE_THROWS(decode.Sensors(std::numeric_limits<int64_t>::max()));
    REQUIRE(decode.Sensors(0));
    REQUIRE(decode.Sensors(order));

    // 2.1) empty input, no sensors: something out of nothing

    // negative
    REQUIRE(decode.ColumnMajor(empty, -1).empty());
    REQUIRE(decode.RowMajor(empty, -1).empty());

    // nothing out of nothing
    REQUIRE(decode.ColumnMajor(empty, 0).empty());
    REQUIRE(decode.RowMajor(empty, 0).empty());

    // too much
    REQUIRE(decode.ColumnMajor(empty, 1).empty());
    REQUIRE(decode.RowMajor(empty, 1).empty());
    REQUIRE(decode.ColumnMajor(empty, std::numeric_limits<int64_t>::max()).empty());
    REQUIRE(decode.RowMajor(empty, std::numeric_limits<int64_t>::max()).empty());

    // 2.2) input, no sensors: something out of nothing

    // negative
    REQUIRE_THROWS(decode.ColumnMajor(bytes, -1));
    REQUIRE_THROWS(decode.RowMajor(bytes, -1));

    // nothing out of nothing
    REQUIRE_THROWS(decode.ColumnMajor(bytes, 0));
    REQUIRE_THROWS(decode.RowMajor(bytes, 0));

    // too much
    REQUIRE_THROWS(decode.ColumnMajor(bytes, 1));
    REQUIRE_THROWS(decode.RowMajor(bytes, 1));
    REQUIRE_THROWS(decode.ColumnMajor(bytes, std::numeric_limits<int64_t>::max()));
    REQUIRE_THROWS(decode.RowMajor(bytes, std::numeric_limits<int64_t>::max()));

    // 2.3) empty input, sensors: something out of nothing
    REQUIRE(decode.Sensors(1));

    // negative
    REQUIRE(decode.ColumnMajor(empty, -1).empty());
    REQUIRE(decode.RowMajor(empty, -1).empty());

    // nothing out of nothing
    REQUIRE(decode.ColumnMajor(empty, 0).empty());
    REQUIRE(decode.RowMajor(empty, 0).empty());

    // too much
    REQUIRE(decode.ColumnMajor(empty, 1).empty());
    REQUIRE(decode.RowMajor(empty, 1).empty());
    REQUIRE(decode.ColumnMajor(empty, std::numeric_limits<int64_t>::max()).empty());
    REQUIRE(decode.RowMajor(empty, std::numeric_limits<int64_t>::max()).empty());

    // 2.4) input, sensors: something out of something

    // negative
    REQUIRE_THROWS(decode.ColumnMajor(bytes, -1));
    REQUIRE_THROWS(decode.RowMajor(bytes, -1));

    // nothing out of something
    REQUIRE_THROWS(decode.ColumnMajor(bytes, 0));
    REQUIRE_THROWS(decode.RowMajor(bytes, 0));

    // something out of something
    REQUIRE(decode.ColumnMajor(bytes, 1).empty() == false);
    REQUIRE(decode.RowMajor(bytes, 1).empty() == false);
    REQUIRE(decode.ColumnMajor(bytes, 1)[0] == input[0]);
    REQUIRE(decode.RowMajor(bytes, 1)[0] == input[0]);

    // too much
    REQUIRE_THROWS(decode.ColumnMajor(bytes, 2));
    REQUIRE_THROWS(decode.RowMajor(bytes, 2));
}


TEST_CASE("well known input", "[consistent]") {
    // 6 data points
    std::vector<int32_t> input{ 0, 1, 2, 3, 4, 5 };
    std::vector<int32_t> output;
    std::vector<uint8_t> bytes;

    CompressReflib encode;
    DecompressReflib decode;

    // 2 electrodes
    REQUIRE(encode.Sensors(2));
    REQUIRE(decode.Sensors(2));

    REQUIRE_THROWS(encode.ColumnMajor(input, -1));
    REQUIRE_THROWS(encode.RowMajor(input, -1));
    REQUIRE_THROWS(encode.ColumnMajor(input, 0));
    REQUIRE_THROWS(encode.RowMajor(input, 0));
    REQUIRE_THROWS(encode.ColumnMajor(input, 1));
    REQUIRE_THROWS(encode.RowMajor(input, 1));
    REQUIRE_THROWS(encode.ColumnMajor(input, 2));
    REQUIRE_THROWS(encode.RowMajor(input, 2));

    // 6 data points = 2 electrodes x 3 samples
    bytes = encode.ColumnMajor(input, 3);
    REQUIRE(!bytes.empty());

    output = decode.ColumnMajor(bytes, 3);
    REQUIRE(output == input);

    output = decode.RowMajor(bytes, 3);
    REQUIRE(output != input);

    bytes = encode.RowMajor(input, 3);
    REQUIRE(!bytes.empty());

    output = decode.RowMajor(bytes, 3);
    REQUIRE(output == input);

    output = decode.ColumnMajor(bytes, 3);
    REQUIRE(output != input);

    REQUIRE_THROWS(encode.ColumnMajor(input, 4));
    REQUIRE_THROWS(encode.RowMajor(input, 4));
    REQUIRE_THROWS(encode.ColumnMajor(input, 5));
    REQUIRE_THROWS(encode.RowMajor(input, 5));
    REQUIRE_THROWS(encode.RowMajor(input, 6));
    REQUIRE_THROWS(encode.ColumnMajor(input, 6));
}


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
                REQUIRE(encode.Sensors(height));
                REQUIRE(decode.Sensors(height));

                bytes = encode.ColumnMajor(input, length);
                REQUIRE(!bytes.empty());
                output = decode.ColumnMajor(bytes, length);
                REQUIRE(!output.empty());
                REQUIRE(input == output);

                bytes = encode.RowMajor(input, length);
                REQUIRE(!bytes.empty());
                output = decode.RowMajor(bytes, length);
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
    catch (const ctk::api::v1::CtkLimit& e) {
        std::cerr << ": " << e.what();
    }

    std::cerr << "\n";
}

TEST_CASE("oo interface encode/decode", "[consistency]") {
    using namespace ctk::api::v1;

	random_values random;

	for (ptrdiff_t input_size : { 1, 256, 512 }) {
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
