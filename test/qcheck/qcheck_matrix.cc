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

#include <algorithm>

#include "compress/matrix.h"
#include "qcheck.h"
#include "make_block.h"


using namespace ctk::impl;
using namespace qcheck;


template<typename T>
using matrix_tuple = std::tuple<std::vector<T>, sensor_count, measurement_count>;


template<typename T, typename Format>
auto print_class(const matrix_tuple<uint8_t>& args, T, Format) -> std::string {
    const auto& bytes{ std::get<0>(args) };
    const auto& electrodes{ std::get<1>(args) };
    const auto& samples{ std::get<2>(args) };

    std::array<bool, static_cast<unsigned>(encoding_method::length)> seen;
    std::fill(begin(seen), end(seen), false);

    bit_reader reader{ begin(bytes), end(bytes) };
    std::vector<T> xs(static_cast<size_t>(static_cast<measurement_count::value_type>(samples)));
    for (sensor_count i{ 0 }; i < electrodes; ++i) {
        const auto [next, method]{ decode_block(reader, begin(xs), end(xs), Format{}) };
        seen[static_cast<unsigned>(method)] = true;
    }

    std::ostringstream oss;
    if (seen[0]) { oss << encoding_method::copy << " "; };
    if (seen[1]) { oss << encoding_method::time << " "; };
    if (seen[2]) { oss << encoding_method::time2 << " "; };
    if (seen[3]) { oss << encoding_method::chan << " "; };

    return oss.str();
}


template<typename T>
struct make_uncompressed
{
    random_source* _random;

    make_uncompressed(random_source& r, T)
        : _random{ &r } {
    }


    auto operator()(size_t size) const -> matrix_tuple<T> {
        constexpr const size_t min_x{ 0 };
        const auto elc{ choose(min_x, size, *_random) };
        const auto smpl{ choose(min_x, size, *_random) };

        const auto area{ static_cast<size_t>(elc * smpl) };
        const auto xs{ gen(area, *_random, std::vector<T>{}) };

        const auto electrodes{ cast(elc, sensor_count::value_type{}, ok{}) };
        const auto samples{ cast(smpl, measurement_count::value_type{}, ok{}) };
        return std::make_tuple(xs, sensor_count{ electrodes }, measurement_count{ samples });
    }
};

template<typename T, typename Format>
struct make_compressed
{
    random_source* _random;

    make_compressed(random_source& r, T, Format)
        : _random{ &r } {
    }

    auto operator()(size_t size) -> matrix_tuple<uint8_t> {
        std::vector<uint8_t> xs;
        constexpr const size_t min_x{ 0 };
        const auto elc{ choose(min_x, size, *_random) };
        const auto smpl{ choose(min_x, size, *_random) };

        for (size_t i{ 0 }; i < elc; ++i) {
            auto [bytes, usize]{ generate_block(smpl, *_random, T{}, Format{}) };
            xs.insert(end(xs), begin(bytes), end(bytes));
        }

        const auto electrodes{ cast(elc, sensor_count::value_type{}, ok{}) };
        const auto samples{ cast(smpl, measurement_count::value_type{}, ok{}) };
        return std::make_tuple(xs, sensor_count{ electrodes }, measurement_count{ samples });
    }
};


template<typename T, typename Format>
struct encode_decode_matrix : arguments<matrix_tuple<T>>
{
    virtual auto accepts(const matrix_tuple<T>& args) const -> bool override final {
        const auto& xs{ std::get<0>(args) };
        const auto electrodes{ std::get<1>(args) };
        const auto samples{ std::get<2>(args) };

        return !xs.empty() && 0 < electrodes && 0 < samples;
    }

    virtual auto holds(const matrix_tuple<T>& args) const -> bool override final {
        using enc_type = matrix_encoder_general<T, Format>;
        using dec_type = matrix_decoder_general<T, Format>;

        const auto& xs{ std::get<0>(args) };
        const auto electrodes{ std::get<1>(args) };
        const auto samples{ std::get<2>(args) };

        enc_type encode;
        dec_type decode;
        encode.row_count(electrodes);
        decode.row_count(electrodes);

        constexpr const row_major2row_major copy;
        const auto bytes{ encode(xs, samples, copy) };
        auto ys{ decode(bytes, samples, copy) };

        return xs == ys;
    }

    auto classify(const matrix_tuple<T>& args) const -> std::string override final {
        using enc_type = matrix_encoder_general<T, Format>;

        const auto& xs{ std::get<0>(args) };
        const auto electrodes{ std::get<1>(args) };
        const auto samples{ std::get<2>(args) };

        enc_type encode;
        encode.row_count(electrodes);

        constexpr const row_major2row_major copy;
        const auto bytes{ encode(xs, samples, copy) };

        return print_class(std::make_tuple(bytes, electrodes, samples), T{}, Format{});
    }
};


template<typename T, typename Format>
struct decode_encode_matrix : arguments<matrix_tuple<uint8_t>>
{
    virtual auto accepts(const matrix_tuple<uint8_t>& args) const -> bool override final {
        const auto& bytes{ std::get<0>(args) };
        const auto electrodes{ std::get<1>(args) };
        const auto samples{ std::get<2>(args) };

        return !bytes.empty() && 0 < electrodes && 0 < samples;
    }

    virtual auto holds(const matrix_tuple<uint8_t>& args) const -> bool override final {
        using enc_type = matrix_encoder_general<T, Format>;
        using dec_type = matrix_decoder_general<T, Format>;

        const auto& bytes_x{ std::get<0>(args) };
        const auto electrodes{ std::get<1>(args) };
        const auto samples{ std::get<2>(args) };

        dec_type decode;
        enc_type encode;

        decode.row_count(electrodes);
        encode.row_count(electrodes);

        // decodes the compressed byte stream
        constexpr const column_major2row_major transpose;
        auto decoded_x{ decode(bytes_x, samples, transpose) };

        // encodes the decoded sequence
        const auto bytes_y{ encode(decoded_x, samples, transpose) };

        // the encoder almost certainly picked different parameters meaning
        // that the compressed streams (bytes_x and bytes_y) can not be compared verbatim.
        // for this reason another decoding step is performed and the decoded sequences are compared instead.
        auto decoded_y{ decode(bytes_y, samples, transpose) };

        return decoded_x == decoded_y;
    }

    virtual auto classify(const matrix_tuple<uint8_t>& args) const -> std::string override final {
        return print_class(args, T{}, Format{});
    }
};


auto main(int, char*[]) -> int {
    using enc_dec_32ref = encode_decode_matrix<uint32_t, reflib>;
    using enc_dec_8ext = encode_decode_matrix<uint8_t, extended>;
    using enc_dec_16ext = encode_decode_matrix<uint16_t, extended>;
    using enc_dec_32ext = encode_decode_matrix<uint32_t, extended>;
    using enc_dec_64ext = encode_decode_matrix<uint64_t, extended>;

    using dec_enc_32ref = decode_encode_matrix<uint32_t, reflib>;
    using dec_enc_8ext = decode_encode_matrix<uint8_t, extended>;
    using dec_enc_16ext = decode_encode_matrix<uint16_t, extended>;
    using dec_enc_32ext = decode_encode_matrix<uint32_t, extended>;
    using dec_enc_64ext = decode_encode_matrix<uint64_t, extended>;


    random_source r;
    //random_source r{ 3946883574 };
    bool ok{ true };

    ok &= check("enc/dec reflib 32 bit",   enc_dec_32ref{}, make_uncompressed{ r, uint32_t{} });
    ok &= check("enc/dec extended 8 bit",  enc_dec_8ext{},  make_uncompressed{ r, uint8_t{} });
    ok &= check("enc/dec extended 16 bit", enc_dec_16ext{}, make_uncompressed{ r, uint16_t{} });
    ok &= check("enc/dec extended 32 bit", enc_dec_32ext{}, make_uncompressed{ r, uint32_t{} });
    ok &= check("enc/dec extended 64 bit", enc_dec_64ext{}, make_uncompressed{ r, uint64_t{} });

    ok &= check("dec/enc reflib 32 bit",   dec_enc_32ref{}, make_compressed{ r, uint32_t{}, reflib{} });
    ok &= check("dec/enc extended 8 bit",  dec_enc_8ext{},  make_compressed{ r, uint8_t{},  extended{} });
    ok &= check("dec/enc extended 16 bit", dec_enc_16ext{}, make_compressed{ r, uint16_t{}, extended{} });
    ok &= check("dec/enc extended 32 bit", dec_enc_32ext{}, make_compressed{ r, uint32_t{}, extended{} });
    ok &= check("dec/enc extended 64 bit", dec_enc_64ext{}, make_compressed{ r, uint64_t{}, extended{} });

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

