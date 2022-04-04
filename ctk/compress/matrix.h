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

#include <cmath>
#include <algorithm>
#include <numeric>
#include <array>
#include <iostream>
#include <bit>

#include "compress/block.h"
#include "compress/magnitude.h"
#include "compress/multiplex.h"

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4244 4267)
#endif // WIN32

/*
Implementation of the compression algorithm used by libeeep
(https://sourceforge.net/projects/libeep/).

The input for the encoding algorithm is a matrix of two's complement integral
entities.  The data for each matrix row is compressed separately and appended
to the output (8 bit unsigned integers).

The compression for each row is performed in the following way:

1) The input row data is reduced in 3 different ways. The goal of these
reduction steps is to produce smaller integer values with as many leading
zeroes (or ones) which will be truncated during the block encoding.
2) The ouptut size of each reduction is computed and the reduction leading to
the most compact output is selected for encoding.
3) The residual data from the selected reduction is encoded by the functions in
block.h and the result is appended to the output.

Decompression works in reverse:

1) The residual data for each row is decoded by the functions in block.h.
2) The original row data is reconstructed from the residual data and copied to
the output matrix.
*/

namespace ctk { namespace impl {


struct count_raw3
{
    // returns a value in the closed interval [2, sizeof(T) * 8]
    template<typename T>
    // requirements
    //     - T is unsigned integral type with two's complement implementation
    auto operator()(T x) const -> bit_count {
        static_assert(std::is_integral<T>::value);
        static_assert(std::is_unsigned<T>::value);

        const auto msb{ size_in_bits(x) };
        if (is_set(x, msb)) {
            x = static_cast<T>(~x);
        }

        const T width{ static_cast<T>(std::bit_width(x) + T{ 1 }) }; // n data bits + 1 signum bit
        const T min_width{ 2 };
        const T result{ std::max(min_width, width) };
        return bit_count{ cast(result, bit_count::value_type{}, unguarded{}) };
    }
};


// this structure represents histogram bin for bit size N.
// count contains the amount of row residuals which can be encoded in N bits.
// exceptions contains the amount of row residuals whose bit pattern equals the exception marker for N bits.
struct bucket
{
    sint count;
    sint exceptions;
};



template<typename T>
// requirements
//     - T is unsigned integral type with two's complement implementation
struct estimation {
    // 2 bins waisted in order to allow for natural indexing
    static constexpr const unsigned array_size{ sizeof(T) * bits_per_byte + 1 };

    bit_count max_output_size;

    std::array<bucket, array_size> histogram;
    std::array<bit_count, array_size> output_sizes;

    template<typename Format>
    auto resize(measurement_count samples, Format format) -> void {
        // worst case estimation for the maximum size of a reduction: all
        // entities are encoded in variable width (n + nexc) bits.
        // NB: this size can exceed max_block_size.
        // allows evaluate_histogram to perform unguarded arithmetic.
        const encoding_size data_size{ Format::as_size(T{}) };
        const bit_count header{ compressed_header_width(data_size, format) };
        constexpr const bit_count nexc{ size_in_bits(T{}) };
        // maximum variable length encoding size in bits: 2 * size_in_bits(T) - 1.
        // 2 * size_in_bits(T) is not possible because this is fixed width encoding (n == nexc), encoded in size_in_bits(T) bits.
        const bit_count data{ scale(nexc + nexc - bit_count{ 1 }, samples, ok{}) };
        const bit_count::value_type sh{ header };
        const bit_count::value_type sd{ data };
        max_output_size = bit_count{ plus(sh, sd, ok{}) };
    }
};


template<typename T>
struct reduction
{
    encoding_size data_size;
    encoding_method method;
    bit_count n;
    bit_count nexc;
    byte_count output_size;

    std::vector<T> residuals;
    std::vector<bool> encoding_map;
    std::vector<bit_count> residual_sizes;

    auto resize(measurement_count samples) -> void {
        const auto length{ as_sizet(samples) };
        residuals.resize(length);
        encoding_map.resize(length);
        residual_sizes.resize(length);
    }
};

// populates the histogram bins in the closed interval [2, nexc]
template<typename T>
auto create_histogram(reduction<T>& r, estimation<T>& e) -> void {
    auto& histogram{ e.histogram };
    std::fill(begin(histogram), end(histogram), bucket{ 0, 0 });

    const auto& residuals{ r.residuals };
    const auto& residual_sizes{ r.residual_sizes };
    assert(residuals.size() == residual_sizes.size());
    assert(0 < residuals.size());

    constexpr const sint no{ 0 };
    constexpr const sint yes{ 1 };

    const size_t samples{ residuals.size() };
    for (size_t i{ 1 /* skips the master value, encoded in the header */ }; i < samples; ++i) {
        const auto n{ residual_sizes[i] };
        const auto un{ as_sizet_unchecked(n) };
        const auto [count, exceptions]{ histogram[un] };

        const sint quoted{ is_exception_marker(residuals[i], n) ? yes : no };
        histogram[un] = { plus(count, yes, ok{}), plus(exceptions, quoted, ok{}) };
    }
}


template<typename T, typename Format>
auto evaluate_histogram(measurement_count samples, bit_count nexc, encoding_size data_size, estimation<T>& e, Format format) -> void {
    using Int = bit_count::value_type;

    assert(1 < samples);
    measurement_count::value_type length{ samples };
    --length; // skips the master value, encoded in the header

    const auto& histogram{ e.histogram };
    assert(pattern_size_min() <= nexc);
    assert(as_sizet_unchecked(nexc) < histogram.size());
    // the histogram is populated in the closed interval [2, nexc]
    assert(histogram[0].count == 0 && histogram[0].exceptions == 0);
    assert(histogram[1].count == 0 && histogram[1].exceptions == 0);
    assert(std::accumulate(begin(histogram) + 2,
                           begin(histogram) + static_cast<Int>(nexc) + 1,
                           Int{0},
                           [](Int acc, const bucket& x) { return acc + x.count; }) == length);
    assert(std::accumulate(begin(histogram) + static_cast<Int>(nexc) + 1,
                           end(histogram),
                           Int{0},
                           [](Int acc, const bucket& x) { return acc + x.count; }) == Int{0});
    auto& output_sizes{ e.output_sizes };
    const bit_count header_size{ compressed_header_width(data_size, format) };
    const Int header{ header_size };

    // n != nexc: variable width encoding
    Int wider_than_n{ length };
    for (bit_count n{ pattern_size_min() }; n < nexc; ++n) {
        const size_t i{ as_sizet_unchecked(n) };

        wider_than_n -= histogram[i].count;
        assert(0 <= wider_than_n); // sum(histogram[i].count) = length, for i in [2, sizeof(T) * 8]
        assert(0 <= histogram[i].count && histogram[i].count <= length);

        const Int fixed_size{ scale(n, length, guarded{}) };
        const Int overhead{ scale(nexc, wider_than_n + histogram[i].exceptions, guarded{}) };
        const Int data{ plus(fixed_size, overhead, guarded{}) };
        const bit_count output{ plus(header, data, guarded{}) };
        if (e.max_output_size < output) {
            throw api::v1::CtkBug{ "evaluate_histogram: initialization error, variable width" };
        }

        output_sizes[i] = output;
    }

    // n == nexc: fixed width encoding
    const size_t i{ as_sizet_unchecked(nexc) };
    assert(wider_than_n - histogram[i].count == 0);
    const Int data{ scale(nexc, length, guarded{}) };
    const auto output{ bit_count{ plus(header, data, guarded{}) } };
    if (e.max_output_size < output) {
        throw api::v1::CtkBug{ "evaluate_histogram: initialization error, fixed width" };
    }

    output_sizes[i] = output;
}


template<typename Array>
auto select_n(const Array& output_sizes, bit_count nexc) -> std::pair<bit_count, bit_count> {
    // output_sizes is populated in the closed interval [2, nexc]
    const auto ref{ begin(output_sizes) };
    const auto first{ ref + nbits_min };
    const bit_count::value_type inexc{ nexc };
    const auto last{ ref + inexc + 1 };

    const auto best{ std::min_element(first, last) };
    assert(best != last);
    const auto n{ std::distance(ref, best) };
    const auto in{ cast(n, bit_count::value_type{}, guarded{}) };
    return { bit_count{ in }, *best };
}


template<typename T, typename Format>
auto pick_parameters(reduction<T>& r, estimation<T>& e, bit_count nexc, encoding_size data_size, Format format) -> void {
    const measurement_count samples{ vsize(r.residuals) };

    create_histogram(r, e);
    evaluate_histogram(samples, nexc, data_size, e, format);
    const auto [n, compressed_size]{ select_n(e.output_sizes, nexc) };

    r.data_size = data_size;
    r.n = n;
    r.nexc = nexc;
    r.output_size = as_bytes(compressed_size);
    assert(valid_block_encoding(r.data_size, r.method, r.n, r.nexc, T{}, format));
}


auto min_data_size(bit_count nexc, bit_count master, reflib) -> encoding_size;
auto min_data_size(bit_count nexc, bit_count master, extended) -> encoding_size;


template<typename T, typename Format>
auto compressed_parameters(reduction<T>& r, estimation<T>& e, Format format) -> void {
    const auto& residuals{ r.residuals };
    assert(0 < residuals.size());
    auto& residual_sizes{ r.residual_sizes };
    assert(residuals.size() == residual_sizes.size());
    const auto first{ begin(residual_sizes) };
    const auto last{ end(residual_sizes) };
    if (std::transform(begin(residuals), end(residuals), first, count_raw3{}) != last) {
        throw api::v1::CtkBug{ "compressed_parameters: can not count bits" };
    }

    // epoch length == 1: master value only. encoded as part of the header.
    const auto second{ std::next(first) };
    if (second == last) {
        r.data_size = Format::as_size(T{});
        r.n = pattern_size_min(); // arbitrary valid amount
        r.nexc = pattern_size_min();
        assert(valid_block_encoding(r.data_size, r.method, r.n, r.nexc, T{}, format));
        r.output_size = as_bytes(compressed_header_width(r.data_size, format));
        return;
    }

    assert(second != last);
    const auto nexc{ *std::max_element(second, last) };
    assert(pattern_size_min() <= nexc);
    assert(nexc <= size_in_bits(T{}));

    const auto data_size{ min_data_size(nexc, residual_sizes[0], format) };
    assert(sizeof_word(data_size) <= sizeof(T));

    pick_parameters(r, e, nexc, data_size, format);
}



template<typename I, typename T, typename Format>
// requirements
//     - I is ForwardIterator
//     - ValueType(I) is unsigned integral type with two's complement implementation
auto reduce_row_uncompressed(I first, I last, reduction<T>& r, estimation<T>&, Format format) -> void {
    r.data_size = Format::as_size(T{});
    r.n = size_in_bits(T{});
    r.nexc = r.n;
    r.output_size = max_block_size(first, last, format);
    assert(valid_block_encoding(r.data_size, r.method, r.n, r.nexc, T{}, format));
}


template<typename I>
// requirements
//     - I is ForwardIterator
auto restore_magnitude(I previous, I first, I last, I buffer, encoding_method method) -> I {
    switch(method) {
        case encoding_method::copy: return last;
        case encoding_method::time: return restore_row_time(first, last);
        case encoding_method::time2: return restore_row_time2(first, last);
        case encoding_method::chan: return restore_row_chan(previous, first, last, buffer);
        default: throw api::v1::CtkBug{ "restore_magnitude: invalid method" };
    }
}



struct is_exception
{
    bit_count n;

    template<typename T>
    constexpr
    auto operator()(T pattern, bit_count pattern_width) const -> bool {
        if (pattern_width < n) {
            return false;
        }
        else if (pattern_width == n) {
            return is_exception_marker(pattern, n);
        }
        else {
            return true;
        }
    }
};


template<typename T>
auto build_encoding_map(reduction<T>& r) -> void {
    // fixed width encoding: no exceptions.
    // the map is ignored by entity_fixed_width::encode() in block.h.
    if (r.n == r.nexc) {
        return;
    }

    assert(r.residuals.size() == r.encoding_map.size());
    assert(r.residuals.size() == r.residual_sizes.size());
    const auto first_residual{ begin(r.residuals) };
    const auto last_residual{ end(r.residuals) };
    const auto first_size{ begin(r.residual_sizes) };
    const auto first_map{ begin(r.encoding_map) };
    const auto last_map{ end(r.encoding_map) };

    // variable width encoding
    if (std::transform(first_residual, last_residual, first_size, first_map, is_exception{ r.n }) != last_map) {
        const std::string e{ "[build_encoding_map, matrix] can not compute exception map" };
        throw api::v1::CtkBug{ e };
    }
}


template<typename I, typename T, typename IByte, typename Format>
auto encode_residuals(I first, I last, const reduction<T>& r, bit_writer<IByte>& bits, T type_tag, Format format) -> IByte {
    if (!valid_block_encoding(r.data_size, r.method, r.n, r.nexc, type_tag, format)) {
        const auto e{ invalid_row_header(r.data_size, r.method, r.n, r.nexc, sizeof(T)) };
        throw api::v1::CtkBug{ e };
    }

    const auto first_map{ begin(r.encoding_map) };
    const auto first_out{ bits.current() };
    const auto last_out{ encode_block(first, last, first_map, bits, r.data_size, r.method, r.n, r.nexc, format) };

    if (byte_count{ cast(std::distance(first_out, last_out), byte_count::value_type{}, ok{}) } != r.output_size) {
        throw api::v1::CtkBug{ "encode_residuals: encoding failed" };
    }

    return last_out;
}


template<typename IR>
auto select_reduction(IR first, IR last) -> IR {
    assert(first != last);

    const auto is_shorter = [](const auto& x, const auto& y) -> bool { return x.output_size < y.output_size; };
    const auto shortest{ std::min_element(first, last, is_shorter) };
    assert(shortest != last);

    return shortest;
}



template<typename T, typename Format>
class row_encoder
{
    static constexpr const unsigned array_size{ static_cast<unsigned>(encoding_method::length) };

    std::array<reduction<T>, array_size> reductions;
    estimation<T> scratch;

public:

    row_encoder() {
        reductions[unsigned(encoding_method::copy)].method = encoding_method::copy;
        reductions[unsigned(encoding_method::time)].method = encoding_method::time;
        reductions[unsigned(encoding_method::time2)].method = encoding_method::time2;
        reductions[unsigned(encoding_method::chan)].method = encoding_method::chan;
    }
    row_encoder(const row_encoder&) = default;
    row_encoder(row_encoder&&) = default;
    auto operator=(const row_encoder&) -> row_encoder& = default;
    auto operator=(row_encoder&&) -> row_encoder& = default;
    ~row_encoder() = default;


    auto resize(measurement_count samples) -> void {
        const auto resize_reduction = [samples](auto& reduciton) -> void { reduciton.resize(samples); };

        std::for_each(begin(reductions), end(reductions), resize_reduction);
        scratch.resize(samples, Format{});
    }


    template<typename I, typename IByte>
    // requirements
    //     - I is ForwardIterator
    //     - ValueType(I) is unsigned integral type with two's complement implementation
    auto operator()(I previous, I first, I last, bit_writer<IByte>& bits) -> IByte {
        using U = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_same<T, U>::value);
        assert(std::distance(previous, first) == std::distance(first, last));

        reduce_magnitude(previous, first, last);
        crunch();
        const auto best{ select_reduction(begin(reductions), end(reductions)) };
        if (best == end(reductions) || max_block_size(first, last, Format{}) < best->output_size) {
            const std::string e{ "[row_encoder, matrix] reduction failed" };
            throw api::v1::CtkBug{ e };
        }

        // the residuals buffer for method copy is intentionally not populated by reduce_row_uncompressed.
        // it would have been a verbatim copy of the input.
        // hence a pointer to the input buffer is passed to the block encoder if the method is copy.
        I f{ begin(best->residuals) };
        I l{ end(best->residuals) };
        if (best->method == encoding_method::copy) {
            f = first;
            l = last;
        }

        build_encoding_map(*best);
        return encode_residuals(f, l, *best, bits, T{}, Format{});
    }

private:

    template<typename I>
    auto reduce_magnitude(I previous, I first, I last) -> void {
        reduce_row_uncompressed(first, last, reductions[unsigned(encoding_method::copy)], scratch, Format{});

        auto& residuals_time { reductions[unsigned(encoding_method::time)].residuals };
        auto& residuals_time2{ reductions[unsigned(encoding_method::time2)].residuals };
        auto& residuals_chan { reductions[unsigned(encoding_method::chan)].residuals };

        const I ft{ begin(residuals_time) };
        const I lt{ reduce_row_time(first, last, ft) };
        if (lt != end(residuals_time)) {
            throw api::v1::CtkBug{ "reduce_magnitude: reduction time failed" };
        }

        const I ft2{ begin(residuals_time2) };
        const I lt2{ reduce_row_time2_from_time(ft, lt, ft2) };
        if (lt2 != end(residuals_time2)) {
            throw api::v1::CtkBug{ "reduce_magnitude: reduction time2 failed" };
        }

        const I fch{ begin(residuals_chan) };
        const I lch{ reduce_row_chan_from_time(previous, first, ft, lt, fch) };
        if (lch != end(residuals_chan)) {
            throw api::v1::CtkBug{ "reduce_magnitude: reduction chan failed" };
        }
    }


    auto crunch() -> void {
        constexpr const Format format;

        compressed_parameters(reductions[unsigned(encoding_method::time)], scratch, format);
        compressed_parameters(reductions[unsigned(encoding_method::time2)], scratch, format);
        compressed_parameters(reductions[unsigned(encoding_method::chan)], scratch, format);
    }
};

template<typename IByteConst, typename I, typename Format>
// Format has an interface identical to reflib or extended
auto decode_row(bit_reader<IByteConst>& bits, I previous, I first, I last, I buffer, Format format) -> IByteConst {
    const auto [next, method]{ decode_block(bits, first, last, format) };

    if (restore_magnitude(previous, first, last, buffer, method) != last) {
        throw api::v1::CtkBug{ "decode_row: can not restore magnitude" };
    }

    return next;
}


class dimensions
{
    sensor_count height;
    measurement_count length;

public:

    dimensions();
    dimensions(sensor_count, measurement_count);
    dimensions(const dimensions&) = default;
    dimensions(dimensions&&) = default;
    auto operator=(const dimensions&) -> dimensions& = default;
    auto operator=(dimensions&&) -> dimensions& = default;
    ~dimensions() = default;

    auto electrodes() const -> sensor_count;
    auto samples() const -> measurement_count;

    friend auto operator==(const dimensions&, const dimensions&) -> bool = default;
    friend auto operator!=(const dimensions&, const dimensions&) -> bool = default;
};

auto matrix_size(sensor_count, measurement_count) -> sint;
auto matrix_size(sensor_count, byte_count) -> byte_count;


// the interval [previous, matrix) represents a row with index -1 (filled with zeroes) for method chan.
// the interval [matrix, buffer) contains the input data for the encoder or the output data for the decoder.
// the interval [buffer, buffer + samples) contains a working buffer for method chan.
// the decoder actually uses the memory for the next channel as a rolling buffer
// instead of using the fixed buffer space at the end of the memory block.
template<typename T>
class matrix_buffer
{
    std::vector<T> previous_matrix_buffer;
    ptrdiff_t row_length;
    ptrdiff_t area;

    auto buffer_size(sensor_count electrodes, measurement_count samples) -> std::tuple<ptrdiff_t, ptrdiff_t, size_t> {
        using Int = measurement_count::value_type;

        assert(0 < electrodes && 0 < samples);

        const auto size{ matrix_size(electrodes, samples) };
        const Int length{ samples };
        const Int two_rows{ plus(length, length, ok{}) }; // dummy previous row and buffer

        return { length, size, as_sizet(plus(size, two_rows, ok{})) };
    }

public:

    matrix_buffer()
    : row_length{ 0 }
    , area{ 0 } {
    }
    matrix_buffer(const matrix_buffer&) = default;
    matrix_buffer(matrix_buffer&&) = default;
    auto operator=(const matrix_buffer&) -> matrix_buffer& = default;
    auto operator=(matrix_buffer&&) -> matrix_buffer& = default;
    ~matrix_buffer() = default;

    auto resize(sensor_count electrodes, measurement_count samples) -> void {
        if (electrodes < 1 || samples < 1) {
            std::ostringstream oss;
            oss << "[matrix_buffer::resize, matrix] invalid dimensions " << electrodes << " x " << samples;
            const auto e{ oss.str() };
            throw api::v1::CtkBug{ e };
        }

        const auto [length, a, size]{ buffer_size(electrodes, samples) };
        previous_matrix_buffer.resize(size);
        row_length = length;
        area = a;

        std::fill(previous(), matrix(), T{ 0 });
    }

    auto previous() -> typename std::vector<T>::iterator {
        return begin(previous_matrix_buffer);
    }

    auto matrix() -> typename std::vector<T>::iterator {
        return previous() + row_length;
    }

    auto buffer() -> typename std::vector<T>::iterator {
        return matrix() + area;
    }
};


auto natural_row_order(sensor_count) -> std::vector<int16_t>;
auto is_valid_row_order(std::vector<int16_t> row_order) -> bool;




template<typename T>
// T is signed/unsigned integral type with size 1, 2, 4 or 8 bytes
struct matrix_common
{
    static_assert(std::is_integral<T>::value);
    using UT = typename std::make_unsigned<T>::type;

    matrix_buffer<UT> data;
    std::vector<int16_t> order;
    sensor_count electrodes;
    dimensions initialized_for;
    sint multiplex_upper_bound; // assigned to in order to prevent the compiler from skipping the execution of matrix_size()


    matrix_common()
    : electrodes{ 0 }
    , multiplex_upper_bound{ 0 } {
    }
    matrix_common(const matrix_common&) = default;
    matrix_common(matrix_common&&) = default;
    auto operator=(const matrix_common&) -> matrix_common& = default;
    auto operator=(matrix_common&&) -> matrix_common& = default;
    ~matrix_common() = default;


    auto initialized(measurement_count samples) const -> bool {
        return initialized_for == dimensions{ electrodes, samples };
    }

    auto validate_client_size(measurement_count samples, size_t area) const -> void {
        const auto isize{ matrix_size(electrodes, samples) };
        if (area != static_cast<size_t>(isize)) {
            std::ostringstream oss;
            oss << "[validate_client_size, matrix] initialized for " << area
                << ", requested " << isize
                << " (" << electrodes << " electrodes x " << samples << " samples)";
            const auto e{ oss.str() };
            throw api::v1::CtkData{ e };
        }
    }


    auto resize(measurement_count samples) -> void {
        const dimensions dim{ electrodes, samples }; // validity guard

        data.resize(electrodes, samples);
        multiplex_upper_bound = matrix_size(electrodes, samples);
        initialized_for = dim;
    }


    auto row_order(const std::vector<int16_t>& input) -> bool {
        if (!is_valid_row_order(input)) {
            return false;
        }

        order = input;
        electrodes = sensor_count{ vsize(input) };
        return true;
    }


    auto row_count(sensor_count sensors) -> bool /* void */{
        order = natural_row_order(sensors);
        electrodes = sensors;
        return true;
    }


    auto row_count() const -> sensor_count {
        return electrodes;
    }
};


// functionality equivalent to decompepoch_mux, libcnt/raw3.c
template<typename T, typename Format>
// T is signed/unsigned integral type with size 1, 2, 4 or 8 bytes
// Format is reflib or extended
class matrix_decoder_general
{
    matrix_common<T> common; // row major first order

public:

    using value_type = T;
    using format_type = Format;

    matrix_decoder_general() = default;
    matrix_decoder_general(const matrix_decoder_general&) = default;
    matrix_decoder_general(matrix_decoder_general&&) = default;
    auto operator=(const matrix_decoder_general&) -> matrix_decoder_general& = default;
    auto operator=(matrix_decoder_general&&) -> matrix_decoder_general& = default;
    ~matrix_decoder_general() = default;

    auto row_order(const std::vector<int16_t>& input) -> bool {
        return common.row_order(input);
    }

    auto row_count(sensor_count sensors) -> bool {
        return common.row_count(sensors);
    }

    auto row_count() const -> sensor_count {
        return common.row_count();
    }

    template<typename Multiplex>
    // Multiplex has an interface compatible with column_major2row_major and row_major2row_major (multiplex.h)
    auto operator()(const std::vector<uint8_t>& bytes, measurement_count samples, Multiplex multiplex) -> std::vector<T> {
        if (bytes.empty()) {
            return {};
        }

        if (!common.initialized(samples)) {
            common.resize(samples);
        }

        const sensor_count electrodes{ common.electrodes };
        const auto area{ matrix_size(electrodes, samples) };
        std::vector<T> output(as_sizet(area));
        common.validate_client_size(samples, output.size());

        const measurement_count::value_type l{ samples };
        const ptrdiff_t length{ cast(l, ptrdiff_t{}, ok{}) };
        auto previous{ common.data.previous() };
        auto first{ common.data.matrix() };
        auto next{ first + length };

        auto first_in{ begin(bytes) };
        auto last_in{ end(bytes) };
        bit_reader bits{ first_in, last_in };

        for (sensor_count i{ 0 }; i < electrodes; ++i, next += length) {
            // rolling buffer: the interval [next, next + length) is allocated by matrix_buffer::resize()
            first_in = decode_row(bits, previous, first, next, next, Format{});

            previous = first;
            first = next;
        }

        if (first_in != last_in) {
            const std::string e{ "[matrix_decoder_general, matrix] partial input consumption" };
            throw api::v1::CtkData{ e };
        }

        multiplex.to_client(common.data.matrix(), begin(output), common.order, samples);
        return output;
    }
};


// functionality equivalent to compepoch_mux, libcnt/raw3.c
template<typename T, typename Format>
// T is signed/unsigned integral type with size 1, 2, 4 or 8 bytes
// Format is reflib or extended
class matrix_encoder_general
{
    static_assert(std::is_integral<T>::value);
    using UT = typename std::make_unsigned<T>::type;

    matrix_common<T> common; // row major first order
    row_encoder<UT, Format> encode_row;

public:

    using value_type = T;
    using format_type = Format;

    matrix_encoder_general() = default;
    matrix_encoder_general(const matrix_encoder_general&) = default;
    matrix_encoder_general(matrix_encoder_general&&) = default;
    auto operator=(const matrix_encoder_general&) -> matrix_encoder_general& = default;
    auto operator=(matrix_encoder_general&&) -> matrix_encoder_general& = default;
    ~matrix_encoder_general() = default;

    auto row_order(const std::vector<int16_t>& input) -> bool {
        return common.row_order(input);
    }

    auto row_order() -> const std::vector<int16_t>& {
        return common.order;
    }

    auto row_count(sensor_count electrodes) -> bool {
        return common.row_count(electrodes);
    }

    auto row_count() const -> sensor_count {
        return common.row_count();
    }

    template<typename Multiplex>
    // Multiplex has an interface compatible with column_major2row_major and row_major2row_major (multiplex.h)
    auto operator()(const std::vector<T>& input, measurement_count samples, Multiplex multiplex) -> std::vector<uint8_t> {
        if (input.empty()) {
            return {};
        }

        if (!common.initialized(samples)) {
            encode_row.resize(samples);
            common.resize(samples);
        }
        common.validate_client_size(samples, input.size());

        const auto electrodes{ common.electrodes };
        const auto compressed{ matrix_size(electrodes, max_block_size(samples, Format{}, T{})) };
        std::vector<uint8_t> bytes(as_sizet(compressed));
        auto first_out{ begin(bytes) };
        auto last_out{ end(bytes) };
        std::fill(first_out, last_out, 0);
        bit_writer bits{ first_out, last_out };

        auto previous{ common.data.previous() };
        auto first{ common.data.matrix() };
        auto last{ common.data.buffer() };
        multiplex.from_client(begin(input), first, common.order, samples); // signed -> unsigned conversion

        const measurement_count::value_type l{ samples };
        const ptrdiff_t length{ cast(l, ptrdiff_t{}, ok{}) };
        auto next{ first + length };

        for (sensor_count i{ 0 }; i < electrodes; ++i, next += length) {
            first_out = encode_row(previous, first, next, bits);

            previous = first;
            first = next;
        }

        const auto output_size{ as_sizet(std::distance(begin(bytes), first_out)) };
        if (bytes.size() < output_size) {
            const std::string e{ "[matrix_encoder_general, matrix] write memory access violation" };
            throw api::v1::CtkBug{ e };
        }

        bytes.resize(output_size);
        return bytes;
    }
};


} /* namespace impl */

using matrix_encoder_reflib = impl::matrix_encoder_general<int32_t, impl::reflib>;
using matrix_decoder_reflib = impl::matrix_decoder_general<int32_t, impl::reflib>;

template<typename T>
using matrix_encoder = impl::matrix_encoder_general<T, impl::extended>;
template<typename T>
using matrix_decoder = impl::matrix_decoder_general<T, impl::extended>;


} /* namespace ctk */

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32

