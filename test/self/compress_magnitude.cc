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

#include <vector>
#include <numeric>
#include <memory>
#include <iostream>
#include <chrono>

#include "test/util.h"
#include "ctk.h"
#include "file/cnt_reflib.h"


namespace ctk { namespace impl { namespace test {

TEST_CASE("reduce/restore magnitude", "[concistency]") {
    using T = uint32_t; // arbitrary

    std::vector<T> input(4096);
    std::vector<T> buffer(input.size());

	random_values random;
    random.fill(0U, 4096U, input);
    const auto first{ begin(input) };
    const auto last{ end(input) };

    std::vector<T> previous(input.size());
    std::copy(rbegin(input), rend(input), begin(previous)); // arbitrary

    std::vector<T> v_time(input.size());
    std::vector<T> v_time2_v1(input.size());
    std::vector<T> v_time2_v2(input.size());
    std::vector<T> v_time2_v3(input.size());
    std::vector<T> v_chan_v1(input.size());
    std::vector<T> v_chan_v2(input.size());

    for(int i{0}; i < 10; ++i) {
        // reduce time
        const auto ftime{ begin(v_time) };
        const auto ltime{ reduce_row_time(first, last, ftime) };
        REQUIRE(ltime == end(v_time));

        // reduce time2
        const auto ftime2_v1{ begin(v_time2_v1) };
        const auto ltime2_v1{ reduce_row_time2_from_input(first, last, begin(buffer), ftime2_v1) };
        REQUIRE(ltime2_v1 == end(v_time2_v1));

        // reduce time2
        const auto ftime2_v2{ begin(v_time2_v2) };
        const auto ltime2_v2{ reduce_row_time2_from_time(ftime, ltime, ftime2_v2) };
        REQUIRE(ltime2_v2 == end(v_time2_v2));
        REQUIRE(v_time2_v1 == v_time2_v2);

        // reduce time2
        const auto ftime2_v3{ begin(v_time2_v3) };
        const auto ltime2_v3{ reduce_row_time2_from_input_one_pass(first, last, ftime2_v3) };
        REQUIRE(ltime2_v3 == end(v_time2_v3));
        REQUIRE(v_time2_v1 == v_time2_v3);

        // reduce chan
        const auto fchan_v1{ begin(v_chan_v1) };
        const auto lchan_v1{ reduce_row_chan_from_input(begin(previous), first, last, begin(buffer), fchan_v1) };
        REQUIRE(lchan_v1 == end(v_chan_v1));

        // reduce chan
        const auto fchan_v2{ begin(v_chan_v2) };
        const auto lchan_v2{ reduce_row_chan_from_time(begin(previous), first, ftime, ltime, fchan_v2) };
        REQUIRE(lchan_v2 == end(v_chan_v2));
        REQUIRE(v_chan_v1 == v_chan_v2);

        // restore time
        REQUIRE(restore_row_time(ftime, ltime) == ltime);
        REQUIRE(v_time == input);

        // restore time2
        REQUIRE(restore_row_time2(ftime2_v1, ltime2_v1) == ltime2_v1);
        REQUIRE(v_time2_v1 == input);

        // restore time2
        std::copy(ftime2_v2, ltime2_v2, begin(buffer));
        REQUIRE(restore_row_time2_from_buffer(begin(buffer), end(buffer), ftime2_v2) == ltime2_v2);
        REQUIRE(v_time2_v2 == input);

        // restore chan
        REQUIRE(restore_row_chan(begin(previous), fchan_v1, lchan_v1, begin(buffer)) == lchan_v1);
        REQUIRE(v_chan_v1 == input);

        // restore chan
        std::copy(fchan_v2, lchan_v2, begin(buffer));
        REQUIRE(restore_row_chan_from_buffer(begin(buffer), end(buffer), begin(previous), fchan_v2) == lchan_v2);
        REQUIRE(v_chan_v2 == input);

        random.fill(0U, 4096U, input);
    }
}


struct reduction_stat
{
    std::chrono::microseconds time;
    std::chrono::microseconds time2_from_time;
    std::chrono::microseconds time2_from_input_one_pass;
    std::chrono::microseconds time2_from_input_two_pass;
    std::chrono::microseconds chan_from_time;
    std::chrono::microseconds chan_from_input;
    std::chrono::microseconds chan_from_input_alt;

    reduction_stat()
    : time{0}
    , time2_from_time{0}
    , time2_from_input_one_pass{0}
    , time2_from_input_two_pass{0}
    , chan_from_time{0}
    , chan_from_input{0}
    , chan_from_input_alt{0} {
    }

    auto operator+=(const reduction_stat& x) -> reduction_stat& {
        time += x.time;
        time2_from_time += x.time2_from_time;
        time2_from_input_one_pass += x.time2_from_input_one_pass;
        time2_from_input_two_pass += x.time2_from_input_two_pass;
        chan_from_time += x.chan_from_time;
        chan_from_input += x.chan_from_input;
        chan_from_input_alt += x.chan_from_input_alt;
        return *this;
    }
};

struct restore_stat
{
    std::chrono::microseconds time;
    std::chrono::microseconds time_buffer;
    std::chrono::microseconds time2_inplace;
    std::chrono::microseconds time2_buffer;
    std::chrono::microseconds chan_plus;
    std::chrono::microseconds chan_buffer;

    restore_stat()
    : time{0}
    , time_buffer{0}
    , time2_inplace{0}
    , time2_buffer{0}
    , chan_plus{0}
    , chan_buffer{0} {
    }

    auto operator+=(const restore_stat& x) -> restore_stat& {
        time += x.time;
        time_buffer += x.time_buffer;
        time2_inplace += x.time2_inplace;
        time2_buffer += x.time2_buffer;
        chan_plus += x.chan_plus;
        chan_buffer += x.chan_buffer;
        return *this;
    }
};



struct reduce_time
{
    template<typename I>
    auto operator()(I /* previous */, I first, I last, I /* buffer */, I output, I /* first_time */, I /* last_time */) const -> I {
        return reduce_row_time(first, last, output);
    }
};


struct reduce_time2_input_one_pass
{
    template<typename I>
    auto operator()(I /* previous */, I first, I last, I /* buffer */, I output, I /* first_time */, I /* last_time */) const -> I {
        return reduce_row_time2_from_input_one_pass(first, last, output);
    }
};


struct reduce_time2_input_two_pass
{
    template<typename I>
    auto operator()(I /* previous */, I first, I last, I buffer, I output, I /* first_time */, I /* last_time */) const -> I {
        return reduce_row_time2_from_input(first, last, buffer, output);
    }
};


struct reduce_time2_time
{
    template<typename I>
    auto operator()(I /* previous */, I /* first */, I /* last */, I /* buffer */, I output, I first_time, I last_time) const -> I {
        return reduce_row_time2_from_time(first_time, last_time, output);
    }
};

struct reduce_chan_input
{
    template<typename I>
    auto operator()(I previous, I first, I last, I /* buffer */, I output, I /* first_time */, I /* last_time */) const -> I {
        return reduce_row_chan_from_input(previous, first, last, output);
    }
};

struct reduce_chan_time
{
    template<typename I>
    auto operator()(I previous, I first, I /* last */, I /* buffer */, I output, I first_time, I last_time) const -> I {
        return reduce_row_chan_from_time(previous, first, first_time, last_time, output);
    }
};


struct reduce_chan_input_alt
{
    template<typename I>
    auto operator()(I previous, I first, I last, I buffer, I output, I /* first_time */, I /* last_time */) const -> I {
        return reduce_row_chan_from_input(previous, first, last, buffer, output);
    }
};

struct restore_time_inplace
{
    template<typename I>
    auto operator()(I /* previous */, I first, I last, I /* first_buffer */, I /* last_buffer */) const -> I {
        return restore_row_time(first, last);
    }
};

struct restore_time_from_buffer
{
    template<typename I>
    auto operator()(I /* previous */, I first, I /* last */, I first_buffer, I last_buffer) const -> I {
        return std::partial_sum(first_buffer, last_buffer, first);
    }
};


struct restore_time2_inplace
{
    template<typename I>
    auto operator()(I /* previous */, I first, I last, I /* first_buffer */, I /* last_buffer */) const -> I {
        return restore_row_time2(first, last);
    }
};


struct restore_time2_from_buffer
{
    template<typename I>
    auto operator()(I /* previous */, I first, I /* last */, I first_buffer, I last_buffer) const -> I {
        return restore_row_time2_from_buffer(first_buffer, last_buffer, first);
    }
};


struct restore_chan_plus
{
    template<typename I>
    auto operator()(I previous, I first, I last, I first_buffer, I /* last_buffer */) const -> I {
        return restore_row_chan(previous, first, last, first_buffer);
    }
};


struct restore_chan_buffer
{
    template<typename I>
    auto operator()(I previous, I first, I /* last */, I first_buffer, I last_buffer) const -> I {
        return restore_row_chan_from_buffer(first_buffer, last_buffer, previous, first);
    }
};





template<typename I, typename Op>
auto time_reduction(Op op, I previous, I first, I last, I buffer, I first_time, I last_time, int repetitions = 1) -> std::pair<std::vector<typename std::iterator_traits<I>::value_type>, std::chrono::microseconds> {
    using T = typename std::iterator_traits<I>::value_type;

    std::vector<T> output(size_t(std::distance(first, last)));
    const auto fout{ begin(output) };
    std::chrono::microseconds sum{0};

    for (int i{0}; i < repetitions; ++i) {
        const auto start{ std::chrono::steady_clock::now() };
        const auto lout{ op(previous, first, last, buffer, fout, first_time, last_time) };
        const auto stop{ std::chrono::steady_clock::now() };
        sum += std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        REQUIRE(lout == end(output));
    }

    return { output, sum };
}

template<typename I, typename Op>
auto time_restore(Op op, I previous, I first, I last, I first_buffer, I last_buffer, int repetitions = 1) -> std::chrono::microseconds {
    using T = typename std::iterator_traits<I>::value_type;

    const std::vector<T> input(first, last);
    const std::vector<T> buffer(first_buffer, last_buffer);
    std::chrono::microseconds sum{0};

    for (int i{0}; i < repetitions; ++i) {
        std::copy(begin(input), end(input), first);
        std::copy(begin(buffer), end(buffer), first_buffer);

        const auto start{ std::chrono::steady_clock::now() };
        const auto lout{ op(previous, first, last, first_buffer, last_buffer) };
        const auto stop{ std::chrono::steady_clock::now() };
        sum += std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        REQUIRE(lout == last);
    }

    return sum;
}



auto reduction_time(const compressed_epoch& e, const std::vector<int16_t>& order, int repetitions) -> reduction_stat {
    const auto height{ sensor_count{ vsize(order) } };

    using T = typename std::make_unsigned<int32_t>::type;
    matrix_buffer<T> data;
    data.resize(height, e.length);

    api::v1::DecompressReflib reader;
    reader.Sensors(order);
    const int64_t epoch_length{ static_cast<measurement_count::value_type>(e.length) };
    const auto input{ reader.ColumnMajor(e.data, epoch_length) };
    assert(input.size() == size_t(std::distance(data.matrix(), data.buffer())));
    std::copy(begin(input), end(input), data.matrix());

    const ptrdiff_t i_length{ cast(static_cast<sint>(e.length), ptrdiff_t{}, ok{}) };
    auto previous{ data.previous() };
    auto first{ data.matrix() };
    auto buffer{ first + i_length };
    reduction_stat t_reduction;

    for (sensor_count row{0}; row < height; ++row) {
        const auto next{ first + i_length };
        const auto next_buffer{ buffer + i_length };

        auto[v_time, t_time]{ time_reduction(reduce_time{}, previous, first, next, buffer, first, next, repetitions) };
        const auto ftime{ begin(v_time) };
        const auto ltime{ end(v_time) };

        auto[v_time2_time, t_time2_time]{ time_reduction(reduce_time2_time{}, previous, first, next, buffer, ftime, ltime, repetitions) };
        auto[v_time2_input_1pass, t_time2_input_1pass]{ time_reduction(reduce_time2_input_one_pass{}, previous, first, next, buffer, ftime, ltime, repetitions) };
        auto[v_time2_input_2pass, t_time2_input_2pass]{ time_reduction(reduce_time2_input_two_pass{}, previous, first, next, buffer, ftime, ltime, repetitions) };
        REQUIRE(v_time2_time == v_time2_input_1pass);
        REQUIRE(v_time2_time == v_time2_input_2pass);

        auto[v_chan_time, t_chan_time]{ time_reduction(reduce_chan_time{}, previous, first, next, buffer, ftime, ltime, repetitions) };
        auto[v_chan_input, t_chan_input]{ time_reduction(reduce_chan_input{}, previous, first, next, buffer, ftime, ltime, repetitions) };
        auto[v_chan_input_alt, t_chan_input_alt]{ time_reduction(reduce_chan_input_alt{}, previous, first, next, buffer, ftime, ltime, repetitions) };
        REQUIRE(v_chan_input == v_chan_time);
        REQUIRE(v_chan_input == v_chan_input_alt);

        t_reduction.time += t_time;
        t_reduction.time2_from_time += t_time2_time;
        t_reduction.time2_from_input_one_pass += t_time2_input_1pass;
        t_reduction.time2_from_input_two_pass += t_time2_input_2pass;
        t_reduction.chan_from_time += t_chan_time;
        t_reduction.chan_from_input += t_chan_input;
        t_reduction.chan_from_input_alt += t_chan_input_alt;

        previous = first;
        first = next;
        buffer = next_buffer;
    }

    return t_reduction;
}

auto restore_time(const compressed_epoch& e, const std::vector<int16_t>& order, int repetitions) -> restore_stat {
    const auto height{ sensor_count{ vsize(order) } };

    using T = typename std::make_unsigned<int32_t>::type;
    matrix_buffer<T> data;
    data.resize(height, e.length);

    api::v1::DecompressReflib reader;
    reader.Sensors(order);
    const int64_t epoch_length{ static_cast<measurement_count::value_type>(e.length) };
    const auto input{ reader.ColumnMajor(e.data, epoch_length) };
    assert(input.size() == size_t(std::distance(data.matrix(), data.buffer())));
    std::copy(begin(input), end(input), data.matrix());

    const ptrdiff_t i_length{ cast(static_cast<sint>(e.length), ptrdiff_t{}, ok{}) };
    auto previous{ data.previous() };
    auto first{ data.matrix() };
    auto first_buffer{ first + i_length };
    restore_stat t_restore;

    for (sensor_count row{0}; row < height; ++row) {
        const auto next{ first_buffer };
        const auto next_buffer{ next + i_length };

        auto[v_time, u1]{ time_reduction(reduce_time{}, previous, first, next, first_buffer, first, next) };
        auto[v_time2, u2]{ time_reduction(reduce_time2_time{}, previous, first, next, first_buffer, begin(v_time), end(v_time)) };
        auto[v_chan, u3]{ time_reduction(reduce_chan_time{}, previous, first, next, first_buffer, begin(v_time), end(v_time)) };

        std::copy(begin(v_time), end(v_time), first);
        const auto t_time{ time_restore(restore_time_inplace{}, previous, first, next, first_buffer, next_buffer, repetitions) };

        std::copy(begin(v_time), end(v_time), first_buffer);
        const auto t_time_buf{ time_restore(restore_time_from_buffer{}, previous, first, next, first_buffer, next_buffer, repetitions) };

        std::copy(begin(v_time2), end(v_time2), first);
        const auto t_time2_inplace{ time_restore(restore_time2_inplace{}, previous, first, next, first_buffer, next_buffer, repetitions) };

        std::copy(begin(v_time2), end(v_time2), first_buffer);
        const auto t_time2_buffer{ time_restore(restore_time2_from_buffer{}, previous, first, next, first_buffer, next_buffer, repetitions) };

        std::copy(begin(v_chan), end(v_chan), first);
        const auto t_chan_plus{ time_restore(restore_chan_plus{}, previous, first, next, first_buffer, next_buffer, repetitions) };

        std::copy(begin(v_chan), end(v_chan), first_buffer);
        const auto t_chan_buffer{ time_restore(restore_chan_buffer{}, previous, first, next, first_buffer, next_buffer, repetitions) };

        t_restore.time += t_time;
        t_restore.time_buffer += t_time_buf;
        t_restore.time2_inplace += t_time2_inplace;
        t_restore.time2_buffer += t_time2_buffer;
        t_restore.chan_plus += t_chan_plus;
        t_restore.chan_buffer += t_chan_buffer;

        previous = first;
        first = next;
        first_buffer = next_buffer;
    }

    return t_restore;
}



auto print(const reduction_stat& enc, const restore_stat& dec, std::string msg = std::string{}) -> void {
    std::cerr << msg << " reduce:";
    if (enc.time2_from_input_one_pass.count() > 0) {
        const double time_input{ 100.0 * static_cast<double>(enc.time2_from_time.count()) / static_cast<double>(enc.time2_from_input_one_pass.count()) };
        const double two_one{ 100.0 * static_cast<double>(enc.time2_from_input_two_pass.count()) / static_cast<double>(enc.time2_from_input_one_pass.count()) };

        std::cerr << " t2 i1[t" << d2s(time_input) << "%,";
        std::cerr << " i2" << d2s(two_one) << "%],";
    }
    else {
        std::cerr << " t2 i1[t" << s2s("n/a", 7) << "%,";
        std::cerr << " i2" << s2s("n/a", 7) << "%],";
    }

    if (enc.chan_from_input.count() > 0 ) {
        const double time_input{ 100.0 * static_cast<double>(enc.chan_from_time.count()) / static_cast<double>(enc.chan_from_input.count()) };
        const double alt_input{ 100.0 * static_cast<double>(enc.chan_from_input_alt.count()) / static_cast<double>(enc.chan_from_input.count()) };

        std::cerr << " ch i[t" << d2s(time_input) << "%, a" << d2s(alt_input) << "%]";
    }
    else {
        std::cerr << " ch i[t" << s2s("n/a", 7) << " , a" << s2s("n/a", 7) << " ]";
    }


    std::cerr << " | restore:";
    if (dec.time.count() > 0) {
        const double buffer_inplace{ 100.0 * static_cast<double>(dec.time_buffer.count()) / static_cast<double>(dec.time.count()) };

        std::cerr << " t i[b" << d2s(buffer_inplace) << "%],";
    }
    else {
        std::cerr << " t2 i[b" << s2s("n/a", 7) << " ],";
    }

    if (dec.time2_inplace.count() > 0) {
        const double buffer_inplace{ 100.0 * static_cast<double>(dec.time2_buffer.count()) / static_cast<double>(dec.time2_inplace.count()) };

        std::cerr << " t2 i[b" << d2s(buffer_inplace) << "%],";
    }
    else {
        std::cerr << " t2 i[b" << s2s("n/a", 7) << " ],";
    }

    if (dec.chan_plus.count() > 0) {
        const double buffer_plus{ 100.0 * static_cast<double>(dec.chan_buffer.count()) / static_cast<double>(dec.chan_plus.count()) };

        std::cerr << " ch a[b" << d2s(buffer_plus) << "%]";
    }
    else {
        std::cerr << " ch a[b" << s2s("n/a", 7) << " ]";
    }

    std::cerr << std::endl;
}


auto run(epoch_reader_riff& reader, int repetitions) -> std::pair<reduction_stat, restore_stat> {
    reduction_stat t_reduction;
    restore_stat t_restore;
    const auto& order{ reader.data().order() };

    const epoch_count epochs{ reader.data().count() };
    for (epoch_count i{ 0 }; i < epochs; ++i) {
        const compressed_epoch ce{ reader.data().epoch(i) };
        if (ce.data.empty()) {
            std::cerr << "cnt: cannot read epoch " << (i + epoch_count{ 1 }) << "/" << epochs << "\n";
            continue;
        }

        t_reduction += reduction_time(ce, order, repetitions);
        t_restore += restore_time(ce, order, repetitions);
    }

    print(t_reduction, t_restore);
    return { t_reduction, t_restore };
}


TEST_CASE("magnitude", "[performance]") {
    constexpr const size_t fname_width{ 20 };
    constexpr const int repetitions{ 3 };
    std::cerr << repetitions << " repetitions per epoch\n";
    reduction_stat t_reduction;
    restore_stat t_restore;
    size_t i{0};

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
		try {
			std::cerr << s2s(fname, fname_width);
            epoch_reader_riff reader{ fname };
            const auto[t_enc, t_dec]{ run(reader, repetitions) };
            t_reduction += t_enc;
            t_restore += t_dec;
            if (i % 10 == 0) {
                print(t_reduction, t_restore, s2s("AVG", fname_width));
                std::cerr << "\n";
            }
            ++i;
		}
        catch (const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
    }

    print(t_reduction, t_restore, s2s("TOTAL", fname_width));
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */

