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

auto operator<<(std::ostream& oss, std::chrono::system_clock::time_point x) -> std::ostream& {
    using namespace std::chrono;

    nanoseconds x_us{ duration_cast<nanoseconds>(x.time_since_epoch()) };
    days x_days{ duration_cast<days>(x_us) };
    date::year_month_day ymd{ date::sys_days{ x_days } };
    oss << ymd << " ";

    nanoseconds reminder{ x_us - x_days };
    hours h{ duration_cast<hours>(reminder) };
    reminder -= h;
    oss << std::setfill('0') << std::setw(2) << h.count() << ":";

    minutes m{ duration_cast<minutes>(reminder) };
    reminder -= m;
    oss << std::setfill('0') << std::setw(2) << m.count() << ":";

    seconds s{ duration_cast<seconds>(reminder) };
    reminder -= s;
    oss << std::setfill('0') << std::setw(2) << s.count() << ":";

    oss << reminder.count();

    return oss;
}


TEST_CASE("time point -> dcdate -> time point conversion", "[consistency]") {
    using namespace std::chrono;

    auto input{ system_clock::now() };
    auto end{ input + milliseconds{ 3 } };

    while (input < end) {
        const auto date{ api::timepoint2dcdate(input) };
        const auto output{ api::dcdate2timepoint(date) };

        nanoseconds diff{ duration_cast<nanoseconds>(input - output) };
        if (diff < nanoseconds{ 0 }) {
            diff = nanoseconds{ 0 } - diff;
        }
        assert(0 <= diff.count());

#ifdef _WIN32
        REQUIRE(diff <= nanoseconds{ 500 });
        input += microseconds{ 1 };
#else
        REQUIRE(diff == nanoseconds{ 0 });
        input += nanoseconds{ 1 };
#endif // _WIN32
    }

    auto date{ api::timepoint2dcdate(input) };
    date.fraction += 151.001;
    const nanoseconds offset{ minutes{ 2 } + seconds{ 31 } + milliseconds{ 1 } }; // 151.001 seconds

#ifdef _WIN32
    nanoseconds diff{ duration_cast<nanoseconds>(api::dcdate2timepoint(date) - (input + offset)) };
    if (diff < nanoseconds{ 0 }) {
        diff = nanoseconds{ 0 } - diff;
    }
    assert(0 <= diff.count());
    REQUIRE(diff <= nanoseconds{ 500 } );
#else
    REQUIRE(api::dcdate2timepoint(date) == input + offset);
#endif // _WIN32
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


