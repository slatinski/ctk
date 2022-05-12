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

#include "api_bindings.h"

#include "file/cnt_reflib.h"
#include "file/evt.h"
#include "exception"
#include "logger.h"


namespace ctk { namespace api {
    namespace v1 {

        using namespace ctk::impl;

        struct WriterReflib::impl
        {
            std::unique_ptr<CntWriterReflib> writer;
            EventWriter events;
            WriterPhase phase;

            impl(const std::filesystem::path& fname, RiffType cnt)
            : writer{ std::make_unique<CntWriterReflib>(fname, cnt) }
            , events{ std::filesystem::path{ fname }.replace_extension("evt") }
            , phase{ WriterPhase::Setup } {
                assert(writer);
            }

            auto initialize(const TimeSeries& param, const Info& recording_info) -> void {
                assert(writer);

                validate_writer_phase(phase, WriterPhase::Setup, "WriterReflib::initialize");
                validate(param);

                writer->ParamEeg(param);
                writer->RecordingInfo(recording_info);
                phase = WriterPhase::Writing;
            }
        };


        WriterReflib::WriterReflib(const std::filesystem::path& fname, RiffType riff)
            : p{ new impl{ fname, riff } } {
            assert(p);
        }

        WriterReflib::WriterReflib(WriterReflib&& x)
        : p{ std::move(x.p) } {
            assert(p);
        }

        auto WriterReflib::operator=(WriterReflib&& x) -> WriterReflib& {
            p = std::move(x.p);
            assert(p);
            return *this;
        }

        WriterReflib::~WriterReflib() {
        }


        auto WriterReflib::cnt_ptr() -> CntWriterReflib* {
            assert(p);

            if (p->phase == WriterPhase::Setup) {
                p->initialize(ParamEeg, RecordingInfo);
            }

            validate_writer_phase(p->phase, WriterPhase::Writing, "WriterReflib::cnt_ptr");

            return p->writer.get();
        }

        auto WriterReflib::evt_ptr() -> EventWriter* {
            assert(p);

            // strictly not needed.
            // but makes the interface usage more consistent and simplifies the implementation of Close.
            if (p->phase == WriterPhase::Setup) {
                p->initialize(ParamEeg, RecordingInfo);
            }

            validate_writer_phase(p->phase, WriterPhase::Writing, "WriterReflib::evt_ptr");

            return &p->events;
        }


        auto WriterReflib::Close() -> void {
            assert(p);

            if (p->phase == WriterPhase::Closed) {
                return;
            }

            p->events.Close();

            assert(p->writer);
            if (p->writer->IsClosed() == false) {
                p->writer->RecordingInfo(RecordingInfo);
                p->writer->Close();
            }

            p->writer.reset(nullptr);
            p->phase = WriterPhase::Closed;
        }

        auto WriterReflib::Phase() const -> WriterPhase {
            assert(p);

            return p->phase;
        }



        struct ReaderReflib::impl
        {
            CntReaderReflib reader;
            std::filesystem::path file_name;

            impl(const std::filesystem::path& fname)
            : reader{ fname }
            , file_name{ fname } {
            }
        };


        ReaderReflib::ReaderReflib(const std::filesystem::path& fname)
            : p{ new impl{ fname } } {
            assert(p);

            SampleCount = p->reader.SampleCount();
            EpochCount = p->reader.Epochs();
            Type = p->reader.CntType();
            ParamEeg = p->reader.ParamEeg();
            RecordingInfo = p->reader.RecordingInfo();
            Triggers = p->reader.Triggers();
            Embedded = p->reader.EmbeddedFiles();

            const auto evt_fname{ std::filesystem::path{ fname }.replace_extension("evt") };
            if (!std::filesystem::exists(evt_fname)) {
                return;
            }

            EventReader events{ evt_fname };
            Impedances = events.ImpedanceEvents();
            Videos = events.VideoEvents();
            Epochs = events.EpochEvents();
        }

        ReaderReflib::ReaderReflib(const ReaderReflib& x)
        : p{ new impl{ x.p->file_name } } {
            assert(p);
        }

        ReaderReflib::ReaderReflib(ReaderReflib&& x)
        : p{ std::move(x.p) } {
            assert(p);
        }

        auto swap(ReaderReflib& x, ReaderReflib& y) -> void {
            std::swap(x.p, y.p);
        }

        auto ReaderReflib::operator=(const ReaderReflib& x) -> ReaderReflib& {
            ReaderReflib y{ x };
            swap(*this, y);
            return *this;
        }

        auto ReaderReflib::operator=(ReaderReflib&& x) -> ReaderReflib& {
            p = std::move(x.p);
            assert(p);
            return *this;
        }

        ReaderReflib::~ReaderReflib() {
        }

        auto ReaderReflib::RangeColumnMajor(int64_t i, int64_t length) -> std::vector<double> {
            assert(p);
            return p->reader.RangeColumnMajor(i, length);
        }

        auto ReaderReflib::RangeRowMajor(int64_t i, int64_t length) -> std::vector<double> {
            assert(p);
            return p->reader.RangeRowMajor(i, length);
        }

        auto ReaderReflib::RangeV4(int64_t i, int64_t length) -> std::vector<float> {
            assert(p);
            return p->reader.RangeV4(i, length);
        }

        auto ReaderReflib::RangeColumnMajorInt32(int64_t i, int64_t length) -> std::vector<int32_t> {
            assert(p);
            return p->reader.RangeColumnMajorInt32(i, length);
        }

        auto ReaderReflib::RangeRowMajorInt32(int64_t i, int64_t length) -> std::vector<int32_t> {
            assert(p);
            return p->reader.RangeRowMajorInt32(i, length);
        }


        auto ReaderReflib::EpochColumnMajor(int64_t i) -> std::vector<double> {
            assert(p);
            return p->reader.EpochColumnMajor(i);
        }

        auto ReaderReflib::EpochRowMajor(int64_t i) -> std::vector<double> {
            assert(p);
            return p->reader.EpochRowMajor(i);
        }

        auto ReaderReflib::EpochCompressed(int64_t i) -> std::vector<uint8_t> {
            assert(p);
            return p->reader.EpochCompressed(i);
        }

        auto ReaderReflib::ExtractEmbedded(const v1::UserFile& x) -> void {
            assert(p);
            p->reader.ExtractEmbeddedFile(x);
        }

    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */



