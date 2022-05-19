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

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <tuple>

#include "compress/matrix.h"
#include "make_block.h"
#include "qcheck.h"


using namespace ctk::impl;
using namespace qcheck;


template<typename T, typename Format>
auto make_reduction(const std::vector<T>& xs, Format) -> reduction<T> {
    reduction<T> x;
    estimation<T> scratch;

    const measurement_count samples{ vsize(xs) };
    scratch.resize(samples, Format{});
    x.resize(samples);
    x.residuals = xs;
    x.method = encoding_method::time;
    compressed_parameters(x, scratch, Format{});

    const auto max_bytes{ max_block_size(begin(xs), end(xs), Format{}) };

    if (max_bytes < x.output_size) {
        x.data_size = Format::as_size(T{});
        x.method = encoding_method::copy;
        x.n = size_in_bits(T{});
        x.nexc = x.n;
        x.output_size = max_bytes;
    }
    else {
        if (x.n != x.nexc) {
            build_encoding_map(x);
        }
    }

    return x;
}


template<typename T, typename Format>
struct encode_decode : arguments<std::vector<T>>
{
    explicit encode_decode(T/* type tag */, Format/* type tag */) {}

    virtual auto accepts(const std::vector<T>& xs) const -> bool override final {
        return !xs.empty();
    }

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        const auto r{ make_reduction(xs, Format{}) };
        auto bytes{ make_bytes(xs) };
        std::vector<T> ys(xs.size());

        bit_writer writer{ begin(bytes), end(bytes) };
        auto next{ encode_block(begin(xs), end(xs), begin(r.encoding_map), writer, r.data_size, r.method, r.n, r.nexc, Format{}) };

        bit_reader reader{ begin(bytes), next };
        auto [last, method]{ decode_block(reader, begin(ys), end(ys), Format{}) };
        byte_count consumed{ std::distance(begin(bytes), last) };

        return xs == ys && r.output_size == consumed && r.method == method;
    }

    virtual auto print(std::ostream& os, const std::vector<T>& xs) const -> std::ostream& override final {
        return print_vector(os, xs);
    }
};


using bytes_uncompressed = std::tuple<std::vector<uint8_t>, std::vector<bool>, encoding_size, bit_count, bit_count, size_t>;

template<typename T, typename Format>
struct decode_encode : arguments<bytes_uncompressed>
{
    explicit decode_encode(T/* type tag */, Format/* type tag */) {}

    virtual auto accepts(const bytes_uncompressed& x) const -> bool override final {
        const auto uncompressed_size{ std::get<5>(x) };
        return 0 < uncompressed_size;
    }

    virtual auto holds(const bytes_uncompressed& x) const -> bool override final {
        const auto& bytes{ std::get<0>(x) };
        const auto& encoding_map{ std::get<1>(x) };
        const auto data_size{ std::get<2>(x) };
        const auto n{ std::get<3>(x) };
        const auto nexc{ std::get<4>(x) };
        const auto uncompressed_size{ std::get<5>(x) };

        std::vector<T> xs(uncompressed_size);
        bit_reader reader{ begin(bytes), end(bytes) };
        const auto [last_decoded, method]{ decode_block(reader, begin(xs), end(xs), Format{}) };

        auto encoded{ make_bytes(xs) };
        bit_writer writer{ begin(encoded), end(encoded) };
        const auto last_encoded{ encode_block(begin(xs), end(xs), begin(encoding_map), writer, data_size, method, n, nexc, Format{}) };
        const auto encoded_size{ std::distance(begin(encoded), last_encoded) };
        encoded.resize(static_cast<size_t>(encoded_size));

        return last_decoded == end(bytes) && bytes == encoded;
    }

    virtual auto classify(const bytes_uncompressed& x) const -> std::string override final {
        const auto& bytes{ std::get<0>(x) };
        bit_reader reader{ begin(bytes), end(bytes) };
        std::vector<T> output(1);
        const auto [onext, n, nexc, method]{ read_header(begin(output), end(output), reader, Format{}) };

        std::ostringstream oss;
        oss << std::setw(5) << method;
        if (method != encoding_method::copy) {
            if (n == nexc) {
                oss << " fixed width";
            }
            else {
                oss << " variable width";
            }
        }
        return oss.str();
    }

    virtual auto print(std::ostream& os, const bytes_uncompressed& x) const -> std::ostream& override final {
        const auto& bytes{ std::get<0>(x) };
        const auto& encoding_map{ std::get<1>(x) };
        const auto data_size{ std::get<2>(x) };
        const auto n{ std::get<3>(x) };
        const auto nexc{ std::get<4>(x) };
        const auto uncompressed_size{ std::get<5>(x) };

        os << "encoding size " << data_size << ", n " << n << ", nexc " << nexc << ", uncompressed amount " << uncompressed_size;
        os << "\nbytes ";
        print_vector(os, bytes);
        os << "\nencoding map ";
        print_vector(os, encoding_map);
        os << "\n";
        return os;
    }
};


template<typename T, typename Format>
struct make_encoded
{
    random_source* _random;

    make_encoded(random_source& r, T, Format)
        : _random{ &r } {
    }

    auto operator()(size_t size) -> bytes_uncompressed {
        return generate_encoded_block(size, *_random, T{}, Format{});
    }
};


auto main(int, char*[]) -> int {
    random_source r;
    //random_source r{ 1421005479 };

    if (!check("enc/dec reflib, single, 32 bit", encode_decode{ uint32_t{}, reflib{} }, make_vectors{ r, uint32_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("enc/dec extended, single, 8 bit", encode_decode{ uint8_t{}, extended{} }, make_vectors{ r, uint8_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("enc/dec extended, single, 16 bit", encode_decode{ uint16_t{}, extended{} }, make_vectors{ r, uint16_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("enc/dec extended, single, 32 bit", encode_decode{ uint32_t{}, extended{} }, make_vectors{ r, uint32_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("enc/dec extended, single, 64 bit", encode_decode{ uint64_t{}, extended{} }, make_vectors{ r, uint64_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("dec/enc reflib, single, 32 bit", decode_encode{ uint32_t{}, reflib{} }, make_encoded{ r, uint32_t{}, reflib{} })) {
        return EXIT_FAILURE;
    }

    if (!check("dec/enc extended, single, 8 bit", decode_encode{ uint8_t{}, extended{} }, make_encoded{ r, uint8_t{}, extended{} })) {
        return EXIT_FAILURE;
    }

    if (!check("dec/enc extended, single, 16 bit", decode_encode{ uint16_t{}, extended{} }, make_encoded{ r, uint16_t{}, extended{} })) {
        return EXIT_FAILURE;
    }

    if (!check("dec/enc extended, single, 32 bit", decode_encode{ uint32_t{}, extended{} }, make_encoded{ r, uint32_t{}, extended{} })) {
        return EXIT_FAILURE;
    }

    if (!check("dec/enc extended, single, 64 bit", decode_encode{ uint64_t{}, extended{} }, make_encoded{ r, uint64_t{}, extended{} })) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

