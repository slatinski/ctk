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

#include "file/cnt_reflib.h"
#include "file/evt.h"
#include "exception"



namespace ctk { namespace api {
    namespace v1 {

        using namespace ctk::impl;


        struct CntReaderReflib::impl
        {
            cnt_reader_reflib_riff reader;

            explicit
            impl(const std::filesystem::path& fname)
            : reader{ fname } {
            }
        };

        CntReaderReflib::CntReaderReflib(const std::filesystem::path& fname)
            : p{ new impl{ fname } } {
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

        auto CntReaderReflib::SampleCount() const -> int64_t {
            assert(p);
            const measurement_count::value_type result{ p->reader.sample_count() };
            return result;
        }

        auto CntReaderReflib::RangeRowMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t> {
            assert(p);
            return p->reader.range_row_major(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::RangeRowMajor(int64_t i, int64_t samples) -> std::vector<double> {
            assert(p);
            return p->reader.range_row_major_scaled(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::RangeColumnMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t> {
            assert(p);
            return p->reader.range_column_major(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::RangeColumnMajor(int64_t i, int64_t samples) -> std::vector<double> {
            assert(p);
            return p->reader.range_column_major_scaled(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::RangeLibeep(int64_t i, int64_t samples) -> std::vector<float> {
            assert(p);
            return p->reader.range_libeep_v4(measurement_count{ i }, measurement_count{ samples });
        }

        auto CntReaderReflib::Epochs() const -> int64_t {
            assert(p);
            const epoch_count::value_type result{ p->reader.epochs() };
            return result;
        }

        auto CntReaderReflib::EpochRowMajorInt32(int64_t i) -> std::vector<int32_t> {
            assert(p);
            return p->reader.epoch_row_major(epoch_count{ i });
        }

        auto CntReaderReflib::EpochRowMajor(int64_t i) -> std::vector<double> {
            assert(p);
            return p->reader.epoch_row_major_scaled(epoch_count{ i });
        }

        auto CntReaderReflib::EpochColumnMajorInt32(int64_t i) -> std::vector<int32_t> {
            assert(p);
            return p->reader.epoch_column_major(epoch_count{ i });
        }

        auto CntReaderReflib::EpochColumnMajor(int64_t i) -> std::vector<double> {
            assert(p);
            return p->reader.epoch_column_major_scaled(epoch_count{ i });
        }

        auto CntReaderReflib::EpochCompressed(int64_t i) -> std::vector<uint8_t> {
            assert(p);
            return p->reader.epoch_compressed(epoch_count{ i });
        }

        auto CntReaderReflib::ParamEeg() const -> TimeSeries {
            assert(p);
            return p->reader.param_eeg();
        }

        auto CntReaderReflib::CntType() const -> RiffType {
            assert(p);
            return p->reader.cnt_type();
        }

        auto CntReaderReflib::History() const -> std::string {
            assert(p);
            return p->reader.history();
        }

        auto CntReaderReflib::Triggers() const -> std::vector<Trigger> {
            assert(p);
            return p->reader.triggers();
        }

        auto CntReaderReflib::RecordingInfo() const -> Info {
            assert(p);
            return p->reader.information();
        }

        auto CntReaderReflib::CntFileVersion() const -> FileVersion {
            assert(p);
            return p->reader.file_version();
        }

        auto CntReaderReflib::EmbeddedFiles() const -> std::vector<UserFile> {
            assert(p);
            const auto labels{ p->reader.embedded_files() };
            std::vector<UserFile> result(labels.size());
            std::transform(begin(labels), end(labels), begin(result), [](const std::string& s) -> UserFile { return { s, "" }; });
            return result;
        }

        auto CntReaderReflib::ExtractEmbeddedFile(const UserFile& x) const -> void {
            assert(p);
            p->reader.extract_embedded_file(x.Label, x.FileName);
        }


        auto validate_writer_phase(WriterPhase x, WriterPhase expected, const std::string& func) -> void {
            if (x == expected) {
                return;
            }

            std::ostringstream oss;
            oss << "[" << func << ", api_reflib] invalid phase '" << x << "', expected '" << expected << "'";
            throw CtkLimit{ oss.str() };
        }


        struct CntWriterReflib::impl
        {
            cnt_writer_reflib_riff writer;
            cnt_writer_reflib_flat *raw3;
            WriterPhase phase;

            impl(const std::filesystem::path& fname, RiffType riff)
            : writer{ fname, riff }
            , raw3{ nullptr }
            , phase{ WriterPhase::Setup } {
                if (fname.empty() || !fname.has_filename()) {
                    std::ostringstream oss;
                    oss << "[CntWriterReflib, api_reflib] no filename '" << fname.string() << "'";
                    const auto e{ oss.str() };
                    throw CtkData{ e };
                }
            }
        };

        CntWriterReflib::CntWriterReflib(const std::filesystem::path& fname, RiffType riff)
            : p{ new impl{ fname, riff } } {
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

        auto CntWriterReflib::Close() -> void {
            assert(p);

            if (p->phase == WriterPhase::Closed || !p->raw3) {
                p->phase = WriterPhase::Closed;
                return;
            }

            p->writer.close();
            p->raw3 = nullptr;
            p->phase = WriterPhase::Closed;
        }

        auto CntWriterReflib::IsClosed() const -> bool {
            assert(p);
            return p->phase == WriterPhase::Closed;
        }

        auto CntWriterReflib::RecordingInfo(const Info& info) -> void {
            assert(p);
            if (p->phase == WriterPhase::Closed) {
                std::ostringstream oss;
                oss << "[CntWriterReflib::RecordingInfo, api_reflib] invalid phase '" << p->phase << "'";
                const auto e{ oss.str() };
                throw CtkLimit{ e };
            }

            p->writer.recording_info(info);
        }

        auto CntWriterReflib::ParamEeg(const TimeSeries& param_eeg) -> void {
            assert(p);
            if (p->raw3) {
                const std::string e{ "[CntWriterReflib::ParamEeg, api_reflib] one segment only" };
                throw CtkLimit{ e };
            }
            assert(p->raw3 == nullptr);

            validate_writer_phase(p->phase, WriterPhase::Setup, "CntWriterReflib::TimeSignal");

            p->raw3 = p->writer.add_time_signal(param_eeg);
            if (p->raw3 == nullptr) {
                const std::string e{ "[CntWriterReflib::TimeSignal, api_reflib] add_time_signal failed" };
                throw CtkLimit{ e };
            }

            p->phase = WriterPhase::Writing;
        }

        auto CntWriterReflib::RangeColumnMajorInt32(const std::vector<int32_t>& client) -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::RangeColumnMajorInt32");

            if (!p->raw3) {
                throw CtkLimit{ "CntWriterReflib::rangeColumnMajor: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->range_column_major(client);
        }

        auto CntWriterReflib::RangeRowMajorInt32(const std::vector<int32_t>& client) -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::RangeRowMajorInt32");

            if (!p->raw3) {
                throw CtkLimit{ "CntWriterReflib::rangeRowMajor: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->range_row_major(client);
        }

        auto CntWriterReflib::RangeColumnMajor(const std::vector<double>& client) -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::RangeColumnMajor");

            if (!p->raw3) {
                throw CtkLimit{ "CntWriterReflib::rangeColumnMajor: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->range_column_major_scaled(client);
        }

        auto CntWriterReflib::RangeRowMajor(const std::vector<double>& client) -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::RangeRowMajor");

            if (!p->raw3) {
                throw CtkLimit{ "CntWriterReflib::rangeRowMajor: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->range_row_major_scaled(client);
        }

        auto CntWriterReflib::RangeLibeep(const std::vector<float>& client) -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::RangeLibeep");

            if (!p->raw3) {
                const std::string e{ "[CntWriterReflib::RangeLibeep, api_reflib] invalid pointer" };
                throw CtkBug{ e };
            }

            p->raw3->range_libeep_v4(client);
        }

        auto CntWriterReflib::AddTrigger(const Trigger& x) -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::Trigger");

            if (!p->raw3) {
                throw CtkLimit{ "CntWriterReflib::trigger: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->trigger(x);
        }

        auto CntWriterReflib::AddTriggers(const std::vector<Trigger>& triggers) -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::Triggers");

            if (!p->raw3) {
                throw CtkLimit{ "CntWriterReflib::triggers: addTimeSignal not invoked or close already invoked" };
            }

            p->raw3->triggers(triggers);
        }

        auto CntWriterReflib::History(const std::string& x) -> void {
            assert(p);
            if (p->phase == WriterPhase::Closed) {
                std::ostringstream oss;
                oss << "[CntWriterReflib::History, api_reflib] invalid phase '" << p->phase << "'";
                const auto e{ oss.str() };
                throw CtkBug{ e };
            }

            p->writer.history(x);
        }

        auto CntWriterReflib::Flush() -> void {
            assert(p);

            validate_writer_phase(p->phase, WriterPhase::Writing, "CntWriterReflib::Flush");

            p->writer.flush();
        }

        auto CntWriterReflib::Embed(const UserFile& x) -> void {
            assert(p);
            p->writer.embed(x.Label, x.FileName);
        }

        auto CntWriterReflib::Commited() const -> int64_t {
            assert(p);
            const measurement_count::value_type result{ p->writer.commited() };
            return result;
        }

        auto CntWriterReflib::RangeRowMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t> {
            assert(p);
            const measurement_count n{ i };
            const measurement_count s{ samples };
            return p->writer.range_row_major(n, s);
        }

        auto CntWriterReflib::RangeColumnMajorInt32(int64_t i, int64_t samples) -> std::vector<int32_t> {
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


        auto swap(EventReader& x, EventReader& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        EventReader::EventReader(const std::filesystem::path& file_name)
        : p{ new impl{ file_name } } {
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

        auto EventReader::ImpedanceCount() const -> size_t {
            assert(p);
            return p->lib.impedances.size();
        }

        auto EventReader::VideoCount() const -> size_t {
            assert(p);
            return p->lib.videos.size();
        }

        auto EventReader::EpochCount() const -> size_t {
            assert(p);
            return p->lib.epochs.size();
        }

        auto EventReader::ImpedanceEvent(size_t i) -> EventImpedance {
            assert(p);
            if (p->lib.impedances.size() <= i) {
                std::ostringstream oss;
                oss << "EventReader::ImpedanceEvent: invalid index " << i << "/" << (p->lib.impedances.size() - 1);
                throw api::v1::CtkLimit{ oss.str() };
            }

            return marker2impedance(p->lib.impedances[i]);
        }

        auto EventReader::VideoEvent(size_t i) -> EventVideo {
            assert(p);
            if (p->lib.videos.size() <= i) {
                std::ostringstream oss;
                oss << "EventReader::VideoEvent: invalid index " << i << "/" << (p->lib.videos.size() - 1);
                throw api::v1::CtkLimit{ oss.str() };
            }

            return marker2video(p->lib.videos[i]);
        }

        auto EventReader::EpochEvent(size_t i) -> EventEpoch {
            assert(p);
            if (p->lib.epochs.size() <= i) {
                std::ostringstream oss;
                oss << "EventReader::EpochEvent: invalid index " << i << "/" << (p->lib.epochs.size() - 1);
                throw api::v1::CtkLimit{ oss.str() };
            }

            return epochevent2eventepoch(p->lib.epochs[i]);
        }

        auto EventReader::ImpedanceEvents() -> std::vector<EventImpedance> {
            assert(p);

            const auto& events{ p->lib.impedances };
            std::vector<EventImpedance> xs(events.size());
            std::transform(begin(events), end(events), begin(xs), marker2impedance);
            return xs;
        }

        auto EventReader::VideoEvents() -> std::vector<EventVideo> {
            assert(p);

            const auto& events{ p->lib.videos };
            std::vector<EventVideo> xs(events.size());
            std::transform(begin(events), end(events), begin(xs), marker2video);
            return xs;
        }

        auto EventReader::EpochEvents() -> std::vector<EventEpoch> {
            assert(p);

            const auto& events{ p->lib.epochs };
            std::vector<EventEpoch> xs(events.size());
            std::transform(begin(events), end(events), begin(xs), epochevent2eventepoch);
            return xs;
        }



        static
        auto fname_archive_bin(std::filesystem::path x) -> std::filesystem::path {
            auto name{ x.filename() };
            name += "_archive.bin";
            x.replace_filename(name);
            return x;
        }


        struct EventWriter::impl
        {
            // used for the evt file construction, not as event storage
            event_library lib;

            // contains the data written out by store_vector_of_pointers
            // (event_lib.cc) after the collection size.
            // on close this data is appended to the approrpiate archive header
            // and collection size in order to construct a valid evt file.
            std::filesystem::path fname_evt;
            file_ptr f_temp;
            uint32_t events;

            explicit impl(const std::filesystem::path& evt)
            : fname_evt{ evt }
            , f_temp{ open_w(fname_archive_bin(evt)) }
            , events{ 0 } {
                assert(f_temp);
            }
        };


        template<typename T>
        auto update_size(uint32_t x, T y) -> uint32_t {
            static_assert(std::is_unsigned<T>::value);

            const uint32_t sum{ static_cast<uint32_t>(x + y) };
            if (sum < x || sum < y) {
                std::ostringstream oss;
                oss << "update_size: unsigned wrap around " << x << " + " << y << " = " << sum;
                throw CtkLimit{ oss.str() };
            }

            return sum;
        }


        EventWriter::EventWriter(const std::filesystem::path& evt)
            : p{ new impl{ evt } } {
            assert(p);
            write_part_header(p->f_temp.get(), file_tag::satellite_evt, as_label("sevt"));
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

        auto EventWriter::AddImpedance(const EventImpedance& x) -> void {
            if (p->f_temp == nullptr) {
                throw CtkLimit{ "EventWriter::AddImpedance: close already invoked" };
            }

            const uint32_t sum{ update_size(p->events, 1U) };

            write_impedance(p->f_temp.get(), impedance2marker(x), p->lib.version);
            p->events = sum;
        }

        auto EventWriter::AddVideo(const EventVideo& x) -> void {
            if (p->f_temp == nullptr) {
                throw CtkLimit{ "EventWriter::AddVideo: close already invoked" };
            }

            const uint32_t sum{ update_size(p->events, 1U) };

            write_video(p->f_temp.get(), video2marker(x), p->lib.version);
            p->events = sum;
        }

        auto EventWriter::AddEpoch(const EventEpoch& x) -> void {
            if (p->f_temp == nullptr) {
                throw CtkLimit{ "EventWriter::AddEpoch: close already invoked" };
            }

            const uint32_t sum{ update_size(p->events, 1U) };

            write_epoch(p->f_temp.get(), eventepoch2epochevent(x), p->lib.version);
            p->events = sum;
        }

        auto EventWriter::AddImpedances(const std::vector<EventImpedance>& xs) -> void {
            if (p->f_temp == nullptr) {
                throw CtkLimit{ "EventWriter::AddImpedances: close already invoked" };
            }

            const uint32_t sum{ update_size(p->events, xs.size()) };

            FILE* f{ p->f_temp.get() };
            const auto version{ p->lib.version };

            for (const auto& x : xs) {
                write_impedance(f, impedance2marker(x), version);
            }
            p->events = sum;
        }

        auto EventWriter::AddVideos(const std::vector<EventVideo>& xs) -> void {
            if (p->f_temp == nullptr) {
                throw CtkLimit{ "EventWriter::AddVideos: close already invoked" };
            }

            const uint32_t sum{ update_size(p->events, xs.size()) };

            FILE* f{ p->f_temp.get() };
            const auto version{ p->lib.version };

            for (const auto& x : xs) {
                write_video(f, video2marker(x), version);
            }
            p->events = sum;
        }

        auto EventWriter::AddEpochs(const std::vector<EventEpoch>& xs) -> void {
            if (p->f_temp == nullptr) {
                throw CtkLimit{ "EventWriter::AddEpochs: close already invoked" };
            }

            const uint32_t sum{ update_size(p->events, xs.size()) };

            FILE* f{ p->f_temp.get() };
            const auto version{ p->lib.version };

            for (const auto& x : xs) {
                write_epoch(f, eventepoch2epochevent(x), version);
            }
            p->events = sum;
        }

        auto EventWriter::Close() -> bool {
            if (p->f_temp == nullptr) {
                return true;
            }
            p->f_temp.reset(nullptr);
            const auto tmp{ fname_archive_bin(p->fname_evt) };

            if (0 < p->events) {
                const int64_t fsize{ content_size(tmp) };
                file_ptr f_in{ open_r(tmp) };
                read_part_header(f_in.get(), file_tag::satellite_evt, as_label("sevt"), true);
                // TODO: read the events sequentially in order to ensure that the data is consitent?

                file_ptr f_out{ open_w(p->fname_evt) };
                write_partial_archive(f_out.get(), p->lib, p->events);
                copy_file_portion(f_in.get(), { part_header_size, fsize - part_header_size }, f_out.get());
            }

            return std::filesystem::remove(tmp);
        }


        auto operator<<(std::ostream& os, WriterPhase x) -> std::ostream& {
            switch(x) {
                case WriterPhase::Setup: os << "Setup"; break;
                case WriterPhase::Writing: os << "Writing"; break;
                case WriterPhase::Closed: os << "Closed"; break;
            }
            return os;
        }

    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */


