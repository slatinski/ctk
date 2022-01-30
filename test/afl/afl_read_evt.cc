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
#include "ctk.h"

auto generate_input_file(const std::filesystem::path& fname) -> void {
    std::cerr << "writing " << fname << "\n";

    std::vector<float> impedances(3);
    std::iota(begin(impedances), end(impedances), 12.0);
    ctk::EventImpedance impedance{ std::chrono::system_clock::now(), impedances };

    ctk::EventVideo video{std::chrono::system_clock::now(), 1.0, 128 };
    video.condition_label = L"Rare";
    video.description = "a description";
    video.video_file = L"/path/to/file";

    ctk::EventEpoch epoch{ std::chrono::system_clock::now(), 2.0, -1.5, 128 };
    epoch.condition_label = L"Frequent";

    ctk::EventWriter writer{ fname };
    writer.addImpedance(impedance);
    writer.addVideo(video);
    writer.addEpoch(epoch);
    writer.close();
}


auto read(const std::filesystem::path& fname) -> void {
    ctk::EventReader reader{ fname };
    const auto impedances{ reader.impedanceEvents() };
    const auto videos{ reader.videoEvents() };
    const auto epochs{ reader.epochEvents() };

    size_t i_count{ reader.impedanceCount() };
    size_t v_count{ reader.videoCount() };
    size_t e_count{ reader.epochCount() };
    assert(i_count == impedances.size());
    assert(v_count == videos.size());
    assert(e_count == epochs.size());

    for (size_t i{ 0 }; i < i_count; ++i) {
        assert(impedances[i] == reader.impedanceEvent(i));
    }

    for (size_t i{ 0 }; i < v_count; ++i) {
        assert(videos[i] == reader.videoEvent(i));
    }

    for (size_t i{ 0 }; i < e_count; ++i) {
        assert(epochs[i] == reader.epochEvent(i));
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
    catch (const ctk::ctk_limit& e) {
        std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
    }
    catch (const ctk::ctk_data& e) {
        std::cerr << " " << e.what() << "\n"; // garbage data
    }

    // not expected
    catch (const ctk::ctk_bug& e) {
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

