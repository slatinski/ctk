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

#include <exception>
#include <algorithm>
#include <iostream>

#include "api_reflib.h"
#include "container/file_reflib.h"
#include "evt/event_lib.h"


namespace ctk { namespace api {
    namespace v1 {

        using namespace ctk::impl;

        auto to_api(const time_signal& x) -> TimeSeries {
            return x.ts;
        }

        auto from_api(const TimeSeries& x) -> time_signal {
            return time_signal{ x };
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

        auto CntReaderReflib::description() const -> TimeSeries {
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

        auto CntWriterReflib::addTimeSignal(const TimeSeries& description) -> bool {
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
            const measurement_count::value_type result{ p->writer.commited() };
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





        struct EventReader::impl
        {
            event_library lib;

            impl(const std::filesystem::path& file_name)
            : lib{ ctk::impl::read_archive(open_r(file_name).get()) } {
            }

            impl(const event_library& x)
            : lib{ x } {
            }
        };


        static auto is_impedance(const event_descriptor& x) -> bool { return x.name == descriptor_name::impedance; }
        static auto is_condition_label(const event_descriptor& x) -> bool { return x.name == descriptor_name::condition; }
        static auto is_event_code(const event_descriptor& x) -> bool { return x.name == descriptor_name::event_code; }
        static auto is_video_file(const event_descriptor& x) -> bool { return x.name == descriptor_name::video_file_name; }
        static auto ohm2kohm(float x) -> float { return x / std::kilo::num; }
        static auto kohm2ohm(float x) -> float { return x * std::kilo::num; }



        static
        auto marker2impedance(const marker_event& x) -> EventImpedance {
            const auto first{ begin(x.common.descriptors) };
            const auto last{ end(x.common.descriptors) };

            const auto i_impedance{ std::find_if(first, last, is_impedance) };
            if (i_impedance == last || !is_float_array(i_impedance->value)) {
                throw api::v1::ctk_bug{ "marker2impedance: no impedance descriptor" };
            }
            const std::vector<float> impedances{ as_float_array(i_impedance->value) };

            EventImpedance result;
            result.values.resize(impedances.size());
            std::transform(begin(impedances), end(impedances), begin(result.values), kohm2ohm);

            result.stamp = x.common.stamp;
            return result;
        }

        static
        auto impedance2marker(const EventImpedance& x) -> marker_event {
            std::vector<float> impedance(x.values.size());
            std::transform(begin(x.values), end(x.values), begin(impedance), ohm2kohm);

            const event_descriptor descriptor{ str_variant{ impedance }, descriptor_name::impedance, "kOhm" };
            const base_event common{ x.stamp, event_type::marker, event_name::marker, { descriptor }, 0, 0 };
            return marker_event{ common, event_description::impedance };
        }


        static
        auto marker2video(const marker_event& x) -> EventVideo {
            EventVideo result;
            const auto first{ begin(x.common.descriptors) };
            const auto last{ end(x.common.descriptors) };

            const auto i_condition_label{ std::find_if(first, last, is_condition_label) };
            if (i_condition_label != last && is_wstring(i_condition_label->value)) {
                result.condition_label = as_wstring(i_condition_label->value);
            }

            const auto i_event_code{ std::find_if(first, last, is_event_code) };
            if (i_event_code != last && is_int32(i_event_code->value)) {
                result.trigger_code = as_int32(i_event_code->value);
            }

            const auto i_video_file{ std::find_if(first, last, is_video_file) };
            if (i_video_file != last && is_wstring(i_video_file->value)) {
                result.video_file = as_wstring(i_video_file->value);
            }

            result.description = x.description;
            result.duration = x.common.duration;
            result.stamp = x.common.stamp;
            return result;
        }

        static
        auto video2marker(const EventVideo& x) -> marker_event {
            const std::string unit;
            std::vector<event_descriptor> descriptors;
            descriptors.reserve(4);

            // compatibility: if present, the condition descriptor must be first
            if (!x.condition_label.empty()) {
                descriptors.emplace_back(str_variant{ x.condition_label }, descriptor_name::condition, unit);
            }

            if (x.trigger_code != std::numeric_limits<int32_t>::min()) {
                descriptors.emplace_back(str_variant{ x.trigger_code }, descriptor_name::event_code, unit);
            }

            descriptors.emplace_back(str_variant{ video_marker_type::recording }, descriptor_name::video_marker_type, unit);

            if (!x.video_file.empty()) {
                descriptors.emplace_back(str_variant{ x.video_file }, descriptor_name::video_file_name, unit);
            }

            const base_event common{ x.stamp, event_type::marker, event_name::marker, descriptors, x.duration, 0 };
            return marker_event{ common, x.description };
        }


        static
        auto epochevent2eventepoch(const epoch_event& x) -> EventEpoch {
            EventEpoch result;
            const auto first{ begin(x.common.descriptors) };
            const auto last{ end(x.common.descriptors) };

            const auto i_event_code{ std::find_if(first, last, is_event_code) };
            if (i_event_code != last && is_int32(i_event_code->value)) {
                result.trigger_code = as_int32(i_event_code->value);
            }

            const auto i_condition_label{ std::find_if(first, last, is_condition_label) };
            if (i_condition_label != last && is_wstring(i_condition_label->value)) {
                result.condition_label = as_wstring(i_condition_label->value);
            }

            result.duration = x.common.duration;
            result.offset = x.common.duration_offset;
            result.stamp = x.common.stamp;
            return result;
        }

        static
        auto eventepoch2epochevent(const EventEpoch& x) -> epoch_event {
            const std::string unit;
            std::vector<event_descriptor> descriptors;
            descriptors.reserve(2);

            // compatibility: if present, the condition descriptor must be first
            if (!x.condition_label.empty()) {
                descriptors.emplace_back(str_variant{ x.condition_label }, descriptor_name::condition, unit);
            }

            if (x.trigger_code != std::numeric_limits<int32_t>::min()) {
                descriptors.emplace_back(str_variant{ x.trigger_code }, descriptor_name::event_code, unit);
            }

            const base_event common{ x.stamp, event_type::epoch, event_name::epoch, descriptors, x.duration, x.offset };
            return epoch_event{ common };
        }



        auto swap(EventReader& x, EventReader& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        EventReader::EventReader(const std::filesystem::path& file_name)
        : p{ new impl{  file_name } } {
            assert(p);
        }

        EventReader::EventReader(const EventReader& x) {
            assert(x.p);
            p.reset(new impl{ x.p->lib });
            assert(p);
        }

        EventReader::EventReader(EventReader&& x)
        : p{ std::move(x.p) }
        {
            assert(p);
        }

        EventReader::~EventReader() {
        }

        auto EventReader::operator=(const EventReader& x) -> EventReader& {
            EventReader y{ x };
            swap(*this, y);
            return *this;
        }

        auto EventReader::operator=(EventReader&& x) -> EventReader& {
            p = std::move(x.p);
            assert(p);
            return *this;
        }

        auto EventReader::impedanceCount() const -> size_t {
            assert(p);
            return p->lib.impedances.size();
        }

        auto EventReader::videoCount() const -> size_t {
            assert(p);
            return p->lib.videos.size();
        }

        auto EventReader::epochCount() const -> size_t {
            assert(p);
            return p->lib.epochs.size();
        }

        auto EventReader::impedanceEvent(size_t i) -> EventImpedance {
            assert(p);
            if (p->lib.impedances.size() <= i) {
                std::ostringstream oss;
                oss << "EventReader::impedanceEvent: invalid index " << i << "/" << (p->lib.impedances.size() - 1);
                throw api::v1::ctk_limit{ oss.str() };
            }

            return marker2impedance(p->lib.impedances[i]);
        }

        auto EventReader::videoEvent(size_t i) -> EventVideo {
            assert(p);
            if (p->lib.videos.size() <= i) {
                std::ostringstream oss;
                oss << "EventReader::videoEvent: invalid index " << i << "/" << (p->lib.videos.size() - 1);
                throw api::v1::ctk_limit{ oss.str() };
            }

            return marker2video(p->lib.videos[i]);
        }

        auto EventReader::epochEvent(size_t i) -> EventEpoch {
            assert(p);
            if (p->lib.epochs.size() <= i) {
                std::ostringstream oss;
                oss << "EventReader::epochEvent: invalid index " << i << "/" << (p->lib.epochs.size() - 1);
                throw api::v1::ctk_limit{ oss.str() };
            }

            return epochevent2eventepoch(p->lib.epochs[i]);
        }

        auto EventReader::impedanceEvents() -> std::vector<EventImpedance> {
            assert(p);

            const auto& events{ p->lib.impedances };
            std::vector<EventImpedance> xs(events.size());
            std::transform(begin(events), end(events), begin(xs), marker2impedance);
            return xs;
        }

        auto EventReader::videoEvents() -> std::vector<EventVideo> {
            assert(p);

            const auto& events{ p->lib.videos };
            std::vector<EventVideo> xs(events.size());
            std::transform(begin(events), end(events), begin(xs), marker2video);
            return xs;
        }

        auto EventReader::epochEvents() -> std::vector<EventEpoch> {
            assert(p);

            const auto& events{ p->lib.epochs };
            std::vector<EventEpoch> xs(events.size());
            std::transform(begin(events), end(events), begin(xs), epochevent2eventepoch);
            return xs;
        }

        auto EventReader::fileVersion() -> int32_t {
            assert(p);
            return p->lib.version;
        }




        struct EventWriter::impl
        {
            event_library lib;
        };


        EventWriter::EventWriter()
            : p{ new impl } {
            assert(p);
        }


        EventWriter::EventWriter(EventWriter&& x)
        : p{ std::move(x.p) } {
            assert(p);
        }


        auto EventWriter::operator=(EventWriter&& x) -> EventWriter& {
            p = std::move(x.p);
            assert(p);
            return *this;
        }


        EventWriter::~EventWriter() {
        }


        auto EventWriter::addImpedance(const EventImpedance& x) -> void {
            add_impedance(impedance2marker(x), p->lib);
        }


        auto EventWriter::addVideo(const EventVideo& x) -> void {
            add_video(video2marker(x), p->lib);
        }


        auto EventWriter::addEpoch(const EventEpoch& x) -> void {
            add_epoch(eventepoch2epochevent(x), p->lib);
        }


        auto EventWriter::eventCount() const -> size_t {
            return ctk::impl::event_count(p->lib, size_t{});
        }

        auto EventWriter::fileVersion(int32_t version) -> void {
            if (177 < version) {
                throw api::v1::ctk_limit{ "EventWriter::file_version: not implemented" };
            }

            if (version <= 0) {
                p->lib.version = event_library::default_output_file_version();
            }
            else {
                p->lib.version = version;
            }
        }

        auto EventWriter::write(const std::filesystem::path &fname) -> void {
            file_ptr f{ open_w(fname) };
            write_archive(f.get(), p->lib);
        }

    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */


