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

    const std::string note{ "initial recording" };
    ctk::Info i;
    i.SubjectName = "Jane Doe";
    i.Physician = "Dr X";
    i.Technician = "Mr Y";
    i.MachineMake = "eego";
    i.MachineModel = "201";
    i.MachineSn = "0000";

    constexpr const int64_t epoch_length{ 4 };
    constexpr const size_t height{ 3 };
    std::vector<int32_t> input(epoch_length * height);
    std::iota(begin(input), end(input), 0);

    ctk::TimeSeries param;
    param.EpochLength = epoch_length;
    param.SamplingFrequency = 1024;
    param.StartTime = ctk::api::v1::dcdate2timepoint({ 0, 0 });
    param.Electrodes.resize(height);
    for (size_t j{ 0 }; j < height; ++j) {
        param.Electrodes[j].ActiveLabel = "fpx";
        param.Electrodes[j].Reference = "ref";
        param.Electrodes[j].Unit = "u";
        param.Electrodes[j].IScale = 1.0;
        param.Electrodes[j].RScale = 1.0;
    }

    ctk::CntWriterReflib writer{ fname, ctk::RiffType::riff64 };
    writer.ParamEeg(param);
    writer.RecordingInfo(i);
    writer.History(note);

    for (size_t epoch{ 0 }; epoch < 3; ++epoch) {
        writer.RangeColumnMajorInt32(input);
    }

    std::vector<ctk::Trigger> triggers;
    for (size_t sample{ 0 }; sample < 3; ++sample) {
        triggers.emplace_back(sample, "code");
    }
    writer.AddTriggers(triggers);

    writer.Close();
}


auto read(const std::string& fname) -> void {
    ctk::CntReaderReflib reader{ fname };

    const auto total{ reader.SampleCount() };
    const auto param{ reader.ParamEeg() };
    reader.RecordingInfo();
    reader.Triggers();
    reader.CntFileVersion();
    reader.History();

    for (int64_t i{ 0 }; i < total; ++i) {
        reader.RangeColumnMajorInt32(i, 1);
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

