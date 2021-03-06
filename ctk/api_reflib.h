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

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>
#include <memory>
#include <filesystem>
#include "exception.h"
#include "api_data.h"

namespace ctk { namespace api {
    namespace v1 {

        struct CntReaderReflib
        {
            CntReaderReflib(const std::filesystem::path& fname);
            CntReaderReflib(const CntReaderReflib&);
            CntReaderReflib(CntReaderReflib&&);
            auto operator=(const CntReaderReflib&) -> CntReaderReflib&;
            auto operator=(CntReaderReflib&&) -> CntReaderReflib& = default;
            ~CntReaderReflib();

            auto SampleCount() const -> int64_t;

            /*
            range interface: returns variable amount of samples
            the output is in row major format.
            used internally by libeep and by this library.
            vector<int32_t> output {
                11, 12, 13, 14, // sample data at time points 1, 2, 3 and 4 for sensor 1
                21, 22, 23, 24, // sample data at time points 1, 2, 3 and 4 for sensor 2
                31, 32, 33, 34  // sample data at time points 1, 2, 3 and 4 for sensor 3
            } */
            auto RangeRowMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto RangeRowMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;

            /*
            the output is in column major format.
            used by the libeep interface.
            vector<int32_t> output {
                11, 21, 31, // measurement at time point 1: sample data for sensors 1, 2 and 3
                12, 22, 32, // measurement at time point 2: sample data for sensors 1, 2 and 3
                13, 23, 33, // measurement at time point 3: sample data for sensors 1, 2 and 3
                14, 24, 34  // measurement at time point 4: sample data for sensors 1, 2 and 3
            } */
            auto RangeColumnMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto RangeColumnMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;

            // libeep v4 interface: column major, float
            auto RangeV4(int64_t i, int64_t samples) -> std::vector<float>;

            /*
            epoch interface:
            omits the intermediate buffer used to collect samples into the variable sized output
            requested by the user (as range) out of "epoch length" sized epochs.
            this interface delivers data in the following fashion:
                - all output epochs but the last contain epoch length measurements
                - the last epoch might be shorter
            */
            auto Epochs() const -> int64_t;
            auto EpochRowMajor(int64_t i) -> std::vector<double>;
            auto EpochColumnMajor(int64_t i) -> std::vector<double>;
            auto EpochRowMajorInt32(int64_t i) -> std::vector<int32_t>;
            auto EpochColumnMajorInt32(int64_t i) -> std::vector<int32_t>;
            auto EpochCompressed(int64_t i) -> std::vector<uint8_t>;

            auto ParamEeg() const -> TimeSeries;
            auto CntType() const -> RiffType;
            auto History() const -> std::string;

            auto Triggers() const -> std::vector<Trigger>;
            auto RecordingInfo() const -> Info;
            auto CntFileVersion() const -> FileVersion;

            auto EmbeddedFiles() const -> std::vector<UserFile>;
            auto ExtractEmbeddedFile(const UserFile&) const -> void;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CntReaderReflib&, CntReaderReflib&) -> void;
        };


        struct CntReaderReflibUnpacked
        {
            CntReaderReflibUnpacked(const std::filesystem::path& fname);
            CntReaderReflibUnpacked(const CntReaderReflibUnpacked&);
            CntReaderReflibUnpacked(CntReaderReflibUnpacked&&);
            auto operator=(const CntReaderReflibUnpacked&) -> CntReaderReflibUnpacked&;
            auto operator=(CntReaderReflibUnpacked&&) -> CntReaderReflibUnpacked& = default;
            ~CntReaderReflibUnpacked();

            auto SampleCount() const -> int64_t;

            /*
            range interface: returns variable amount of samples
            the output is in row major format.
            used internally by libeep and by this library.
            vector<int32_t> output {
                11, 12, 13, 14, // sample data at time points 1, 2, 3 and 4 for sensor 1
                21, 22, 23, 24, // sample data at time points 1, 2, 3 and 4 for sensor 2
                31, 32, 33, 34  // sample data at time points 1, 2, 3 and 4 for sensor 3
            } */
            auto RangeRowMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto RangeRowMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;

            /*
            the output is in column major format.
            used by the libeep interface.
            vector<int32_t> output {
                11, 21, 31, // measurement at time point 1: sample data for sensors 1, 2 and 3
                12, 22, 32, // measurement at time point 2: sample data for sensors 1, 2 and 3
                13, 23, 33, // measurement at time point 3: sample data for sensors 1, 2 and 3
                14, 24, 34  // measurement at time point 4: sample data for sensors 1, 2 and 3
            } */
            auto RangeColumnMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto RangeColumnMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;

            // libeep v4 interface: column major, float
            auto RangeV4(int64_t i, int64_t samples) -> std::vector<float>;

            /*
            epoch interface:
            omits the intermediate buffer used to collect samples into the variable sized output
            requested by the user (as range) out of "epoch length" sized epochs.
            this interface delivers data in the following fashion:
                - all output epochs but the last contain epoch length measurements
                - the last epoch might be shorter
            */
            auto Epochs() const -> int64_t;
            auto EpochRowMajor(int64_t i) -> std::vector<double>;
            auto EpochColumnMajor(int64_t i) -> std::vector<double>;
            auto EpochRowMajorInt32(int64_t i) -> std::vector<int32_t>;
            auto EpochColumnMajorInt32(int64_t i) -> std::vector<int32_t>;
            auto EpochCompressed(int64_t i) -> std::vector<uint8_t>;

            auto ParamEeg() const -> TimeSeries;
            auto CntType() const -> RiffType;
            auto History() const -> std::string;

            auto Triggers() const -> std::vector<Trigger>;
            auto RecordingInfo() const -> Info;
            auto CntFileVersion() const -> FileVersion;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CntReaderReflibUnpacked&, CntReaderReflibUnpacked&) -> void;
        };


        enum class WriterPhase{ Setup, Writing, Closed };
        auto operator<<(std::ostream&, WriterPhase) -> std::ostream&;
        auto validate_writer_phase(WriterPhase x, WriterPhase expected, const std::string& func) -> void;

        struct CntWriterReflib
        {
            CntWriterReflib(const std::filesystem::path&, RiffType);
            CntWriterReflib(const CntWriterReflib&) = delete;
            CntWriterReflib(CntWriterReflib&&);
            auto operator=(const CntWriterReflib&) -> CntWriterReflib& = delete;
            auto operator=(CntWriterReflib&&) -> CntWriterReflib&;
            ~CntWriterReflib();

            // NB: the output file construction happens here.
            // assemblies the generated files into a single RIFF file.
            // Close is the last function to call before destroying the object.
            auto Close() -> void;
            auto IsClosed() const -> bool; // can be called after Close

            auto RecordingInfo(const Info&) -> void;
            auto ParamEeg(const TimeSeries&) -> void;

            /*
            the input is in column major format.
            used by the libeep interface.
            vector<int32_t> client {
                11, 21, 31, // measurement at time point 1: sample data for sensors 1, 2 and 3
                12, 22, 32, // measurement at time point 2: sample data for sensors 1, 2 and 3
                13, 23, 33, // measurement at time point 3: sample data for sensors 1, 2 and 3
                14, 24, 34  // measurement at time point 4: sample data for sensors 1, 2 and 3
            } */
            auto ColumnMajor(const std::vector<double>&) -> void;
            auto ColumnMajorInt32(const std::vector<int32_t>&) -> void;

            /*
            the input is in row major format.
            used internally by libeep and by this library.
            vector<int32_t> client {
                11, 12, 13, 14, // sample data at time points 1, 2, 3 and 4 for sensor 1
                21, 22, 23, 24, // sample data at time points 1, 2, 3 and 4 for sensor 2
                31, 32, 33, 34  // sample data at time points 1, 2, 3 and 4 for sensor 3
            } */
            auto RowMajor(const std::vector<double>&) -> void;
            auto RowMajorInt32(const std::vector<int32_t>&) -> void;

            // libeep v4 interface: column major, float
            auto LibeepV4(const std::vector<float>&) -> void;

            auto AddTrigger(const Trigger&) -> void;
            auto AddTriggers(const std::vector<Trigger>&) -> void;

            auto History(const std::string&) -> void;

            auto Flush() -> void;

            // compatible extension: user supplied content placed into a top-level chunk.
            // UserFile::file_name should exist and be accessible when Close is called.
            // UserFile::label is a 4 byte label.
            // UserFile::label can not be any of: "eeph", "info", "evt ", "raw3",
            // "rawf", "stdd", "tfh ", "tfd ", "refh", "imp ", "nsh ", "vish", "egih",
            // "egig", "egiz", "binh", "xevt", "xseg", "xsen", "xtrg".
            // a label can not be used for more than one user supplied chunk.
            auto Embed(const UserFile&) -> void;

            // reader functionality (completely untested, especially for unsynchronized reads during writing - the intended use case).
            // these functions will not work without application-side synchronization.
            auto Commited() const -> int64_t;
            auto ReadRowMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto ReadColumnMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto ReadRowMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;
            auto ReadColumnMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
        };


        struct EventReader
        {
            explicit EventReader(const std::filesystem::path&);
            EventReader(const EventReader&);
            EventReader(EventReader&&);
            auto operator=(const EventReader&) -> EventReader&;
            auto operator=(EventReader&&) -> EventReader&;
            ~EventReader();

            auto ImpedanceCount() const -> size_t;
            auto VideoCount() const -> size_t;
            auto EpochCount() const -> size_t;

            auto ImpedanceEvent(size_t) -> EventImpedance;
            auto VideoEvent(size_t) -> EventVideo;
            auto EpochEvent(size_t) -> EventEpoch;

            auto ImpedanceEvents() -> std::vector<EventImpedance>;
            auto VideoEvents() -> std::vector<EventVideo>;
            auto EpochEvents() -> std::vector<EventEpoch>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(EventReader&, EventReader&) -> void;
        };


        struct EventReaderUnpacked
        {
            explicit EventReaderUnpacked(const std::filesystem::path&);
            EventReaderUnpacked(const EventReaderUnpacked&);
            EventReaderUnpacked(EventReaderUnpacked&&);
            auto operator=(const EventReaderUnpacked&) -> EventReaderUnpacked&;
            auto operator=(EventReaderUnpacked&&) -> EventReaderUnpacked&;
            ~EventReaderUnpacked();

            auto ImpedanceCount() const -> size_t;
            auto VideoCount() const -> size_t;
            auto EpochCount() const -> size_t;

            auto ImpedanceEvent(size_t) -> EventImpedance;
            auto VideoEvent(size_t) -> EventVideo;
            auto EpochEvent(size_t) -> EventEpoch;

            auto ImpedanceEvents() -> std::vector<EventImpedance>;
            auto VideoEvents() -> std::vector<EventVideo>;
            auto EpochEvents() -> std::vector<EventEpoch>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(EventReaderUnpacked&, EventReaderUnpacked&) -> void;
        };



        struct EventWriter
        {
            explicit EventWriter(const std::filesystem::path& evt);
            EventWriter(const EventWriter&) = delete;
            EventWriter(EventWriter&&);
            auto operator=(const EventWriter&) -> EventWriter& = delete;
            auto operator=(EventWriter&&) -> EventWriter&;
            ~EventWriter();

            auto Close() -> bool; /* NB: the output file construction happens here */

            auto AddImpedance(const EventImpedance&) -> void;
            auto AddVideo(const EventVideo&) -> void;
            auto AddEpoch(const EventEpoch&) -> void;

            auto AddImpedances(const std::vector<EventImpedance>&) -> void;
            auto AddVideos(const std::vector<EventVideo>&) -> void;
            auto AddEpochs(const std::vector<EventEpoch>&) -> void;

        private:
            struct impl;
            std::unique_ptr<impl> p;
        };

    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */

