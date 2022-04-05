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

#include <iostream>
#include <numeric>
#include <cassert>
#include <cmath>
#include "ctk.h"

auto generate_input_file(const std::filesystem::path& fname) -> void {
    std::cerr << "writing " << fname << "\n";

    std::vector<float> impedances(3);
    std::iota(begin(impedances), end(impedances), 12.0f);
    ctk::EventImpedance impedance{ std::chrono::system_clock::now(), impedances };

    ctk::EventVideo video{std::chrono::system_clock::now(), 1.0, 128 };
    video.ConditionLabel = L"Rare";
    video.Description = "a description";
    video.VideoFile = L"/path/to/file";

    ctk::EventEpoch epoch{ std::chrono::system_clock::now(), 2.0, -1.5, 128 };
    epoch.ConditionLabel = L"Frequent";

    ctk::EventWriter writer{ fname };
    writer.AddImpedance(impedance);
    writer.AddVideo(video);
    writer.AddEpoch(epoch);
    writer.Close();
}

auto compare(std::chrono::system_clock::time_point x, std::chrono::system_clock::time_point y) -> bool {
    using namespace std::chrono;

    const nanoseconds diff{ duration_cast<nanoseconds>(x - y) };
#ifdef _WIN32
    return -500ns <= diff && diff <= 500ns;
#else
    return diff == 0ns;
#endif
}

auto compare(const ctk::api::v1::EventImpedance& x, const ctk::api::v1::EventImpedance& y) -> bool {
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

auto compare(const ctk::api::v1::EventVideo& x, const ctk::api::v1::EventVideo& y) -> bool {
    if (std::isfinite(x.Duration) && std::isfinite(y.Duration)) {
        if (x.Duration != y.Duration) {
            return false;
        }
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

auto compare(const ctk::api::v1::EventEpoch& x, const ctk::api::v1::EventEpoch& y) -> bool {
    if (std::isfinite(x.Duration) && std::isfinite(y.Duration)) {
        if (x.Duration != y.Duration) {
            return false;
        }
    }

    if (std::isfinite(x.Offset) && std::isfinite(y.Offset)) {
        if (x.Offset != y.Offset) {
            return false;
        }
    }

    if (x.TriggerCode != y.TriggerCode) {
        return false;
    }

    if (x.ConditionLabel != y.ConditionLabel) {
        return false;
    }

    return compare(x.Stamp, y.Stamp);
}

auto read(const std::filesystem::path& fname) -> void {
    ctk::EventReader reader{ fname };
    const auto impedances{ reader.ImpedanceEvents() };
    const auto videos{ reader.VideoEvents() };
    const auto epochs{ reader.EpochEvents() };

    size_t i_count{ reader.ImpedanceCount() };
    size_t v_count{ reader.VideoCount() };
    size_t e_count{ reader.EpochCount() };
    assert(i_count == impedances.size());
    assert(v_count == videos.size());
    assert(e_count == epochs.size());

    for (size_t i{ 0 }; i < i_count; ++i) {
        assert(compare(impedances[i], reader.ImpedanceEvent(i)));
    }

    for (size_t i{ 0 }; i < v_count; ++i) {
        assert(compare(videos[i], reader.VideoEvent(i)));
    }

    for (size_t i{ 0 }; i < e_count; ++i) {
        assert(compare(epochs[i], reader.EpochEvent(i)));
    }
}


auto ignore_expected() -> void {
    try {
        throw;
    }
    // thrown by stl
    catch (const std::bad_alloc& e) {
        std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
    }
    catch (const std::length_error& e) {
        std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
    }
    catch (const std::invalid_argument& e) {
        std::cerr << " " << e.what() << "\n"; // garbage data. stol, stod
    }
    catch (const std::out_of_range& e) {
        std::cerr << " " << e.what() << "\n"; // garbage data. stol, stod
    }
    // thrown by ctk
    catch (const ctk::CtkLimit& e) {
        std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
    }
    catch (const ctk::CtkData& e) {
        std::cerr << " " << e.what() << "\n"; // garbage data
    }

    // not expected
    catch (const ctk::CtkBug& e) {
        std::cerr << " " << e.what() << "\n"; // bug in this library
        throw;
    }
    catch (const std::exception& e) {
        std::cerr << " " << e.what() << "\n"; // bug in this library
        throw;
    }
}


auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << "missing argument: file name\n";
        return 1;
    }
    // generates a seed file for the fuzzer
    //generate_input_file(argv[1]);
    //return 0;


    try {
        read(argv[1]);
    }
    catch (...) {
        ignore_expected();
        return 1;
    }

    return 0;
}

