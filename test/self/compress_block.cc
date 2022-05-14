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

auto print_class(encoding_method method, bit_count n, bit_count nexc) -> std::string {
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


template<typename T, typename Format>
auto encode_decode_block(const std::vector<T>& xs, Format format) -> bool {
    const auto r{ make_reduction(xs, format) };
    auto bytes{ make_bytes(xs) };
    std::vector<T> ys(xs.size());

    bit_writer writer{ begin(bytes), end(bytes) };
    auto next{ encode_block(begin(xs), end(xs), begin(r.encoding_map), writer, r.data_size, r.method, r.n, r.nexc, format) };

    bit_reader reader{ begin(bytes), next };
    auto [last, method]{ decode_block(reader, begin(ys), end(ys), format) };
    byte_count consumed{ std::distance(begin(bytes), last) };

    return xs == ys && r.output_size == consumed && r.method == method;
}

template<typename IByte, typename T, typename Format>
auto decode_encode_block(IByte first, IByte last, const std::vector<size_t>& uncompressed_sizes, T, Format format) -> bool {
    const size_t uncompressed_size{ std::accumulate(begin(uncompressed_sizes), end(uncompressed_sizes), size_t{ 0 }) };

    // decodes the compressed byte stream
    std::vector<T> decoded_x(uncompressed_size);
    auto o_first{ begin(decoded_x) };

    bit_reader reader_x{ first, last };
    for (size_t size : uncompressed_sizes) {
        const auto o_last{ o_first + static_cast<ptrdiff_t>(size) };
        decode_block(reader_x, o_first, o_last, format);
        o_first = o_last;
    }

    // encodes the decoded sequence
    auto bytes{ make_bytes(decoded_x) };
    auto next{ begin(bytes) };
    bit_writer writer{ begin(bytes), end(bytes) };

    auto i_first{ begin(decoded_x) };
    for (size_t size : uncompressed_sizes) {
        const auto i_last{ i_first + static_cast<ptrdiff_t>(size) };
        const std::vector<T> v{ i_first, i_last };
        const auto r{ make_reduction(v, format) };
        next = encode_block(i_first, i_last, begin(r.encoding_map), writer, r.data_size, r.method, r.n, r.nexc, format);
        i_first = i_last;
    }

    // the encoder almost certainly picked different parameters meaning
    // that the compressed streams can not be compared verbatim.
    // for this reason another decoding step is performed and the decoded sequences are compared instead.
    std::vector<T> decoded_y(uncompressed_size);
    o_first = begin(decoded_y);
    bit_reader reader_y{ begin(bytes), next };
    for (size_t size : uncompressed_sizes) {
        const auto o_last{ o_first + static_cast<ptrdiff_t>(size) };
        decode_block(reader_y, o_first, o_last, format);
        o_first = o_last;
    }

    return decoded_x == decoded_y;
}

template<typename T, typename Format>
struct encode_decode_single : arguments<std::vector<T>>
{
    virtual auto accepts(const std::vector<T>& xs) const -> bool override final {
        return !xs.empty();
    }

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        return encode_decode_block(xs, Format{});
    }

    virtual auto classify(const std::vector<T>& xs) const -> std::string override final {
        const auto r{ make_reduction(xs, Format{}) };

        return print_class(r.method, r.n, r.nexc);
    }

    virtual auto print(std::ostream& os, const std::vector<T>& xs) const -> std::ostream& override final {
        return print_vector(os, xs);
    }
};


using bytes_uncompressed = std::tuple<std::vector<uint8_t>, size_t>;

auto not_empty(const bytes_uncompressed& x) -> bool {
    const auto& bytes{ std::get<0>(x) };
    const auto uncompressed_size{ std::get<1>(x) };

    return !bytes.empty() && 0 < uncompressed_size;
}

template<typename T, typename Format>
struct decode_encode_single : arguments<bytes_uncompressed>
{
    virtual auto accepts(const bytes_uncompressed& x) const -> bool override final {
        return not_empty(x);
    }

    virtual auto holds(const bytes_uncompressed& x) const -> bool override final {
        const auto& bytes{ std::get<0>(x) };
        const auto uncompressed_size{ std::get<1>(x) };
        return decode_encode_block(begin(bytes), end(bytes), { uncompressed_size }, T{}, Format{});
    }

    virtual auto classify(const bytes_uncompressed& x) const -> std::string override final {
        const auto& bytes{ std::get<0>(x) };
        bit_reader reader{ begin(bytes), end(bytes) };
        std::vector<T> output(1);
        const auto [onext, n, nexc, method]{ read_header(begin(output), end(output), reader, Format{}) };

        return print_class(method, n, nexc);
    }

    virtual auto print(std::ostream& os, const bytes_uncompressed& x) const -> std::ostream& override final {
        os << "bytes ";
        print_vector(os, std::get<0>(x));
        os << ", uncompressed size ";
        os << std::get<1>(x);
        return os;
    }
};

template<typename T, typename Format>
struct decode_encode_multiple : arguments<std::vector<bytes_uncompressed>>
{
    virtual auto accepts(const std::vector<bytes_uncompressed>& xs) const -> bool override final {
        if (xs.empty()) {
            return false;
        }

        return std::all_of(begin(xs), end(xs), not_empty);
    }

    virtual auto holds(const std::vector<bytes_uncompressed>& xs) const -> bool override final {
        std::vector<uint8_t> bytes;
        std::vector<size_t> uncompressed_sizes;
        uncompressed_sizes.reserve(xs.size());

        for (const auto& x : xs) {
            const auto& b{ std::get<0>(x) };
            const auto s{ std::get<1>(x) };

            bytes.insert(end(bytes), begin(b), end(b));
            uncompressed_sizes.push_back(s);
        }

        return decode_encode_block(begin(bytes), end(bytes), uncompressed_sizes, T{}, Format{});
    }

    virtual auto classify(const std::vector<bytes_uncompressed>& xs) const -> std::string override final {
        std::array<bool, static_cast<unsigned>(encoding_method::length)> seen;
        std::fill(begin(seen), end(seen), false);

        for (const auto& x : xs) {
            const auto& bytes{ std::get<0>(x) };
            bit_reader reader{ begin(bytes), end(bytes) };
            std::vector<T> output(1);
            const auto [onext, n, nexc, method]{ read_header(begin(output), end(output), reader, Format{}) };
            seen[static_cast<unsigned>(method)] = true;
        }

        std::ostringstream oss;
        if (seen[0]) { oss << encoding_method::copy << " "; };
        if (seen[1]) { oss << encoding_method::time << " "; };
        if (seen[2]) { oss << encoding_method::time2 << " "; };
        if (seen[3]) { oss << encoding_method::chan << " "; };
        return oss.str();
    }

    virtual auto print(std::ostream& os, const std::vector<bytes_uncompressed>& xs) const -> std::ostream& override final {
        for (const auto& x : xs) {
            os << "bytes ";
            print_vector(os, std::get<0>(x));
            os << ", uncompressed size ";
            os << std::get<1>(x) << "\n";
        }
        return os;
    }
};



template<typename T, typename Format>
struct make_dec_enc_single
{
    random_source* _random;

    make_dec_enc_single(random_source& r, T, Format)
        : _random{ &r } {
    }

    auto operator()(size_t size) -> bytes_uncompressed {
        return generate_block(size, *_random, T{}, Format{});
    }
};

template<typename T, typename Format>
struct make_dec_enc_multiple
{
    random_source* _random;

    make_dec_enc_multiple(random_source& r, T, Format)
        : _random{ &r } {
    }

    auto operator()(size_t size) -> std::vector<bytes_uncompressed> {
        std::vector<bytes_uncompressed> xs;
        xs.reserve(size);

        for (size_t i{ 0 }; i < size; ++i) {
            xs.push_back(generate_block(size, *_random, T{}, Format{}));
        }
        return xs;
    }
};


auto main(int, char*[]) -> int {
    using single_enc_dec_32ref = encode_decode_single<uint32_t, reflib>;
    using single_enc_dec_8ext = encode_decode_single<uint8_t, extended>;
    using single_enc_dec_16ext = encode_decode_single<uint16_t, extended>;
    using single_enc_dec_32ext = encode_decode_single<uint32_t, extended>;
    using single_enc_dec_64ext = encode_decode_single<uint64_t, extended>;

    using single_dec_enc_32ref = decode_encode_single<uint32_t, reflib>;
    using single_dec_enc_8ext = decode_encode_single<uint8_t, extended>;
    using single_dec_enc_16ext = decode_encode_single<uint16_t, extended>;
    using single_dec_enc_32ext = decode_encode_single<uint32_t, extended>;
    using single_dec_enc_64ext = decode_encode_single<uint64_t, extended>;

    using multiple_dec_enc_32ref = decode_encode_multiple<uint32_t, reflib>;
    using multiple_dec_enc_8ext = decode_encode_multiple<uint8_t, extended>;
    using multiple_dec_enc_16ext = decode_encode_multiple<uint16_t, extended>;
    using multiple_dec_enc_32ext = decode_encode_multiple<uint32_t, extended>;
    using multiple_dec_enc_64ext = decode_encode_multiple<uint64_t, extended>;

    //random_source r{ 1421005479 };
    random_source r;
    bool ok{ true };

    ok &= check("enc/dec reflib, signle, 32 bit",   single_enc_dec_32ref{}, make_vectors{ r, uint32_t{} });
    ok &= check("enc/dec extended, single, 8 bit",  single_enc_dec_8ext{},  make_vectors{ r, uint8_t{} });
    ok &= check("enc/dec extended, single, 16 bit", single_enc_dec_16ext{}, make_vectors{ r, uint16_t{} });
    ok &= check("enc/dec extended, single, 32 bit", single_enc_dec_32ext{}, make_vectors{ r, uint32_t{} });
    ok &= check("enc/dec extended, single, 64 bit", single_enc_dec_64ext{}, make_vectors{ r, uint64_t{} });

    ok &= check("dec/enc reflib, single, 32 bit",   single_dec_enc_32ref{}, make_dec_enc_single{ r, uint32_t{}, reflib{} });
    ok &= check("dec/enc extended, single, 8 bit",  single_dec_enc_8ext{},  make_dec_enc_single{ r, uint8_t{},  extended{} });
    ok &= check("dec/enc extended, single, 16 bit", single_dec_enc_16ext{}, make_dec_enc_single{ r, uint16_t{}, extended{} });
    ok &= check("dec/enc extended, single, 32 bit", single_dec_enc_32ext{}, make_dec_enc_single{ r, uint32_t{}, extended{} });
    ok &= check("dec/enc extended, single, 64 bit", single_dec_enc_64ext{}, make_dec_enc_single{ r, uint64_t{}, extended{} });

    ok &= check("dec/enc reflib, multiple, 32 bit",   multiple_dec_enc_32ref{}, make_dec_enc_multiple{ r, uint32_t{}, reflib{} });
    ok &= check("dec/enc extended, multiple, 8 bit",  multiple_dec_enc_8ext{},  make_dec_enc_multiple{ r, uint8_t{},  extended{} });
    ok &= check("dec/enc extended, multiple, 16 bit", multiple_dec_enc_16ext{}, make_dec_enc_multiple{ r, uint16_t{}, extended{} });
    ok &= check("dec/enc extended, multiple, 32 bit", multiple_dec_enc_32ext{}, make_dec_enc_multiple{ r, uint32_t{}, extended{} });
    ok &= check("dec/enc extended, multiple, 64 bit", multiple_dec_enc_64ext{}, make_dec_enc_multiple{ r, uint64_t{}, extended{} });

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

