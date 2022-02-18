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
    const api::v1::DcDate dc0{ api::timepoint2dcdate(tp0) };
    REQUIRE(api::dcdate2timepoint(dc0) == tp0);
    REQUIRE(dc0.date == 0);
    REQUIRE(dc0.fraction == 0);
    api::print(std::cerr, dc0); std::cerr << "\n";

    const system_clock::time_point tp1{ date::sys_days{ date::December/29/1899 } };
    const api::v1::DcDate dc1{ api::timepoint2dcdate(tp1) };
    const auto delme{ api::dcdate2timepoint(dc1) };
    const std::chrono::nanoseconds diff{ delme - tp1 };
    const auto secs{ std::chrono::duration_cast<std::chrono::seconds>(diff) };
    REQUIRE(api::dcdate2timepoint(dc1) == tp1);
    REQUIRE(dc1.date == -1);
    REQUIRE(dc1.fraction == 0);
    api::print(std::cerr, dc1); std::cerr << "\n";

    const system_clock::time_point tp2{ date::sys_days{ date::December/31/1899 } };
    const api::v1::DcDate dc2{ api::timepoint2dcdate(tp2) };
    REQUIRE(api::dcdate2timepoint(dc2) == tp2);
    REQUIRE(dc2.date == 1);
    REQUIRE(dc2.fraction == 0);
    api::print(std::cerr, dc2); std::cerr << "\n";

    const system_clock::time_point tp3{ date::sys_days{ date::December/29/1899 } + 6h };
    const api::v1::DcDate dc3{ api::timepoint2dcdate(tp3) };
    REQUIRE(api::dcdate2timepoint(dc3) == tp3);
    REQUIRE(dc3.date == -0.75);
    REQUIRE(dc3.fraction == 0);
    api::print(std::cerr, dc3); std::cerr << "\n";

    const system_clock::time_point tp4{ date::sys_days{ date::December/28/1899 } + 12h };
    const api::v1::DcDate dc4{ api::timepoint2dcdate(tp4) };
    REQUIRE(api::dcdate2timepoint(dc4) == tp4);
    REQUIRE(dc4.date == -1.5);
    REQUIRE(dc4.fraction == 0);
    api::print(std::cerr, dc4); std::cerr << "\n";

    const system_clock::time_point tp5{ date::sys_days{ date::January/1/1900 } + 6h };
    const api::v1::DcDate dc5{ api::timepoint2dcdate(tp5) };
    REQUIRE(api::dcdate2timepoint(dc5) == tp5);
    REQUIRE(dc5.date == 2.25);
    REQUIRE(dc5.fraction == 0);
    api::print(std::cerr, dc5); std::cerr << "\n";

    const system_clock::time_point tp6{ date::sys_days{ date::December/29/1899 } + 1s };
    const api::v1::DcDate dc6{ api::timepoint2dcdate(tp6) };
    REQUIRE(api::dcdate2timepoint(dc6) == tp6);
    REQUIRE(dc6.date == -1.0 + (1.0 / seconds_per_day));
    REQUIRE(dc6.fraction == 0);
    api::print(std::cerr, dc6); std::cerr << "\n";

    const system_clock::time_point tp7{ date::sys_days{ date::January/1/1900 } + 6h + 1s };
    const api::v1::DcDate dc7{ api::timepoint2dcdate(tp7) };
    REQUIRE(api::dcdate2timepoint(dc7) == tp7);
    REQUIRE(dc7.date == 2.25 + (1.0 / seconds_per_day));
    REQUIRE(dc7.fraction == 0);
    api::print(std::cerr, dc7); std::cerr << "\n";

    const system_clock::time_point tp8{ date::sys_days{ date::December/29/2020 } };
    const api::v1::DcDate dc8{ api::timepoint2dcdate(tp8) };
    REQUIRE(api::dcdate2timepoint(dc8) == tp8);
    REQUIRE(dc8.date == 44194.0);
    REQUIRE(dc8.fraction == 0.0);
    api::print(std::cerr, dc8); std::cerr << "\n";

    const system_clock::time_point tp9{ date::sys_days{ date::December/29/2020 } + 6h + 1s };
    const api::v1::DcDate dc9{ api::timepoint2dcdate(tp9) };
    REQUIRE(api::dcdate2timepoint(dc9) == tp9);
    REQUIRE(dc9.date == 44194.25 + (1.0 / seconds_per_day));
    REQUIRE(dc9.fraction == 0);
    api::print(std::cerr, dc9); std::cerr << "\n";
}


TEST_CASE("time point -> dcdate -> time point conversion", "[consistency]") {
    using namespace std::chrono;

    auto input{ system_clock::now() };
    auto end{ input + 3ms };

    while (input < end) {
        const auto date{ api::timepoint2dcdate(input) };
        const auto output{ api::dcdate2timepoint(date) };
        const system_clock::duration diff{ duration_cast<system_clock::duration>(input - output) };
        REQUIRE(diff == 0ns);

        input += 1us;
    }

    auto date{ api::timepoint2dcdate(input) };
    date.fraction += 151.001;
    const system_clock::duration offset{ 2min + 31s + 1ms }; // 151.001 seconds
    REQUIRE(api::dcdate2timepoint(date) == input + offset);
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


