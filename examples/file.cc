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
#include "ctk.h"

auto write(const std::filesystem::path& fname) -> void {
    std::cout << "writing " << fname << "\n";
    std::cout << "ctk " << CTK_MAJOR << "." << CTK_MINOR << "." << CTK_PATCH << "." << CTK_BUILD << "\n";

    ctk::CntWriterReflib writer{ fname, ctk::RiffType::riff64 };

    // mandatory
    ctk::TimeSeries param;
    param.SamplingFrequency = 4096;
    param.Electrodes = {
        { "1", "ref" }, // active label, reference
        { "2", "ref" },
        { "3", "ref", "uV" }, // active label, reference, unit
        { "4", "ref", "uV", 1, 1/ctk::Electrode::default_scaling_factor() } // active label, reference, unit, iscale, rscale
    };
    param.StartTime = std::chrono::system_clock::now();
    writer.ParamEeg(param);
    // note: for compatibility reasons do not use "V" in the electrode unit. on the other hand "uV", "nV", etc are fine.
    // for interoperability please use utf8 compatible strings. eg use "uV" instead of "µV"

    // optional
    ctk::Info info;
    info.SubjectName = "Person X";
    info.Physician = "Doctor Y";
    info.Technician = "Operator Z";
    info.MachineMake = "eego";
    info.MachineModel = "ee-201";
    info.MachineSn = "0000";
    writer.RecordingInfo(info);

    // 2 samples, 4 channels, column major first
    std::vector<double> column_major_matrix{
        11, 21, 31, 41,
        12, 22, 32, 42
    };
    writer.RangeColumnMajor(column_major_matrix);

    // 2 samples, 4 channels, row major first
    std::vector<double> row_major_matrix{
        13, 14,
        23, 24,
        33, 34,
        43, 44
    };
    writer.RangeRowMajor(row_major_matrix);
    writer.RangeColumnMajor(column_major_matrix);

    std::vector<ctk::Trigger> triggers{ { 0, "1" }, { 12, "2", }, { 32, "1" } };
    writer.AddTriggers(triggers);
    writer.AddTrigger({ 3, "14" });
    writer.AddTriggers(triggers);

    writer.Close();
}

auto read(const std::filesystem::path& fname) -> void {
    std::cout << "reading " << fname << "\n";

    ctk::CntReaderReflib reader{ fname };

    auto total{ reader.SampleCount() };
    auto param{ reader.ParamEeg() };
    auto info{ reader.RecordingInfo() };
    auto triggers{ reader.Triggers() };

    std::cout << param << "\n";

    std::cout << "triggers { ";
    for (const auto& t : triggers) {
        std::cout << "["<< t << "] ";
    }
    std::cout << "}\n\n";

    std::cout << "data matrix " << total << " samples, " << param.Electrodes.size() << " channels: \n";
    for (int64_t i{ 0 }; i < total; ++i) {
        const auto one_sample{ reader.RangeRowMajor(i, 1) };
        if (one_sample.size() != param.Electrodes.size()) {
            std::cerr << "sample " << i << " is not accessible\n";
            continue;
        }

        for (double x : one_sample) {
            std::cout << x << " ";
        }
        std::cout << "\n";
    }

    std::cout << info.SubjectName << "\n";
}

auto main(int, char*[]) -> int {
    try {
        std::filesystem::path fname{ "example.cnt" };

        write(fname);
        read(fname);

        std::cout << "removing " << fname << "\n";
        std::filesystem::remove(fname);
    }
    catch(const ctk::CtkData& x) {
        std::cerr << x.what() << "\n";
        return 1;
    }
    catch(const ctk::CtkLimit& x) {
        std::cerr << x.what() << "\n";
        return 1;
    }
    catch(const ctk::CtkBug& x) {
        std::cerr << x.what() << "\n";
        return 1;
    }
    catch(const std::exception& x) {
        std::cerr << x.what() << "\n";
        return 1;
    }

    return 0;
}

