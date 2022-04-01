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


#include "arithmetic.h"

namespace ctk { namespace impl { namespace test {

struct different_size_same_sidedness
{
    template<typename Narrow, typename Wide>
    static
    auto around_min(Narrow, Wide) -> void {
        static_assert(sizeof(Narrow) < sizeof(Wide));
        static_assert(
            (std::is_signed<Narrow>::value && std::is_signed<Wide>::value) ||
            (std::is_unsigned<Narrow>::value && std::is_unsigned<Wide>::value));

        constexpr const Narrow none{ 1 };
        constexpr const Narrow nmin{ std::numeric_limits<Narrow>::min() };

        constexpr const Wide wone{ 1 };
        constexpr const Wide w_nmin{ nmin };

        if constexpr (std::is_signed<Narrow>::value && std::is_signed<Wide>::value) {
            REQUIRE_THROWS(cast(static_cast<Wide>(w_nmin - wone), Narrow{}, ok{}));
        }

        // wide -> narrow
        REQUIRE(cast(w_nmin, Narrow{}, ok{}) == nmin);
        REQUIRE(cast(w_nmin + wone, Narrow{}, ok{}) == nmin + none);

        // narrow -> wide
        REQUIRE(cast(nmin, Wide{}, ok{}) == w_nmin);
        REQUIRE(cast(nmin + none, Wide{}, ok{}) == w_nmin + wone);
    }

    template<typename Narrow, typename Wide>
    static
    auto around_max(Narrow, Wide) -> void {
        static_assert(sizeof(Narrow) < sizeof(Wide));
        static_assert(
            (std::is_signed<Narrow>::value && std::is_signed<Wide>::value) ||
            (std::is_unsigned<Narrow>::value && std::is_unsigned<Wide>::value));

        constexpr const Narrow none{ 1 };
        constexpr const Narrow nmax{ std::numeric_limits<Narrow>::max() };

        constexpr const Wide wone{ 1 };
        constexpr const Wide w_nmax{ nmax };

        // wide -> narrow
        REQUIRE(cast(w_nmax - wone, Narrow{}, ok{}) == nmax - none);
        REQUIRE(cast(w_nmax, Narrow{}, ok{}) == nmax);
        REQUIRE_THROWS(cast(w_nmax + wone, Narrow{}, ok{}));

        // narrow -> wide
        REQUIRE(cast(nmax - none, Wide{}, ok{}) == w_nmax - wone);
        REQUIRE(cast(nmax, Wide{}, ok{}) == w_nmax);
    }

    template<typename Narrow, typename Wide>
    static
    auto min_max(Narrow, Wide) -> void {
        around_min(Narrow{}, Wide{});
        around_max(Narrow{}, Wide{});
    }
};

struct different_size_narrow_signed_wide_unsigned
{
    template<typename Narrow, typename Wide>
    static
    auto around_min(Narrow, Wide) -> void {
        static_assert(sizeof(Narrow) < sizeof(Wide));
        static_assert(std::is_signed<Narrow>::value);
        static_assert(std::is_unsigned<Wide>::value);

        constexpr const Narrow nzero{ 0 };
        constexpr const Narrow none{ 1 };
        constexpr const Narrow n_minus_one{ nzero - none };
        constexpr const Narrow nmin{ std::numeric_limits<Narrow>::min() };
        constexpr const Wide wzero{ 0 };
        constexpr const Wide wone{ 1 };
        constexpr const Wide w_minus_one{ static_cast<Wide>(wzero - wone) };
        constexpr const Wide wmin{ std::numeric_limits<Wide>::min() };

        // wide unsigned -> narrow signed
        REQUIRE_THROWS(cast(w_minus_one, Narrow{}, ok{}));
        REQUIRE(cast(wmin, Narrow{}, ok{}) == nzero);
        REQUIRE(cast(wmin + wone, Narrow{}, ok{}) == none);

        // narrow signed -> wide unsigned
        REQUIRE_THROWS(cast(n_minus_one, Wide{}, ok{}));
        REQUIRE(cast(nzero, Wide{}, ok{}) == wzero);
        REQUIRE(cast(nzero + none, Wide{}, ok{}) == wone);
        REQUIRE_THROWS(cast(nmin, Wide{}, ok{}));
    }

    template<typename Narrow, typename Wide>
    static
    auto around_max(Narrow, Wide) -> void {
        static_assert(sizeof(Narrow) < sizeof(Wide));
        static_assert(std::is_signed<Narrow>::value);
        static_assert(std::is_unsigned<Wide>::value);

        constexpr const Narrow none{ 1 };
        constexpr const Narrow nmax{ std::numeric_limits<Narrow>::max() };
        constexpr const Wide wone{ 1 };
        constexpr const Wide w_nmax{ nmax };
        constexpr const Wide wmax{ std::numeric_limits<Wide>::max() };

        // wide unsigned -> narrow signed
        REQUIRE(cast(w_nmax, Narrow{}, ok{}) == nmax);
        REQUIRE_THROWS(cast(w_nmax + wone, Narrow{}, ok{}));
        REQUIRE_THROWS(cast(wmax, Narrow{}, ok{}));

        // narrow signed -> wide unsigned
        REQUIRE(cast(nmax - none, Wide{}, ok{}) == w_nmax - wone);
        REQUIRE(cast(nmax, Wide{}, ok{}) == w_nmax);
    }

    template<typename Narrow, typename Wide>
    static
    auto min_max(Narrow, Wide) -> void {
        around_min(Narrow{}, Wide{});
        around_max(Narrow{}, Wide{});
    }
};


struct different_size_narrow_unsigned_wide_signed
{
    template<typename Narrow, typename Wide>
    static
    auto around_min(Narrow, Wide) -> void {
        static_assert(sizeof(Narrow) < sizeof(Wide));
        static_assert(std::is_unsigned<Narrow>::value);
        static_assert(std::is_signed<Wide>::value);

        constexpr const Narrow none{ 1 };
        constexpr const Narrow nmin{ std::numeric_limits<Narrow>::min() };

        constexpr const Wide wzero{ 0 };
        constexpr const Wide wone{ 1 };
        constexpr const Wide wmin{ std::numeric_limits<Wide>::min() };

        // wide signed -> narrow unsigned
        REQUIRE_THROWS(cast(wmin, Narrow{}, ok{}));
        REQUIRE_THROWS(cast(wzero - wone, Narrow{}, ok{}));
        REQUIRE(cast(wzero, Narrow{}, ok{}) == nmin);
        REQUIRE(cast(wone, Narrow{}, ok{}) == nmin + none);

        // narrow unsigned -> wide signed
        REQUIRE(cast(nmin, Wide{}, ok{}) == wzero);
        REQUIRE(cast(nmin + none, Wide{}, ok{}) == wone);
    }

    template<typename Narrow, typename Wide>
    static
    auto around_max(Narrow, Wide) -> void {
        static_assert(sizeof(Narrow) < sizeof(Wide));
        static_assert(std::is_unsigned<Narrow>::value);
        static_assert(std::is_signed<Wide>::value);

        constexpr const Narrow none{ 1 };
        constexpr const Narrow nmax{ std::numeric_limits<Narrow>::max() };

        constexpr const Wide w_nmax{ nmax };
        constexpr const Wide wmax{ std::numeric_limits<Wide>::max() };

        // wide signed -> narrow unsigned
        REQUIRE_THROWS(cast(wmax, Narrow{}, ok{}));
        REQUIRE_THROWS(cast(w_nmax + none, Narrow{}, ok{}));
        REQUIRE(cast(w_nmax, Narrow{}, ok{}) == nmax);
        REQUIRE(cast(w_nmax - none, Narrow{}, ok{}) == nmax - none);

        // narrow unsigned -> wide signed
        REQUIRE(cast(nmax, Wide{}, ok{}) == w_nmax);
        REQUIRE(cast(nmax - none, Wide{}, ok{}) == w_nmax - none);
    }

    template<typename Narrow, typename Wide>
    static
    auto min_max(Narrow, Wide) -> void {
        around_min(Narrow{}, Wide{});
        around_max(Narrow{}, Wide{});
    }
};


struct same_size
{
    template<typename S, typename U>
    static
    auto around_min(S, U) -> void {
        static_assert(sizeof(S) == sizeof(U));
        static_assert(std::is_signed<S>::value);
        static_assert(std::is_unsigned<U>::value);

        constexpr const S szero{ 0 };
        constexpr const S sone{ 1 };
        constexpr const S smin{ std::numeric_limits<S>::min() };

        constexpr const U uone{ 1 };
        constexpr const U umin{ std::numeric_limits<U>::min() };

        // signed -> unsigned
        REQUIRE_THROWS(cast(smin, U{}, ok{}));
        REQUIRE_THROWS(cast(szero - sone, U{}, ok{}));
        REQUIRE(cast(szero, U{}, ok{}) == umin);
        REQUIRE(cast(szero + sone, U{}, ok{}) == umin + uone);

        // unsigned -> signed
        REQUIRE(cast(umin, S{}, ok{}) == szero);
        REQUIRE(cast(umin + uone, S{}, ok{}) == szero + sone);
    }

    template<typename S, typename U>
    static
    auto around_max(S, U) -> void {
        static_assert(sizeof(S) == sizeof(U));
        static_assert(std::is_signed<S>::value);
        static_assert(std::is_unsigned<U>::value);

        constexpr const S sone{ 1 };
        constexpr const S smax{ std::numeric_limits<S>::max() };

        constexpr const U uone{ 1 };
        constexpr const U umax{ std::numeric_limits<U>::max() };
        constexpr const U u_smax{ smax };

        // signed -> unsigned
        REQUIRE(cast(smax - sone, U{}, ok{}) == u_smax - uone);
        REQUIRE(cast(smax, U{}, ok{}) == u_smax);

        // unsigned -> signed
        REQUIRE_THROWS(cast(umax, S{}, ok{}));
        REQUIRE_THROWS(cast(u_smax + uone, S{}, ok{}));
        REQUIRE(cast(u_smax, S{}, ok{}) == smax);
        REQUIRE(cast(u_smax - uone, S{}, ok{}) == smax - sone);
    }

    template<typename S, typename U>
    static
    auto min_max(S, U) -> void {
        around_min(S{}, U{});
        around_max(S{}, U{});
    }
};


template<typename Narrow, typename Wide>
auto test_cast(Narrow, Wide) -> void {
    using SNarrow = typename std::make_signed<Narrow>::type;
    using UNarrow = typename std::make_unsigned<Narrow>::type;

    using SWide = typename std::make_signed<Wide>::type;
    using UWide = typename std::make_unsigned<Wide>::type;

    different_size_same_sidedness::min_max(SNarrow{}, SWide{});
    different_size_same_sidedness::min_max(UNarrow{}, UWide{});

    different_size_narrow_signed_wide_unsigned::min_max(SNarrow{}, UWide{});
    different_size_narrow_unsigned_wide_signed::min_max(UNarrow{}, SWide{});

    same_size::min_max(SNarrow{}, UNarrow{});
    same_size::min_max(SWide{}, UWide{});
}


TEST_CASE("throw cast", "[correct]") {
    test_cast(int8_t{}, int16_t{});
    test_cast(int8_t{}, int32_t{});
    test_cast(int8_t{}, int64_t{});

    test_cast(int16_t{}, int32_t{});
    test_cast(int16_t{}, int64_t{});

    test_cast(int32_t{}, int64_t{});
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */

