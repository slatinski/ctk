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

auto write(const std::string& fname) -> void {
    std::cerr << "writing " << fname << "\n";

    const std::string note{ "initial recording" };
    ctk::Info info;
    info.SubjectName = "Jane Doe";
    info.Physician = "Dr X";
    info.Technician = "Mr Y";
    info.MachineMake = "eego";
    info.MachineModel = "201";
    info.MachineSn = "0000";

    constexpr const int64_t epoch_length{ 4 };
    constexpr const int64_t height{ 3 };
    std::vector<int32_t> input(epoch_length * height);
    std::iota(begin(input), end(input), 0);

    ctk::TimeSeries description;
    description.EpochLength = epoch_length;
    description.SamplingFrequency = 1024;
    description.StartTime = std::chrono::system_clock::now();
    description.Electrodes.resize(height);
    for (int64_t i{ 0 }; i < height; ++i) {
        description.Electrodes[i].ActiveLabel = "fpx";
        description.Electrodes[i].Reference = "ref";
        description.Electrodes[i].Unit = "u";
        description.Electrodes[i].IScale = 1.0;
        description.Electrodes[i].RScale = 1.0;
    }

    ctk::CntWriterReflib writer{ fname, ctk::RiffType::riff64 };
    writer.addTimeSignal(description);
    writer.recordingInfo(info);
    writer.history(note);

    for (int epoch{ 0 }; epoch < 25; ++epoch) {
        writer.rangeColumnMajorInt32(input);
    }

    std::vector<ctk::Trigger> triggers;
    for (int sample{ 0 }; sample < 16; ++sample) {
        triggers.emplace_back(sample, "code");
    }
    writer.triggers(triggers);

    writer.close();
}

auto read(const std::string& fname) -> void {
    ctk::CntReaderReflib reader{ fname };

    const auto total{ reader.sampleCount() };
    const auto description{ reader.description() };
    const auto information{ reader.information() };
    const auto triggers{ reader.triggers() };
    const auto fv{ reader.fileVersion() };
    const auto history{ reader.history() };

    std::cerr << "sample count: " << total << "\n";
    std::cerr << "channel count: " << description.Electrodes.size() << "\n";
    std::cerr << description << "\n";
    std::cerr << "info: " << information << "\n";
    std::cerr << "triggers: " << triggers.size() << "\n";
    std::cerr << "file version: " << fv << "\n";
    std::cerr << "history: " << history << "\n";

    size_t accessible{ 0 };
    for (int64_t i{ 0 }; i < total; ++i) {
        const auto sample{ reader.rangeColumnMajorInt32(i, 1) };
        if (sample.size() == description.Electrodes.size()) {
            ++accessible;
        }
    }

    std::cerr << accessible << "/" << total << " samples accessible\n";
}

auto main(int argc, char* argv[]) -> int {
    try {
        if (argc < 2) {
            const std::string fname{ "cnt_file.cnt" };

            write(fname);
            read(fname);
        }
        else {
            const std::string fname{ argv[1] };

            read(fname);
        }
    }
    catch(const std::exception& x) {
        std::cerr << x.what() << "\n";
    }

    return 0;
}

