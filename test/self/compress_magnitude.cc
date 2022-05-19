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

#include "compress/magnitude.h"
#include "qcheck.h"


using namespace ctk::impl;
using namespace qcheck;


// convenience wrappers: vector -> vector

template<typename T>
auto reduce_time_v1(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys(xs.size());

    if (reduce_row_time(begin(xs), end(xs), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}

template<typename T>
auto reduce_time2_v1(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys(xs.size());
    std::vector<T> buf(xs.size());

    if (reduce_row_time2_from_input(begin(xs), end(xs), begin(buf), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}

template<typename T>
auto reduce_time2_v2(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys(xs.size());

    if (reduce_row_time2_from_input_one_pass(begin(xs), end(xs), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}

template<typename T>
auto reduce_time2_v3(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys{ xs };
    std::adjacent_difference(begin(ys), end(ys), begin(xs));

    if (reduce_row_time2_from_time(begin(xs), end(xs), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}

template<typename T>
auto reduce_chan_v1(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys(xs.size());
    std::vector<T> prev(xs.size());
    std::fill(begin(prev), end(prev), T{ 0 });

    if (reduce_row_chan_from_input(begin(prev), begin(xs), end(xs), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}

template<typename T>
auto reduce_chan_v2(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys(xs.size());
    std::vector<T> buf(xs.size());
    std::vector<T> prev(xs.size());
    std::fill(begin(prev), end(prev), T{ 0 });

    if (reduce_row_chan_from_input(begin(prev), begin(xs), end(xs), begin(buf), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}

template<typename T>
auto reduce_chan_v3(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> prev(xs.size());
    std::fill(begin(prev), end(prev), T{ 0 });

    std::vector<T> ys{ xs };
    std::adjacent_difference(begin(ys), end(ys), begin(xs));

    if (reduce_row_chan_from_time(begin(prev), begin(xs), begin(xs), end(xs), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}




template<typename T>
auto restore_time_v1(std::vector<T> xs) -> std::vector<T> {
    if (restore_row_time(begin(xs), end(xs)) != end(xs)) {
        return {};
    }

    return xs;
}

template<typename T>
auto restore_time2_v1(std::vector<T> xs) -> std::vector<T> {
    if (restore_row_time2(begin(xs), end(xs)) != end(xs)) {
        return {};
    }
    return xs;
}

template<typename T>
auto restore_time2_v2(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys(xs.size());
    if (restore_row_time2_from_buffer(begin(xs), end(xs), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}

template<typename T>
auto restore_chan_v1(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> buf(xs.size());
    std::vector<T> prev(xs.size());
    std::fill(begin(prev), end(prev), T{ 0 });

    if (restore_row_chan(begin(prev), begin(xs), end(xs), begin(buf)) != end(xs)) {
        return {};
    }

    return xs;
}

template<typename T>
auto restore_chan_v2(std::vector<T> xs) -> std::vector<T> {
    std::vector<T> ys(xs.size());
    std::vector<T> prev(xs.size());
    std::fill(begin(prev), end(prev), T{ 0 });

    if (restore_row_chan_from_buffer(begin(xs), end(xs), begin(prev), begin(ys)) != end(ys)) {
        return {};
    }

    return ys;
}


// checks

template<typename T>
struct time_roundtrip : arguments<std::vector<T>>
{
    explicit time_roundtrip(T/* type tag */) {}

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        const bool reduce_restore{ xs == restore_time_v1(reduce_time_v1(xs)) };
        const bool restore_reduce{ xs == reduce_time_v1(restore_time_v1(xs)) };
        return reduce_restore && restore_reduce;
    }

    virtual auto trivial(const std::vector<T>& xs) const -> bool override final {
        return xs.empty();
    }
};

template<typename T>
struct time2_roundtrip : arguments<std::vector<T>>
{
    explicit time2_roundtrip(T/* type tag */) {}

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        const bool reduce_restore{ xs == restore_time2_v1(reduce_time2_v1(xs)) };
        const bool restore_reduce{ xs == reduce_time2_v1(restore_time2_v1(xs)) };
        return reduce_restore && restore_reduce;
    }

    virtual auto trivial(const std::vector<T>& xs) const -> bool override final {
        return xs.empty();
    }
};

template<typename T>
struct chan_roundtrip : arguments<std::vector<T>>
{
    explicit chan_roundtrip(T/* type tag */) {}

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        const bool reduce_restore{ xs == restore_chan_v1(reduce_chan_v1(xs)) };
        const bool restore_reduce{ xs == reduce_chan_v1(restore_chan_v1(xs)) };
        return reduce_restore && restore_reduce;
    }

    virtual auto trivial(const std::vector<T>& xs) const -> bool override final {
        return xs.empty();
    }
};


template<typename T>
struct time2_versions : arguments<std::vector<T>>
{
    explicit time2_versions(T/* type tag */) {}

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        const auto encoded_v1{ reduce_time2_v1(xs) };
        const auto encoded_v2{ reduce_time2_v2(xs) };
        const auto encoded_v3{ reduce_time2_v3(xs) };
        const auto decoded_v1{ restore_time2_v1(xs) };
        const auto decoded_v2{ restore_time2_v2(xs) };

        const bool encoded_equal{ encoded_v1 == encoded_v2 && encoded_v2 == encoded_v3 };
        const bool decoded_equal{ decoded_v1 == decoded_v2 };
        return encoded_equal && decoded_equal;
    }

    virtual auto trivial(const std::vector<T>& xs) const -> bool override final {
        return xs.empty();
    }
};

template<typename T>
struct chan_versions : arguments<std::vector<T>>
{
    explicit chan_versions(T/* type tag */) {}

    virtual auto holds(const std::vector<T>& xs) const -> bool override final {
        const auto encoded_v1{ reduce_chan_v1(xs) };
        const auto encoded_v2{ reduce_chan_v2(xs) };
        const auto encoded_v3{ reduce_chan_v3(xs) };
        const auto decoded_v1{ restore_chan_v1(xs) };
        const auto decoded_v2{ restore_chan_v2(xs) };

        const bool encoded_equal{ encoded_v1 == encoded_v2 && encoded_v2 == encoded_v3 };
        const bool decoded_equal{ decoded_v1 == decoded_v2 };
        return encoded_equal && decoded_equal;
    }

    virtual auto trivial(const std::vector<T>& xs) const -> bool override final {
        return xs.empty();
    }
};



auto main(int, char*[]) -> int {
    random_source r;

    if (!check("time round trip, 8 bit", time_roundtrip{ uint8_t{} }, make_vectors{ r, uint8_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time round trip, 16 bit", time_roundtrip{ uint16_t{} }, make_vectors{ r, uint16_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time round trip, 32 bit", time_roundtrip{ uint32_t{} }, make_vectors{ r, uint32_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time round trip, 64 bit", time_roundtrip{ uint64_t{} }, make_vectors{ r, uint64_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 round trip, 8 bit", time2_roundtrip{ uint8_t{} }, make_vectors{ r, uint8_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 round trip, 16 bit", time2_roundtrip{ uint16_t{} }, make_vectors{ r, uint16_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 round trip, 32 bit", time2_roundtrip{ uint32_t{} }, make_vectors{ r, uint32_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 round trip, 64 bit", time2_roundtrip{ uint64_t{} }, make_vectors{ r, uint64_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan round trip, 8 bit", chan_roundtrip{ uint8_t{} }, make_vectors{ r, uint8_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan round trip, 16 bit", chan_roundtrip{ uint16_t{} }, make_vectors{ r, uint16_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan round trip, 32 bit", chan_roundtrip{ uint32_t{} }, make_vectors{ r, uint32_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan round trip, 64 bit", chan_roundtrip{ uint64_t{} }, make_vectors{ r, uint64_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 implementations, 8 bit", time2_versions{ uint8_t{} }, make_vectors{ r, uint8_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 implementations, 16 bit", time2_versions{ uint16_t{} }, make_vectors{ r, uint16_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 implementations, 32 bit", time2_versions{ uint32_t{} }, make_vectors{ r, uint32_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("time2 implementations, 64 bit", time2_versions{ uint64_t{} }, make_vectors{ r, uint64_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan implementations, 8 bit", chan_versions{ uint8_t{} }, make_vectors{ r, uint8_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan implementations, 16 bit", chan_versions{ uint16_t{} }, make_vectors{ r, uint16_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan implementations, 32 bit", chan_versions{ uint32_t{} }, make_vectors{ r, uint32_t{} })) {
        return EXIT_FAILURE;
    }

    if (!check("chan implementations, 64 bit", chan_versions{ uint64_t{} }, make_vectors{ r, uint64_t{} })) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

