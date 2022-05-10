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

        writer.ColumnMajorInt32(xs);
        writer.ColumnMajorInt32(xs);
        writer.ColumnMajorInt32(xs);

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

        writer.ColumnMajor(xs);
        writer.ColumnMajor(xs);
        writer.ColumnMajor(xs);

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

        writer.RowMajorInt32(xs);
        writer.RowMajorInt32(xs);
        writer.RowMajorInt32(xs);

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

        writer.RowMajor(xs);
        writer.RowMajor(xs);
        writer.RowMajor(xs);

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


TEST_CASE("writer close", "[consistency] [write]") {
    // close is callable at any time
    std::filesystem::path temporary{ "test_api_reflib_writer_close.cnt" };
    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        writer.Close();
        REQUIRE(writer.IsClosed());
    }
    REQUIRE(!std::filesystem::exists(temporary));

    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        writer.RecordingInfo(ctk::Info{});
        writer.Close();
        REQUIRE(writer.IsClosed());
    }
    REQUIRE(!std::filesystem::exists(temporary));

    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        ctk::TimeSeries x;
        REQUIRE_THROWS(writer.ParamEeg(x));
        x.StartTime = std::chrono::system_clock::now();
        x.Electrodes.emplace_back("1", "ref");
        x.SamplingFrequency = 1;
        writer.ParamEeg(x);
        writer.Close();
        REQUIRE(writer.IsClosed());
    }
    REQUIRE(!std::filesystem::exists(temporary));

    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        REQUIRE_THROWS(writer.AddTrigger({ 0, "0" }));
        REQUIRE_THROWS(writer.AddTriggers({ { 0, "0" }, { 0, "1" } }));
        REQUIRE_THROWS(writer.ColumnMajor({ 0 }));
        REQUIRE_THROWS(writer.Flush());
        writer.History("");
        writer.Close();
        REQUIRE(writer.IsClosed());
    }
    REQUIRE(!std::filesystem::exists(temporary));

    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        ctk::TimeSeries x;
        x.StartTime = std::chrono::system_clock::now();
        x.Electrodes.emplace_back("1", "ref");
        x.SamplingFrequency = 1;
        writer.ParamEeg(x);
        writer.ColumnMajor({ 0 });
        writer.Close();
        REQUIRE(writer.IsClosed());
    }
    REQUIRE(std::filesystem::exists(temporary));
    std::filesystem::remove(temporary);

    // no function is callable after close
    {
        ctk::CntWriterReflib writer{ temporary, RiffType::riff64 };
        writer.Close();
        REQUIRE(writer.IsClosed());

        ctk::TimeSeries x;
        x.StartTime = std::chrono::system_clock::now();
        x.Electrodes.emplace_back("1", "ref");
        x.SamplingFrequency = 1;
        REQUIRE_THROWS(writer.ParamEeg(x));
        REQUIRE_THROWS(writer.RecordingInfo(ctk::Info{}));
        REQUIRE_THROWS(writer.ColumnMajor({ 0 }));
        REQUIRE_THROWS(writer.AddTrigger({ 0, "0" }));
        REQUIRE_THROWS(writer.AddTriggers({ { 0, "0" }, { 0, "1" } }));
        REQUIRE_THROWS(writer.Flush());
        REQUIRE_THROWS(writer.History(""));
    }
    REQUIRE(!std::filesystem::exists(temporary));
}


TEST_CASE("odd input", "[consistency] [write]") {
    REQUIRE_THROWS(ctk::CntWriterReflib{ "", RiffType::riff32 });
    REQUIRE_THROWS(ctk::CntWriterReflib{ "", RiffType::riff64 });
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */



