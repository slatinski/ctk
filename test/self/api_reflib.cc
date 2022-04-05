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

#include "test/util.h"
#include "ctk.h"

namespace ctk { namespace impl { namespace test {


TEST_CASE("read write column major", "[consistency] [read] [write]") {
    const std::vector<ctk::Electrode> electrodes{ { "fp1", "ref", "uV" }, { "fp2", "ref", "uV" } };
    const auto start{ std::chrono::system_clock::now() };
    ctk::TimeSeries header{ start, 2048, electrodes, 1024 };

    const std::vector<int32_t> xs{ 11, 12, 21, 22, 31, 32, 41, 42, 51, 52 };

    std::filesystem::path temporary{ "rw_cm.cnt" };
    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        writer.ParamEeg(header);

        writer.RangeColumnMajorInt32(xs);
        writer.RangeColumnMajorInt32(xs);
        writer.RangeColumnMajorInt32(xs);

        writer.Close();
    }

    {
        ctk::CntReaderReflib reader{ temporary };
        REQUIRE(reader.ParamEeg() == header);

        for (int64_t i : { 0, 5, 10 }) {
            const auto ys{ reader.RangeColumnMajorInt32(i, 5) };
            REQUIRE(ys == xs);
        }
    }

    std::filesystem::remove(temporary);
}


TEST_CASE("read write column major scaled", "[consistency] [read] [write]") {
    const std::vector<ctk::Electrode> electrodes{ { "fp1", "ref", "uV" }, { "fp2", "ref", "uV" } };
    const auto start{ std::chrono::system_clock::now() };
    ctk::TimeSeries header{ start, 2048, electrodes, 1024 };

    const std::vector<double> xs{ 11, 12, 21, 22, 31, 32, 41, 42, 51, 52 };

    std::filesystem::path temporary{ "rw_cms.cnt" };
    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        writer.ParamEeg(header);

        writer.RangeColumnMajor(xs);
        writer.RangeColumnMajor(xs);
        writer.RangeColumnMajor(xs);

        writer.Close();
    }

    {
        ctk::CntReaderReflib reader{ temporary };
        REQUIRE(reader.ParamEeg() == header);

        for (int64_t i : { 0, 5, 10 }) {
            const auto ys{ reader.RangeColumnMajor(i, 5) };
            REQUIRE(ys == xs);
        }
    }

    std::filesystem::remove(temporary);
}


TEST_CASE("read write row major", "[consistency] [read] [write]") {
    const auto start{ std::chrono::system_clock::now() };
    const std::vector<ctk::Electrode> electrodes{
        { "fp1", "ref", "uV" },
        { "fp2", "ref", "uV" },
        { "fp3", "ref", "uV" },
        { "fp4", "ref", "uV" } };
    ctk::TimeSeries header{ start, 2048, electrodes, 1024 };

    const std::vector<int32_t> xs{
        11, 12, 13,
        21, 22, 23,
        31, 32, 33,
        41, 42, 43 };

    std::filesystem::path temporary{ "rw_rm.cnt" };
    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        writer.ParamEeg(header);

        writer.RangeRowMajorInt32(xs);
        writer.RangeRowMajorInt32(xs);
        writer.RangeRowMajorInt32(xs);

        writer.Close();
    }

    {
        ctk::CntReaderReflib reader{ temporary };
        REQUIRE(reader.ParamEeg() == header);

        for (int64_t i : { 0, 3, 6 }) {
            const auto ys{ reader.RangeRowMajorInt32(i, 3) };
            REQUIRE(ys == xs);
        }
    }

    std::filesystem::remove(temporary);
}


TEST_CASE("read write row major scaled", "[consistency] [read] [write]") {
    const auto start{ std::chrono::system_clock::now() };
    const std::vector<ctk::Electrode> electrodes{
        { "fp1", "ref", "uV" },
        { "fp2", "ref", "uV" },
        { "fp3", "ref", "uV" },
        { "fp4", "ref", "uV" } };
    ctk::TimeSeries header{ start, 2048, electrodes, 1024 };

    const std::vector<double> xs{
        11, 12, 13,
        21, 22, 23,
        31, 32, 33,
        41, 42, 43 };

    std::filesystem::path temporary{ "rw_rms.cnt" };
    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        writer.ParamEeg(header);

        writer.RangeRowMajor(xs);
        writer.RangeRowMajor(xs);
        writer.RangeRowMajor(xs);

        writer.Close();
    }

    {
        ctk::CntReaderReflib reader{ temporary };
        REQUIRE(reader.ParamEeg() == header);

        for (int64_t i : { 0, 3, 6 }) {
            const auto ys{ reader.RangeRowMajor(i, 3) };
            REQUIRE(ys == xs);
        }
    }

    std::filesystem::remove(temporary);
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */



