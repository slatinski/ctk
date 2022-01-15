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

#include "api_reflib.h"
#include "container/file_reflib.h"

#include <exception>
#include <algorithm>
#include <iostream>


namespace ctk { namespace api {
    namespace v1 {

        using namespace ctk::impl;

        auto to_api(const time_signal& x) -> TimeSignal {
            //const measurement_count::value_type epoch_length{ x.epoch_length };

            TimeSignal result;
            result.epoch_length = x.ts.epoch_length;
            result.sampling_frequency = x.ts.sampling_frequency;
            result.start_time = api::timepoint2dcdate(x.ts.start_time);
            result.electrodes = x.ts.electrodes;
            return result;
        }

        auto from_api(const TimeSignal& x) -> time_signal {
            time_signal result;
            result.ts.epoch_length = x.epoch_length;
            result.ts.sampling_frequency = x.sampling_frequency;
            result.ts.start_time = api::dcdate2timepoint(x.start_time);
            result.ts.electrodes = x.electrodes;
            return result;
        }


        struct CntReaderReflib::impl
        {
            cnt_reader_reflib_riff reader;

            explicit
            impl(const std::filesystem::path& fname, bool is_broken)
            : reader{ fname, is_broken } {
            }
        };

        CntReaderReflib::CntReaderReflib(const std::filesystem::path& fname, bool is_broken)
            : p{ new impl{ fname, is_broken } } {
            assert(p);
        }

        CntReaderReflib::~CntReaderReflib() {
        }

        CntReaderReflib::CntReaderReflib(const CntReaderReflib& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        CntReaderReflib::CntReaderReflib(CntReaderReflib&& x)
        : p{ std::move(x.p) } {
            assert(p);
        }

        auto swap(CntReaderReflib& x, CntReaderReflib& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CntReaderReflib::operator=(const CntReaderReflib& x) -> CntReaderReflib& {
            CntReaderReflib y{ x };
            swap(*this, y);
            return *this;
        }

        auto CntReaderReflib::sampleCount() const -> int64_t {
            assert(p);
            const measurement_count::value_type result{ p->reader.sample_count() };
            return result;
        }

        auto CntReaderReflib::rangeRowMajor(int64_t i, int64_t samples) -> std::vector<int32_t> {
            assert(p);
            return p->reader.range_row_major(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::rangeColumnMajor(int64_t i, int64_t samples) -> std::vector<int32_t> {
            assert(p);
            return p->reader.range_column_major(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::rangeScaled(int64_t i, int64_t samples) -> std::vector<float> {
            assert(p);
            return p->reader.range_scaled(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::epochs() const -> int64_t {
            assert(p);
            const sint result{ p->reader.epochs() };
            return result;
        }

        auto CntReaderReflib::epochRowMajor(int64_t i) -> std::vector<int32_t> {
            assert(p);
            return p->reader.epoch_row_major(epoch_count{ i });
        }

        auto CntReaderReflib::epochColumnMajor(int64_t i) -> std::vector<int32_t> {
            assert(p);
            return p->reader.epoch_column_major(epoch_count{ i });
        }

        auto CntReaderReflib::epochCompressed(int64_t i) -> std::vector<uint8_t> {
            assert(p);
            return p->reader.epoch_compressed(epoch_count{ i });
        }

        auto CntReaderReflib::description() const -> TimeSignal {
            assert(p);
            return to_api(p->reader.description());
        }

        auto CntReaderReflib::cntType() const -> RiffType {
            assert(p);
            return p->reader.cnt_type();
        }

        auto CntReaderReflib::history() const -> std::string {
            assert(p);
            return p->reader.history();
        }

        auto CntReaderReflib::triggers() const -> std::vector<Trigger> {
            assert(p);
            return p->reader.triggers();
        }

        auto CntReaderReflib::information() const -> Info {
            assert(p);
            return p->reader.information();
        }

        auto CntReaderReflib::fileVersion() const -> FileVersion {
            assert(p);
            return p->reader.file_version();
        }

        auto CntReaderReflib::embeddedFiles() const -> std::vector<UserFile> {
            assert(p);
            const auto labels{ p->reader.embedded_files() };
            std::vector<UserFile> result(labels.size());
            std::transform(begin(labels), end(labels), begin(result), [](const std::string& s) -> UserFile { return { s, "" }; });
            return result;
        }

        auto CntReaderReflib::extractEmbeddedFile(const UserFile& x) const -> void {
            assert(p);
            p->reader.extract_embedded_file(x.label, x.file_name);
        }




        struct CntWriterReflib::impl
        {
            cnt_writer_reflib_riff writer;
            cnt_writer_reflib_flat *raw3;

            impl(const std::filesystem::path& fname, RiffType riff, const std::string& history)
            : writer{ fname, riff, history }
            , raw3{ nullptr } {
            }
        };

        CntWriterReflib::CntWriterReflib(const std::filesystem::path& fname, RiffType riff, const std::string& history)
            : p{ new impl{ fname, riff, history } } {
            assert(p);
        }

        CntWriterReflib::CntWriterReflib(CntWriterReflib&& x)
        : p{ std::move(x.p) } {
            assert(p);
        }

        auto CntWriterReflib::operator=(CntWriterReflib&& x) -> CntWriterReflib& {
            p = std::move(x.p);
            assert(p);
            return *this;
        }

        CntWriterReflib::~CntWriterReflib() {
        }

        auto CntWriterReflib::close() -> void {
            assert(p);
            if (!p->raw3) {
                return;
            }

            p->writer.close();
            p->raw3 = nullptr;
        }

        auto CntWriterReflib::recordingInfo(const Info& information) -> void {
            assert(p);
            p->writer.recording_info(information);
        }

        auto CntWriterReflib::addTimeSignal(const TimeSignal& description) -> bool {
            assert(p);
            if (p->raw3) {
                return false; // one segment only
            }

            p->raw3 = p->writer.add_time_signal(from_api(description));
            return p->raw3 != nullptr;
        }

        auto CntWriterReflib::rangeColumnMajor(const std::vector<int32_t>& client) -> void {
            assert(p);
            if (!p->raw3) {
                throw ctk_limit{ "CntWriterReflib::rangeColumnMajor: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->range_column_major(client);
        }

        auto CntWriterReflib::rangeRowMajor(const std::vector<int32_t>& client) -> void {
            assert(p);
            if (!p->raw3) {
                throw ctk_limit{ "CntWriterReflib::rangeRowMajor: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->range_row_major(client);
        }

        auto CntWriterReflib::trigger(const Trigger& x) -> void {
            assert(p);
            if (!p->raw3) {
                throw ctk_limit{ "CntWriterReflib::trigger: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->trigger(x);
        }

        auto CntWriterReflib::triggers(const std::vector<Trigger>& triggers) -> void {
            assert(p);
            if (!p->raw3) {
                throw ctk_limit{ "CntWriterReflib::triggers: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->triggers(triggers);
        }

        auto CntWriterReflib::flush() -> void {
            assert(p);
            p->writer.flush();
        }

        auto CntWriterReflib::embed(const UserFile& x) -> void {
            assert(p);
            p->writer.embed(x.label, x.file_name);
        }

        auto CntWriterReflib::commited() const -> int64_t {
            assert(p);
            const sint result{ p->writer.commited() };
            return result;
        }

        auto CntWriterReflib::rangeRowMajor(int64_t i, int64_t samples) -> std::vector<int32_t> {
            assert(p);
            const measurement_count n{ i };
            const measurement_count s{ samples };
            return p->writer.range_row_major(n, s);
        }

        auto CntWriterReflib::rangeColumnMajor(int64_t i, int64_t samples) -> std::vector<int32_t> {
            assert(p);
            const measurement_count n{ i };
            const measurement_count s{ samples };
            return p->writer.range_column_major(n, s);
        }

    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */


