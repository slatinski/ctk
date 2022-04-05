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

#include <chrono>

#include "api_reflib.h"
#include "test/util.h"

namespace ctk { namespace impl { namespace test {

auto compare(std::chrono::system_clock::time_point x, std::chrono::system_clock::time_point y) -> bool {
    using namespace std::chrono;

    const nanoseconds diff{ duration_cast<nanoseconds>(x - y) };
#ifdef _WIN32
    return -500ns <= diff && diff <= 500ns;
#else
    return diff == 0ns;
#endif
}

auto compare(const api::v1::EventImpedance& x, const api::v1::EventImpedance& y) -> bool {
    const size_t xsize{ x.Values.size() };
    const size_t ysize{ y.Values.size() };
    if (xsize != ysize) {
        return false;
    }

    for (size_t i{ 0 }; i < xsize; ++i) {
        // ohm -> kohm -> ohm roundtrip might lead to loss of precision
        if (1/* ohm */ <= std::fabs(x.Values[i] - y.Values[i])) {
            return false;
        }
    }

    return compare(x.Stamp, y.Stamp);
}

auto compare(const api::v1::EventVideo& x, const api::v1::EventVideo& y) -> bool {
    if (x.Duration != y.Duration) {
        return false;
    }

    if (x.TriggerCode != y.TriggerCode) {
        return false;
    }

    if (x.ConditionLabel != y.ConditionLabel) {
        return false;
    }

    if (x.Description != y.Description) {
        return false;
    }

    if (x.VideoFile != y.VideoFile) {
        return false;
    }

    return compare(x.Stamp, y.Stamp);
}

auto compare(const api::v1::EventEpoch& x, const api::v1::EventEpoch& y) -> bool {
    if (x.Duration != y.Duration) {
        return false;
    }

    if (x.Offset != y.Offset) {
        return false;
    }

    if (x.TriggerCode != y.TriggerCode) {
        return false;
    }

    if (x.ConditionLabel != y.ConditionLabel) {
        return false;
    }

    return compare(x.Stamp, y.Stamp);
}

template<typename T>
auto similar(const std::vector<T>& xs, const std::vector<T>& ys) -> bool {
    const size_t x_size{ xs.size() };
    const size_t y_size{ ys.size() };
    if (x_size != y_size) {
        return false;
    }

    for (size_t i{ 0 }; i < x_size; ++i) {
        if (!compare(xs[i], ys[i])) {
            return false;
        }
    }

    return true;
}


TEST_CASE("write/read impedance event", "[consistency]") {
    const std::vector<float> impedances{ 0, 1, 2, 3, 4, 5, 6, 7 };
    const api::v1::EventImpedance event_impedance{ std::chrono::system_clock::now(), impedances };

    const std::filesystem::path fname_temp{ "delme.evt" };
    {
        api::v1::EventWriter writer{ fname_temp };
        writer.AddImpedance(event_impedance);
        writer.Close();
    }

    api::v1::EventReader reader{ fname_temp };
    REQUIRE(reader.ImpedanceCount() == 1);
    const auto outupt_event{ reader.ImpedanceEvent(0) };
    const auto output_events{ reader.ImpedanceEvents() };
    REQUIRE(output_events.size() == 1);
    REQUIRE(compare(output_events[0], event_impedance));
    REQUIRE(compare(outupt_event, event_impedance));

    std::filesystem::remove(fname_temp);
}


TEST_CASE("read - write - read roundtrip", "[consistency]") {
    constexpr const size_t fname_width{ 20 };
    const std::filesystem::path fname_temp{ "delme.evt" };

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
		try {
            std::filesystem::path evt{ fname };
            evt.replace_extension("evt");

			std::cerr << s2s(fname, fname_width);
            if (!std::filesystem::exists(evt)) {
                std::cerr << ": skipping - no companion evt file\n";
                fname = input.next();
                continue;
            }

            api::v1::EventReader input_reader{ evt };

            const auto input_impedances{ input_reader.ImpedanceEvents() };
            const auto input_videos{ input_reader.VideoEvents() };
            const auto input_epochs{ input_reader.EpochEvents() };

            {
                api::v1::EventWriter writer{ fname_temp };
                for (const auto& i : input_impedances) {
                    writer.AddImpedance(i);
                }
                for (const auto& v : input_videos) {
                    writer.AddVideo(v);
                }
                for (const auto& e : input_epochs) {
                    writer.AddEpoch(e);
                }
                writer.Close();
            }

            const auto events{ input_impedances.size() + input_videos.size() + input_epochs.size() };
            if (events != 0) {
                api::v1::EventReader output_reader{ fname_temp };
                const auto output_impedances{ output_reader.ImpedanceEvents() };
                const auto output_videos{ output_reader.VideoEvents() };
                const auto output_epochs{ output_reader.EpochEvents() };

                REQUIRE(similar(input_impedances, output_impedances));
                REQUIRE(similar(input_videos, output_videos));
                REQUIRE(similar(input_epochs, output_epochs));
            }

            std::filesystem::remove(fname_temp);
            std::cerr << ": evt file roundtrip OK\n";
        }
        catch (const std::exception& e) {
            ignore_expected();
            std::cerr << ": failed [" << e.what() << "]\n";
        }

        fname = input.next();
    }
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
