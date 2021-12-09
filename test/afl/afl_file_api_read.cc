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
#include "ctk.h"

auto generate_input_file(const std::string& fname) -> void {
    std::cerr << "writing " << fname << "\n";

    const std::string history{ "initial recording" };
    ctk::Info i;
    i.subject_name = "Jane Doe";
    i.physician = "Dr X";
    i.technician = "Mr Y";
    i.machine_make = "eego";
    i.machine_model = "201";
    i.machine_sn = "0000";

    constexpr const int64_t epoch_length{ 4 };
    constexpr const size_t height{ 3 };
    std::vector<int32_t> input(epoch_length * height);
    std::iota(begin(input), end(input), 0);

    ctk::TimeSignal description;
    description.epoch_length = epoch_length;
    description.sampling_frequency = 1024;
    description.start_time = { 0, 0 };
    description.electrodes.resize(height);
    for (size_t i{ 0 }; i < height; ++i) {
        description.electrodes[i].label = "fpx";
        description.electrodes[i].reference = "ref";
        description.electrodes[i].unit = "u";
        description.electrodes[i].iscale = 1.0;
        description.electrodes[i].rscale = 1.0;
    }

    ctk::CntWriterReflib writer{ fname, ctk::RiffType::riff64, history };
    writer.addTimeSignal(description);
    writer.recordingInfo(i);

    for (size_t epoch{ 0 }; epoch < 3; ++epoch) {
        writer.rangeColumnMajor(input);
    }

    std::vector<ctk::Trigger> triggers;
    for (size_t sample{ 0 }; sample < 3; ++sample) {
        triggers.emplace_back(sample, "code");
    }
    writer.triggers(triggers);

    writer.close();
}


auto read(const std::string& fname) -> void {
    ctk::CntReaderReflib reader{ fname };

    const auto total{ reader.sampleCount() };
    const auto description{ reader.description() };
    reader.information();
    reader.triggers();
    reader.fileVersion();
    reader.history();

    for (int64_t i{ 0 }; i < total; ++i) {
        reader.rangeColumnMajor(i, 1);
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
    /*
    catch (const std::ios_base::failure& e) {
        std::cerr << " " << e.what() << "\n"; // string io
    }
    */
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

