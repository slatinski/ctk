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

#include "api_reflib.h"

namespace ctk { namespace api {
    namespace v1 {

        struct WriterReflib
        {
            TimeSeries ParamEeg;
            Info RecordingInfo;

            explicit WriterReflib(const std::filesystem::path&, RiffType = RiffType::riff64);
            WriterReflib(const WriterReflib&) = delete;
            WriterReflib(WriterReflib&&);
            auto operator=(const WriterReflib&) -> WriterReflib& = delete;
            auto operator=(WriterReflib&&) -> WriterReflib&;
            ~WriterReflib();

            auto Close() -> void;

            auto cnt_ptr() -> CntWriterReflib*;
            auto evt_ptr() -> EventWriter*;

            auto Phase() const -> WriterPhase;

        private:
            struct impl;
            std::unique_ptr<impl> p;
        };


        struct ReaderReflib
        {
            int64_t SampleCount;
            int64_t EpochCount;
            v1::RiffType Type;
            v1::TimeSeries ParamEeg;
            v1::Info RecordingInfo;
            std::vector<Trigger> Triggers;
            std::vector<EventImpedance> Impedances;
            std::vector<EventVideo> Videos;
            std::vector<EventEpoch> Epochs;
            std::vector<UserFile> Embedded;

            explicit ReaderReflib(const std::filesystem::path&);
            ReaderReflib(const ReaderReflib&);
            ReaderReflib(ReaderReflib&&);
            auto operator=(const ReaderReflib&) -> ReaderReflib&;
            auto operator=(ReaderReflib&&) -> ReaderReflib&;
            ~ReaderReflib();

            auto RangeColumnMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto RangeRowMajor(int64_t i, int64_t samples) -> std::vector<double>;
            auto RangeV4(int64_t i, int64_t samples) -> std::vector<float>;
            auto RangeColumnMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;
            auto RangeRowMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t>;

            auto EpochColumnMajor(int64_t) -> std::vector<double>;
            auto EpochRowMajor(int64_t) -> std::vector<double>;
            auto EpochCompressed(int64_t) -> std::vector<uint8_t>;

            auto ExtractEmbedded(const v1::UserFile&) -> void;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(ReaderReflib&, ReaderReflib&) -> void;
        };



    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */


