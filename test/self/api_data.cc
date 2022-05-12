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

#include <iomanip>

#include "api_data.h"
#include "date/date.h"


namespace ctk { namespace impl { namespace test {


TEST_CASE("well known values", "[compatibility]") {
    using namespace std::chrono;

    constexpr const double seconds_per_day{ 60 * 60 * 24 };

    const system_clock::time_point tp0{ date::sys_days{ date::December/30/1899 } };
    const api::v1::DcDate dc0{ api::v1::timepoint2dcdate(tp0) };
    REQUIRE(api::v1::dcdate2timepoint(dc0) == tp0);
    REQUIRE(dc0.Date == 0);
    REQUIRE(dc0.Fraction == 0);

    const system_clock::time_point tp1{ date::sys_days{ date::December/29/1899 } };
    const api::v1::DcDate dc1{ api::v1::timepoint2dcdate(tp1) };
    REQUIRE(api::v1::dcdate2timepoint(dc1) == tp1);
    REQUIRE(dc1.Date == -1);
    REQUIRE(dc1.Fraction == 0);

    const system_clock::time_point tp2{ date::sys_days{ date::December/31/1899 } };
    const api::v1::DcDate dc2{ api::v1::timepoint2dcdate(tp2) };
    REQUIRE(api::v1::dcdate2timepoint(dc2) == tp2);
    REQUIRE(dc2.Date == 1);
    REQUIRE(dc2.Fraction == 0);

    const system_clock::time_point tp3{ date::sys_days{ date::December/29/1899 } + 6h };
    const api::v1::DcDate dc3{ api::v1::timepoint2dcdate(tp3) };
    REQUIRE(api::v1::dcdate2timepoint(dc3) == tp3);
    REQUIRE(dc3.Date == -0.75);
    REQUIRE(dc3.Fraction == 0);

    const system_clock::time_point tp4{ date::sys_days{ date::December/28/1899 } + 12h };
    const api::v1::DcDate dc4{ api::v1::timepoint2dcdate(tp4) };
    REQUIRE(api::v1::dcdate2timepoint(dc4) == tp4);
    REQUIRE(dc4.Date == -1.5);
    REQUIRE(dc4.Fraction == 0);

    const system_clock::time_point tp5{ date::sys_days{ date::January/1/1900 } + 6h };
    const api::v1::DcDate dc5{ api::v1::timepoint2dcdate(tp5) };
    REQUIRE(api::v1::dcdate2timepoint(dc5) == tp5);
    REQUIRE(dc5.Date == 2.25);
    REQUIRE(dc5.Fraction == 0);

    const system_clock::time_point tp6{ date::sys_days{ date::December/29/1899 } + 1s };
    const api::v1::DcDate dc6{ api::v1::timepoint2dcdate(tp6) };
    REQUIRE(api::v1::dcdate2timepoint(dc6) == tp6);
    REQUIRE(dc6.Date == -1.0 + (1.0 / seconds_per_day));
    REQUIRE(dc6.Fraction == 0);

    const system_clock::time_point tp7{ date::sys_days{ date::January/1/1900 } + 6h + 1s };
    const api::v1::DcDate dc7{ api::v1::timepoint2dcdate(tp7) };
    REQUIRE(api::v1::dcdate2timepoint(dc7) == tp7);
    REQUIRE(dc7.Date == 2.25 + (1.0 / seconds_per_day));
    REQUIRE(dc7.Fraction == 0);

    const system_clock::time_point tp8{ date::sys_days{ date::December/29/2020 } };
    const api::v1::DcDate dc8{ api::v1::timepoint2dcdate(tp8) };
    REQUIRE(api::v1::dcdate2timepoint(dc8) == tp8);
    REQUIRE(dc8.Date == 44194.0);
    REQUIRE(dc8.Fraction == 0.0);

    const system_clock::time_point tp9{ date::sys_days{ date::December/29/2020 } + 6h + 1s };
    const api::v1::DcDate dc9{ api::v1::timepoint2dcdate(tp9) };
    REQUIRE(api::v1::dcdate2timepoint(dc9) == tp9);
    REQUIRE(dc9.Date == 44194.25 + (1.0 / seconds_per_day));
    REQUIRE(dc9.Fraction == 0);
}


TEST_CASE("time point -> dcdate -> time point conversion", "[consistency]") {
    using namespace std::chrono;

    auto input{ system_clock::now() };
    auto end{ input + 3ms };

    while (input < end) {
        const auto date{ api::v1::timepoint2dcdate(input) };
        const auto output{ api::v1::dcdate2timepoint(date) };
        const system_clock::duration diff{ duration_cast<system_clock::duration>(input - output) };
        REQUIRE(diff == 0ns);

        input += 1us;
    }

    auto date{ api::v1::timepoint2dcdate(input) };
    date.Fraction += 151.001;
    const system_clock::duration offset{ 2min + 31s + 1ms }; // 151.001 seconds
    REQUIRE(api::v1::dcdate2timepoint(date) == input + offset);
}


TEST_CASE("odd input", "[consistency]") {
    using namespace std::chrono;

    const api::v1::DcDate negative_fraction{ 1, -0.2 };
    REQUIRE_THROWS(api::v1::dcdate2timepoint(negative_fraction));

    const api::v1::DcDate giant_date{ 1e200, 0.256 };
    REQUIRE_THROWS(api::v1::dcdate2timepoint(giant_date));

    const api::v1::DcDate giant_fraction{ 0.256, 1e200 };
    REQUIRE_THROWS(api::v1::dcdate2timepoint(giant_fraction));

    const api::v1::DcDate negative_date{ -159.3, 0.265 };
    const auto tp{ api::v1::dcdate2timepoint(negative_date) };
    REQUIRE(api::v1::timepoint2dcdate(tp) == negative_date);

    const auto sys_min{ system_clock::time_point::min() };
    const auto sys_max{ system_clock::time_point::max() };
    const date::year_month_day epoch{ date::December/30/1899 };
    const auto e_len{ date::sys_days{ epoch }.time_since_epoch() }; // negative: 1899/12/30 < 1970/1/1
    const auto t_min{ sys_min + 1h + 1ns };
    const auto t_max{ sys_max + e_len - 2h };
    REQUIRE(api::v1::dcdate2timepoint(api::v1::timepoint2dcdate(t_min)) == t_min);
    REQUIRE(api::v1::dcdate2timepoint(api::v1::timepoint2dcdate(t_max)) == t_max);
    REQUIRE_THROWS(api::v1::dcdate2timepoint(api::v1::timepoint2dcdate(sys_min + 1h)));
    REQUIRE_THROWS(api::v1::dcdate2timepoint(api::v1::timepoint2dcdate(sys_max + e_len - 1h)));

    auto begin{ t_min };
    auto end{ begin + 3ms };
    while (begin != end) {
        REQUIRE(api::v1::dcdate2timepoint(api::v1::timepoint2dcdate(begin)) == begin);
        begin += 1us;
    }

    begin = t_max;
    end = begin - 3ms;
    while (begin != end) {
        REQUIRE(api::v1::dcdate2timepoint(api::v1::timepoint2dcdate(begin)) == begin);
        begin -= 1us;
    }
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


