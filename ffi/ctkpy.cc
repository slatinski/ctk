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

#include <sstream>

#include "pybind11/pybind11.h"
#include "pybind11/operators.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
#include "pybind11/chrono.h"
#include <pybind11/numpy.h>

#include "ctk.h"
#include "arithmetic.h"
#include "file/cnt_epoch.h"

namespace {

    namespace py = pybind11;
    namespace v1 = ctk::api::v1;

    template<typename T>
    static
    auto print(T x) -> std::string {
        std::ostringstream oss;
        oss << "(" << x << ")";
        return oss.str();
    }


    using trigger_v4_tuple = std::tuple<std::string, int64_t, int64_t, std::string, std::string, std::string>;
    using channel_v4_tuple = std::tuple<std::string, std::string, std::string>; // active label, ref label, unit
    using active_ref_tuple = std::tuple<std::string, std::string>;              // active label, ref label
    using trigger_tuple = std::tuple<int64_t, std::string>;


    struct ctkpy_trigger
    {
        int64_t sample;
        std::string code;

        ctkpy_trigger() : sample{ 0 } {}
        ctkpy_trigger(int64_t s, const std::string& c) : sample{ s }, code{ c } {}
        ctkpy_trigger(const ctkpy_trigger&) = default;
        ctkpy_trigger(ctkpy_trigger&&) = default;
        auto operator=(const ctkpy_trigger&) -> ctkpy_trigger& = default;
        auto operator=(ctkpy_trigger&&) -> ctkpy_trigger& = default;
        ~ctkpy_trigger() = default;

        friend auto operator==(const ctkpy_trigger&, const ctkpy_trigger&) -> bool = default;
        friend auto operator!=(const ctkpy_trigger&, const ctkpy_trigger&) -> bool = default;

        auto get_code() const -> std::string {
            return code;
        }

        auto set_code(const std::string& x) -> void {
            if (v1::sizes::evt_trigger_code < x.size()) {
                std::ostringstream oss;
                oss << "the maximum trigger code label size is " << v1::sizes::evt_trigger_code << ", '" << x << "' is " << x.size();
                throw std::runtime_error(oss.str());
            }

            code = x;
        }
    };

    static
    auto ctkpytrigger2v1trigger(const ctkpy_trigger& x) -> v1::Trigger {
        return { x.sample, x.code };
    }

    static
    auto v1trigger2ctkpytrigger(const v1::Trigger& x) -> ctkpy_trigger {
        return { x.Sample, x.Code };
    }

    static
    auto triggers_from_api(const std::vector<v1::Trigger>& xs) -> std::vector<ctkpy_trigger> {
        std::vector<ctkpy_trigger> ys(xs.size());
        std::transform(begin(xs), end(xs), begin(ys), v1trigger2ctkpytrigger);
        return ys;
    }

    static
    auto triggertuple2v1trigger(const trigger_tuple& x) -> v1::Trigger {
        const auto[s, c]{ x };
        return { s, c };
    }


    struct libeep_reader
    {
        v1::CntReaderReflib reader;
        std::vector<ctkpy_trigger> triggers;
        v1::TimeSeries header;

        explicit
        libeep_reader(const std::string& fname)
        : reader{ fname }
        , triggers{ triggers_from_api(reader.triggers()) }
        , header{ reader.description() } {
        }

        auto get_sample_count() const -> int64_t {
            return reader.sampleCount();
        }

        auto get_channel_count() const -> size_t {
          return header.electrodes.size();
        }

        auto get_channel(size_t i) const -> channel_v4_tuple {
            const auto size{ header.electrodes.size() };
            if (size <= i) {
                std::ostringstream oss;
                oss << "get_channel: invalid index " << i << ", available channels " << size;
                throw std::runtime_error(oss.str());
            }

            const v1::Electrode& x{ header.electrodes[i] };
            return { x.label, x.reference, x.unit };
        }

        auto get_sample_frequency() const -> double {
            return header.sampling_frequency;
        }

        auto get_samples(int64_t i, int64_t amount) -> std::vector<float> {
            return reader.rangeLibeep(i, amount);
        }

        auto get_trigger_count() const -> size_t {
            return triggers.size();
        }

        auto get_trigger(size_t i) const -> trigger_v4_tuple {
            const auto size{ triggers.size() };
            if (size <= i) {
                std::ostringstream oss;
                oss << "get_trigger: invalid index " << i << ", available triggers " << size;
                throw std::runtime_error(oss.str());
            }

            const auto code{ triggers[i].code };
            const auto sample{ triggers[i].sample };
            const int64_t duration{ 0 };
            const std::string condition, descr, impedances;
            return { code, sample, duration, condition, descr, impedances };
        }
    };

    static
    auto read_cnt(const std::string& fname) -> libeep_reader {
        return libeep_reader{ fname };
    }



    static
    auto ar2elc(const active_ref_tuple& x) -> v1::Electrode {
        const auto[label, reference]{ x };
        const v1::Electrode y{ label, reference, "uV" };
        ctk::impl::validate(y);
        return y;
    }

    static
    auto ch2elc(const channel_v4_tuple& x) -> v1::Electrode {
        const auto[label, reference, unit]{ x };
        const v1::Electrode y{ label, reference, unit };
        ctk::impl::validate(y);
        return y;
    }

    static
    auto ar2elcs(const std::vector<active_ref_tuple>& xs) -> std::vector<v1::Electrode> {
        std::vector<v1::Electrode> ys(xs.size());
        std::transform(begin(xs), end(xs), begin(ys), ar2elc);
        return ys;
    }

    static
    auto ch2elcs(const std::vector<channel_v4_tuple>& xs) -> std::vector<v1::Electrode> {
        std::vector<v1::Electrode> ys(xs.size());
        std::transform(begin(xs), end(xs), begin(ys), ch2elc);
        return ys;
    }


    static
    auto int2riff(int x) -> v1::RiffType {
        return x == 0 ? v1::RiffType::riff32 : v1::RiffType::riff64;
    }


    struct libeep_writer
    {
        v1::CntWriterReflib writer;

        libeep_writer(const std::string& fname, double sample_rate, const std::vector<channel_v4_tuple>& channels, int cnt64)
        : writer{ fname, int2riff(cnt64) } {
            v1::TimeSeries ts;
            ts.sampling_frequency = sample_rate;
            ts.electrodes = ch2elcs(channels);
            ts.start_time = std::chrono::system_clock::now();
            writer.addTimeSignal(ts);
        }
        libeep_writer(libeep_writer&&) = default;

        auto add_samples(const std::vector<float>& xs) -> void {
            const auto float2double = [](float x) -> double { return x; };

            std::vector<double> ys(xs.size());
            std::transform(begin(xs), end(xs), begin(ys), float2double);
            writer.rangeColumnMajor(ys);
        }

        auto close() -> void {
            writer.close();
        }
    };

    static
    auto write_cnt(const std::string& fname, double sample_rate, const std::vector<channel_v4_tuple>& channels, int rf64 = 0) -> libeep_writer {
        return libeep_writer{ fname, sample_rate, channels, rf64 };
    }


    template<typename T>
    auto from_row_major(const py::array_t<T>& xs) -> std::vector<T> {
        if (xs.ndim() != 2) {
            std::ostringstream oss;
            oss << "invalid input array dimensions: expected 2, got " << xs.ndim();
            throw std::runtime_error(oss.str());
        }

        ssize_t o{ 0 };
        const ssize_t rows{ xs.shape()[0] };
        const ssize_t cols{ xs.shape()[1] };
        std::vector<T> ys(rows * cols);

        for (ssize_t i{ 0 }; i < rows; ++i) {
            for (ssize_t j{ 0 }; j < cols; ++j) {
                ys[o] = xs.at(i, j);
                ++o;
            }
        }
        return ys;
    }

    template<typename T>
    auto from_column_major(const py::array_t<T>& xs) -> std::vector<T> {
        if (xs.ndim() != 2) {
            std::ostringstream oss;
            oss << "invalid input array dimensions: expected 2, got " << xs.ndim();
            throw std::runtime_error(oss.str());
        }

        ssize_t o{ 0 };
        const ssize_t cols{ xs.shape()[0] };
        const ssize_t rows{ xs.shape()[1] };
        std::vector<T> ys(rows * cols);

        for (ssize_t i{ 0 }; i < rows; ++i) {
            for (ssize_t j{ 0 }; j < cols; ++j) {
                ys[o] = xs.at(j, i);
                ++o;
            }
        }
        return ys;
    }


    template<typename T>
    auto to_column_major(std::vector<T> xs, ssize_t length, ssize_t height) -> py::array_t<T> {
        assert(!xs.empty());
        assert(xs.size() == static_cast<size_t>(length * height));

        constexpr const auto itemsize{ sizeof(T) };
        constexpr const int ndim{ 2 };
        const ssize_t shape[]{ length, height };
        const ssize_t strides[]{ height * static_cast<ssize_t>(sizeof(T)), sizeof(T) };
        return py::array_t<T>(
            py::buffer_info(xs.data(), itemsize, py::format_descriptor<T>::format(), ndim, shape, strides)
        );
    }

    template<typename T>
    auto to_row_major(std::vector<T> xs, ssize_t length, ssize_t height) -> py::array_t<T> {
        assert(!xs.empty());
        assert(xs.size() == static_cast<size_t>(length * height));

        constexpr const auto itemsize{ sizeof(T) };
        constexpr const int ndim{ 2 };
        const ssize_t shape[]{ height, length };
        const ssize_t strides[]{ length * static_cast<int>(sizeof(T)), sizeof(T) };
        return py::array_t<T>(
            py::buffer_info(xs.data(), itemsize, py::format_descriptor<T>::format(), ndim, shape, strides)
        );
    }



    struct ctkpy_writer
    {
        std::filesystem::path file_name;
        v1::RiffType type;
        v1::TimeSeries header;
        v1::Info recording_info;

        std::unique_ptr<v1::CntWriterReflib> writer;
        v1::EventWriter events;

        explicit ctkpy_writer(const std::string& fname, v1::RiffType cnt = v1::RiffType::riff64)
        : file_name{ fname }
        , type{ cnt }
        , events{ std::filesystem::path{ fname }.replace_extension("evt") } {
        }

        ctkpy_writer(ctkpy_writer&&) = default;

        auto initialize() -> void {
            if (header.start_time == ctk::api::dcdate2timepoint({ 0, 0 })) {
                header.start_time = std::chrono::system_clock::now();
            }
            ctk::impl::validate(header);

            writer.reset(new v1::CntWriterReflib{ file_name, type });
            assert(writer);

            writer->addTimeSignal(header);
            writer->recordingInfo(recording_info);
        }

        auto column_major(const py::array_t<double>& xs) -> void {
            if (!writer) {
                initialize();
            }

            writer->rangeRowMajor(from_column_major(xs));
        }

        auto row_major(const py::array_t<double>& xs) -> void {
            if (!writer) {
                initialize();
            }

            writer->rangeRowMajor(from_row_major(xs));
        }

        auto close() -> void {
            if (!writer) {
                return;
            }

            events.close();
            writer->recordingInfo(recording_info);
            writer->close();
            writer.reset(nullptr);
        }

        auto add_ar_electrode(const active_ref_tuple& x) -> void {
            header.electrodes.push_back(ar2elc(x));
        }

        auto add_v4_electrode(const channel_v4_tuple& x) -> void {
            header.electrodes.push_back(ch2elc(x));
        }

        auto add_electrode(const v1::Electrode& x) -> void {
            header.electrodes.push_back(x);
        }

        auto add_trigger(const ctkpy_trigger& x) -> void {
            if (!writer) {
                initialize();
            }

            writer->trigger(ctkpytrigger2v1trigger(x));
        }

        auto add_trigger_ctkpy(const trigger_tuple& x) -> void {
            if (!writer) {
                initialize();
            }

            writer->trigger(triggertuple2v1trigger(x));
        }

        auto add_triggers(const std::vector<ctkpy_trigger>& xs) -> void {
            if (!writer) {
                initialize();
            }

            std::vector<v1::Trigger> ys(xs.size());
            std::transform(begin(xs), end(xs), begin(ys), ctkpytrigger2v1trigger);
            writer->triggers(ys);
        }

        auto add_triggers_ctkpy(const std::vector<trigger_tuple>& xs) -> void {
            if (!writer) {
                initialize();
            }

            std::vector<v1::Trigger> ys(xs.size());
            std::transform(begin(xs), end(xs), begin(ys), triggertuple2v1trigger);
            writer->triggers(ys);
        }

        auto add_impedance(const v1::EventImpedance& x) -> void {
            events.addImpedance(x);
        }

        auto add_impedances(const std::vector<v1::EventImpedance>& xs) -> void {
            events.addImpedances(xs);
        }

        auto add_video(const v1::EventVideo& x) -> void {
            events.addVideo(x);
        }

        auto add_videos(const std::vector<v1::EventVideo>& xs) -> void {
            events.addVideos(xs);
        }

        auto add_epoch(const v1::EventEpoch& x) -> void {
            events.addEpoch(x);
        }

        auto add_epochs(const std::vector<v1::EventEpoch>& xs) -> void {
            events.addEpochs(xs);
        }

        auto embed(const v1::UserFile& x) -> void {
            if (!writer) {
                initialize();
            }

            writer->embed(x);
        }
    };


    struct ctkpy_reader
    {
        std::unique_ptr<v1::CntReaderReflib> reader;

        std::filesystem::path file_name;
        v1::RiffType type;
        v1::TimeSeries header;
        v1::Info recording_info;
        std::vector<ctkpy_trigger> triggers;
        std::vector<v1::EventImpedance> impedances;
        std::vector<v1::EventVideo> videos;
        std::vector<v1::EventEpoch> epochs;
        std::vector<v1::UserFile> embedded;

        ctkpy_reader(const std::string& fname)
        : reader{ new v1::CntReaderReflib{ fname } }
        , file_name{ fname } {
            type = reader->cntType();
            header = reader->description();
            recording_info = reader->information();
            triggers = triggers_from_api(reader->triggers());
            embedded = reader->embeddedFiles();

            const auto evt_fname{ file_name.replace_extension("evt") };
            if (!std::filesystem::exists(evt_fname)) {
                return;
            }

            v1::EventReader events{ evt_fname };
            impedances = events.impedanceEvents();
            videos = events.videoEvents();
            epochs = events.epochEvents();
        }

        auto sample_count() -> int64_t {
            assert(reader);
            return reader->sampleCount();
        }


        auto column_major(int64_t i, int64_t length) -> py::array_t<double> {
            assert(reader);

            auto xs{ reader->rangeColumnMajor(i, length) };
            if (xs.empty()) {
                throw std::runtime_error("can not load range");
            }

            assert(!header.electrodes.empty());

            const size_t channels{ header.electrodes.size() };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(xs, l, h);
        }


        auto row_major(int64_t i, int64_t length) -> py::array_t<double> {
            assert(reader);

            auto xs{ reader->rangeRowMajor(i, length) };
            if (xs.empty()) {
                throw std::runtime_error("can not load range");
            }

            assert(!header.electrodes.empty());

            const size_t channels{ header.electrodes.size() };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            return to_row_major(xs, l, h);
        }


        auto epoch_count() -> int64_t {
            assert(reader);
            return reader->epochs();
        }


        auto epoch_column_major(int64_t i) -> py::array_t<double> {
            assert(reader);

            auto xs{ reader->epochColumnMajor(i) };
            if (xs.empty()) {
                throw std::runtime_error("can not load epoch");
            }

            assert(!header.electrodes.empty());

            const size_t channels{ header.electrodes.size() };
            const size_t length{ xs.size() / channels };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(xs, l, h);
        }


        auto epoch_row_major(int64_t i) -> py::array_t<double> {
            assert(reader);

            auto xs{ reader->epochRowMajor(i) };
            if (xs.empty()) {
                throw std::runtime_error("can not load epoch");
            }

            assert(!header.electrodes.empty());

            const size_t channels{ header.electrodes.size() };
            const size_t length{ xs.size() / channels };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            return to_row_major(xs, l, h);
        }


        auto epoch_compressed(int64_t i) -> std::vector<uint8_t> {
            assert(reader);

            return reader->epochCompressed(i);
        }


        auto extract_embedded_file(const v1::UserFile& x) -> bool {
            assert(reader);
            if (x.file_name.empty()) {
                throw std::runtime_error("no output file name is provided in the field file_name");
            }

            return reader->extractEmbeddedFile(x);
        }
    };


    template<typename Compressor>
    // Compressor has interface compatible with CompressReflib, CompressInt16, CompressInt32, CompressInt64, CompressUInt16, CompressUInt32, CompressUInt64
    struct enc_matrix
    {
        Compressor compress;
        using T = typename Compressor::value_type;

        enc_matrix() = default;
        enc_matrix(const enc_matrix&) = default;
        enc_matrix(enc_matrix&&) = default;
        auto operator=(const enc_matrix&) -> enc_matrix& = default;
        auto operator=(enc_matrix&&) -> enc_matrix& = default;
        ~enc_matrix() = default;

        auto sensors(int64_t height) -> bool {
            return compress.sensors(height);
        }

        auto order(const std::vector<int16_t>& xs) -> bool {
            return compress.sensors(xs);
        }

        auto reserve(int64_t length) -> void {
            compress.reserve(length);
        }

        auto column_major(const py::array_t<T>& xs) -> std::vector<uint8_t> {
            const int64_t length{ xs.shape()[0] };
            return compress.row_major(from_column_major(xs), length);
        }

        auto row_major(const py::array_t<T>& xs) -> std::vector<uint8_t> {
            const int64_t length{ xs.shape()[1] };
            return compress.row_major(from_row_major(xs), length);
        }
    };

    template<typename Decompressor>
    // Decompressor has interface compatible with DecompressReflib, DecompressInt16, DecompressInt32, DecompressInt64, DecompressUInt16, DecompressUInt32, DecompressUInt64
    struct dec_matrix
    {
        Decompressor decompress;
        using T = typename Decompressor::value_type;

        dec_matrix() = default;
        dec_matrix(const dec_matrix&) = default;
        dec_matrix(dec_matrix&&) = default;
        auto operator=(const dec_matrix&) -> dec_matrix& = default;
        auto operator=(dec_matrix&&) -> dec_matrix& = default;
        ~dec_matrix() = default;

        auto sensors(int64_t height) -> bool {
            return decompress.sensors(height);
        }

        auto order(const std::vector<int16_t>& xs) -> bool {
            return decompress.sensors(xs);
        }

        auto reserve(int64_t length) -> void {
            decompress.reserve(length);
        }

        auto column_major(const std::vector<uint8_t>& xs, int64_t length) -> py::array_t<T> {
            if (length <= 0) {
                throw std::runtime_error("invalid length");
            }

            // TODO: add height getter to the matrix interface and pass temporary to to_column_major
            const auto ys{ decompress.column_major(xs, length) };
            const ssize_t h{ ctk::impl::cast(ctk::impl::vsize(ys) / length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(ys, l, h);
        }

        auto row_major(const std::vector<uint8_t>& xs, int64_t length) -> py::array_t<T> {
            if (length <= 0) {
                throw std::runtime_error("invalid length");
            }

            // TODO: add height getter to the matrix interface and pass temporary to to_row_major
            const auto ys{ decompress.row_major(xs, length) };
            const ssize_t h{ ctk::impl::cast(ctk::impl::vsize(ys) / length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            return to_row_major(ys, l, h);
        }
    };

} // anonymous namespace


PYBIND11_MODULE(ctkpy, m) {

    py::enum_<v1::RiffType>(m, "cnt_type", py::module_local())
        .value("cnt32", v1::RiffType::riff32)
        .value("cnt64", v1::RiffType::riff64);

    py::enum_<v1::Sex>(m, "sex", py::module_local())
        .value("unknown", v1::Sex::unknown)
        .value("male", v1::Sex::male)
        .value("female", v1::Sex::female);

    py::enum_<v1::Handedness>(m, "handedness", py::module_local())
        .value("unknown", v1::Handedness::unknown)
        .value("left", v1::Handedness::left)
        .value("right", v1::Handedness::right)
        .value("mixed", v1::Handedness::mixed);

    py::class_<v1::FileVersion> fv(m, "file_version", py::module_local());
    fv.def(py::init<uint32_t, uint32_t>())
      .def_readwrite("major", &v1::FileVersion::major)
      .def_readwrite("minor", &v1::FileVersion::minor)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::FileVersion& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.major, x.minor);
        },
        [](py::tuple xs) -> v1::FileVersion { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("file_version pickle: invalid state");
            }
            return v1::FileVersion{ xs[0].cast<uint32_t>(), xs[1].cast<uint32_t>() };
        }
      ))
      .def("__copy__", [](const v1::FileVersion& x){ return v1::FileVersion(x); })
      .def("__deepcopy__", [](const v1::FileVersion& x, py::dict){ return v1::FileVersion(x); })
      .def("__repr__", [](const v1::FileVersion& x) { return print(x); });

    py::class_<ctkpy_trigger> t(m, "trigger", py::module_local());
    t.def(py::init<int64_t, const std::string&>())
     .def_readwrite("sample", &ctkpy_trigger::sample)
     .def_property("code", &ctkpy_trigger::get_code, &ctkpy_trigger::set_code)
     .def(py::self == py::self) // __eq__
     .def(py::self != py::self) // __ne__
     .def(py::pickle(
        [](const ctkpy_trigger& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.sample, x.code);
        },
        [](py::tuple xs) -> ctkpy_trigger { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("trigger pickle: invalid state");
            }
            return ctkpy_trigger { xs[0].cast<int64_t>(), xs[1].cast<std::string>() };
        }
      ))
     .def("__copy__", [](const ctkpy_trigger& x){ return ctkpy_trigger(x); })
     .def("__deepcopy__", [](const ctkpy_trigger& x, py::dict){ return ctkpy_trigger(x); })
     .def("__repr__", [](const ctkpy_trigger& x) { return print(ctkpytrigger2v1trigger(x)); });

    py::class_<v1::Electrode> e(m, "electrode", py::module_local());
    e.def(py::init<const std::string&, const std::string&, const std::string&, double, double>())
     .def_readwrite("label", &v1::Electrode::label)
     .def_readwrite("reference", &v1::Electrode::reference)
     .def_readwrite("unit", &v1::Electrode::unit)
     .def_readwrite("status", &v1::Electrode::status)
     .def_readwrite("type", &v1::Electrode::type)
     .def_readwrite("iscale", &v1::Electrode::iscale)
     .def_readwrite("rscale", &v1::Electrode::rscale)
     .def(py::self == py::self) // __eq__
     .def(py::self != py::self) // __ne__
     .def(py::pickle(
        [](const v1::Electrode& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.label, x.reference, x.unit, x.status, x.type, x.iscale, x.rscale);
        },
        [](py::tuple xs) -> v1::Electrode { // __setstate__
            if (xs.size() != 7) {
                throw std::runtime_error("electrode pickle: invalid state");
            }

            v1::Electrode y;
            y.label = xs[0].cast<std::string>();
            y.reference = xs[1].cast<std::string>();
            y.unit = xs[2].cast<std::string>();
            y.status = xs[3].cast<std::string>();
            y.type = xs[4].cast<std::string>();
            y.iscale = xs[0].cast<double>();
            y.rscale = xs[0].cast<double>();
            return y;

        }
      ))
     .def("__copy__", [](const v1::Electrode& x){ return v1::Electrode(x); })
     .def("__deepcopy__", [](const v1::Electrode& x, py::dict){ return v1::Electrode(x); })
     .def("__repr__", [](const v1::Electrode& x) { return print(x); });

    m.def("electrodes", &ar2elcs, "Converts [('active', 'reference', 'unit')] to [ctk.electrode]; default: range scale 1/256");
    m.def("electrodes", &ch2elcs, "Converts [('active', 'reference')] to [ctk.electrode]; unit uV, range scale 1/256");

    py::class_<v1::TimeSeries> ts(m, "time_series", py::module_local());
    ts.def(py::init<std::chrono::system_clock::time_point, double, const std::vector<v1::Electrode>&, int64_t>())
      .def_readwrite("epoch_length", &v1::TimeSeries::epoch_length)
      .def_readwrite("sampling_frequency", &v1::TimeSeries::sampling_frequency)
      .def_readwrite("start_time", &v1::TimeSeries::start_time)
      .def_readwrite("electrodes", &v1::TimeSeries::electrodes)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::TimeSeries& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.start_time, x.sampling_frequency, x.electrodes, x.epoch_length);
        },
        [](py::tuple xs) -> v1::TimeSeries { // __setstate__
            if (xs.size() != 4) {
                throw std::runtime_error("time_series pickle: invalid state");
            }
            return v1::TimeSeries{ xs[0].cast<std::chrono::system_clock::time_point>(),
                                   xs[1].cast<double>(),
                                   xs[2].cast<std::vector<v1::Electrode>>(),
                                   xs[3].cast<int64_t>() };
        }
      ))
      .def("__copy__", [](const v1::TimeSeries& x){ return v1::TimeSeries(x); })
      .def("__deepcopy__", [](const v1::TimeSeries& x, py::dict){ return v1::TimeSeries(x); })
      .def("__repr__", [](const v1::TimeSeries& x) { return print(x); });

    py::class_<v1::Info> i(m, "information", py::module_local());
    i.def(py::init<>())
     .def_readwrite("hospital", &v1::Info::hospital)
     .def_readwrite("test_name", &v1::Info::test_name)
     .def_readwrite("test_serial", &v1::Info::test_serial)
     .def_readwrite("physician", &v1::Info::physician)
     .def_readwrite("technician", &v1::Info::technician)
     .def_readwrite("machine_make", &v1::Info::machine_make)
     .def_readwrite("machine_model", &v1::Info::machine_model)
     .def_readwrite("machine_sn", &v1::Info::machine_sn)
     .def_readwrite("subject_name", &v1::Info::subject_name)
     .def_readwrite("subject_id", &v1::Info::subject_id)
     .def_readwrite("subject_phone", &v1::Info::subject_phone)
     .def_readwrite("subject_address", &v1::Info::subject_address)
     .def_readwrite("subject_sex", &v1::Info::subject_sex)
     .def_readwrite("subject_handedness", &v1::Info::subject_handedness)
     .def_readwrite("subject_dob", &v1::Info::subject_dob)
     .def_readwrite("comment", &v1::Info::comment)
     .def(py::self == py::self) // __eq__
     .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::Info& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.hospital,
                                  x.test_name,
                                  x.test_serial,
                                  x.physician,
                                  x.technician,
                                  x.machine_make,
                                  x.machine_model,
                                  x.machine_sn,
                                  x.subject_name,
                                  x.subject_id,
                                  x.subject_address,
                                  x.subject_phone,
                                  x.subject_sex,
                                  x.subject_handedness,
                                  x.subject_dob,
                                  x.comment);
        },
        [](py::tuple xs) -> v1::Info { // __setstate__
            if (xs.size() != 16) {
                throw std::runtime_error("info pickle: invalid state");
            }
            v1::Info y;
            y.hospital = xs[0].cast<std::string>();
            y.test_name = xs[1].cast<std::string>();
            y.test_serial = xs[2].cast<std::string>();
            y.physician = xs[3].cast<std::string>();
            y.technician = xs[4].cast<std::string>();
            y.machine_make = xs[5].cast<std::string>();
            y.machine_model = xs[6].cast<std::string>();
            y.machine_sn = xs[7].cast<std::string>();
            y.subject_name = xs[8].cast<std::string>();
            y.subject_id = xs[9].cast<std::string>();
            y.subject_address = xs[10].cast<std::string>();
            y.subject_phone = xs[11].cast<std::string>();
            y.subject_sex = xs[12].cast<v1::Sex>();
            y.subject_handedness = xs[13].cast<v1::Handedness>();
            y.subject_dob = xs[14].cast<std::chrono::system_clock::time_point>();
            y.comment = xs[15].cast<std::string>();
            return y;
        }
      ))
     .def("__copy__", [](const v1::Info& x){ return v1::Info(x); })
     .def("__deepcopy__", [](const v1::Info& x, py::dict){ return v1::Info(x); })
     .def("__repr__", [](const v1::Info& x) { return print(x); });

    py::class_<v1::UserFile> uf(m, "user_file", py::module_local());
    uf.def(py::init<const std::string&, const std::string&>())
      .def_readwrite("label", &v1::UserFile::label)
      .def_readwrite("file_name", &v1::UserFile::file_name)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::UserFile& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.label, x.file_name);
        },
        [](py::tuple xs) -> v1::UserFile { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("trigger pickle: invalid state");
            }
            return v1::UserFile{ xs[0].cast<std::string>(), xs[1].cast<std::string>() };
        }
      ))
      .def("__copy__", [](const v1::UserFile& x){ return v1::UserFile(x); })
      .def("__deepcopy__", [](const v1::UserFile& x, py::dict){ return v1::UserFile(x); })
      .def("__repr__", [](const v1::UserFile& x) { return print(x); });


    py::class_<v1::EventImpedance> ei(m, "event_impedance", py::module_local());
    ei.def(py::init<std::chrono::system_clock::time_point, const std::vector<float>&>())
      .def_readwrite("stamp", &v1::EventImpedance::stamp)
      .def_readwrite("values", &v1::EventImpedance::values)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::EventImpedance& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.stamp, x.values);
        },
        [](py::tuple xs) -> v1::EventImpedance { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("trigger pickle: invalid state");
            }
            return v1::EventImpedance{ xs[0].cast<std::chrono::system_clock::time_point>(), xs[1].cast<std::vector<float>>() };
        }
      ))
      .def("__copy__", [](const v1::EventImpedance& x){ return v1::EventImpedance(x); })
      .def("__deepcopy__", [](const v1::EventImpedance& x, py::dict){ return v1::EventImpedance(x); })
      .def("__repr__", [](const v1::EventImpedance& x) {
          std::ostringstream oss;
          oss << "(" << x.values.size() << " items)";
          return oss.str();
      });

    py::class_<v1::EventVideo> ev(m, "event_video", py::module_local());
    ev.def(py::init<std::chrono::system_clock::time_point, double, int32_t>())
      .def_readwrite("stamp", &v1::EventVideo::stamp)
      .def_readwrite("duration", &v1::EventVideo::duration)
      .def_readwrite("trigger_code", &v1::EventVideo::trigger_code)
      .def_readwrite("condition_label", &v1::EventVideo::condition_label)
      .def_readwrite("description", &v1::EventVideo::description)
      .def_readwrite("video_file", &v1::EventVideo::video_file)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::EventVideo& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.stamp, x.duration, x.trigger_code, x.condition_label, x.description, x.video_file);
        },
        [](py::tuple xs) -> v1::EventVideo { // __setstate__
            if (xs.size() != 6) {
                throw std::runtime_error("trigger pickle: invalid state");
            }
            v1::EventVideo y;
            y.stamp = xs[0].cast<std::chrono::system_clock::time_point>();
            y.duration = xs[1].cast<double>();
            y.trigger_code = xs[2].cast<int32_t>();
            y.condition_label = xs[3].cast<std::wstring>();
            y.description = xs[4].cast<std::string>();
            y.video_file = xs[5].cast<std::wstring>();
            return y;
        }
      ))
      .def("__copy__", [](const v1::EventVideo& x){ return v1::EventVideo(x); })
      .def("__deepcopy__", [](const v1::EventVideo& x, py::dict){ return v1::EventVideo(x); })
      .def("__repr__", [](const v1::EventVideo& x) {
          std::ostringstream oss;
          oss << "(" << x.duration << " " << x.trigger_code << ")";
          return oss.str();
      });

    py::class_<v1::EventEpoch> ee(m, "event_epoch", py::module_local());
    ee.def(py::init<std::chrono::system_clock::time_point, double, double, int32_t>())
      .def_readwrite("stamp", &v1::EventEpoch::stamp)
      .def_readwrite("duration", &v1::EventEpoch::duration)
      .def_readwrite("offset", &v1::EventEpoch::offset)
      .def_readwrite("trigger_code", &v1::EventEpoch::trigger_code)
      .def_readwrite("condition_label", &v1::EventEpoch::condition_label)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::EventEpoch& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.stamp, x.duration, x.offset, x.trigger_code, x.condition_label);
        },
        [](py::tuple xs) -> v1::EventEpoch { // __setstate__
            if (xs.size() != 5) {
                throw std::runtime_error("trigger pickle: invalid state");
            }
            v1::EventEpoch y;
            y.stamp = xs[0].cast<std::chrono::system_clock::time_point>();
            y.duration = xs[1].cast<double>();
            y.offset = xs[2].cast<double>();
            y.trigger_code = xs[3].cast<int32_t>();
            y.condition_label = xs[4].cast<std::wstring>();
            return y;
        }
      ))
      .def("__copy__", [](const v1::EventEpoch& x){ return v1::EventEpoch(x); })
      .def("__deepcopy__", [](const v1::EventEpoch& x, py::dict){ return v1::EventEpoch(x); })
      .def("__repr__", [](const v1::EventEpoch& x) {
          std::ostringstream oss;
          oss << "(" << x.duration << " " << x.offset << " " << x.trigger_code << ")";
          return oss.str();
      });


    py::class_<v1::Version> lv(m, "lib_version", py::module_local());
    lv.def(py::init<>())
      .def_readonly("major", &v1::Version::major)
      .def_readonly("minor", &v1::Version::minor)
      .def_readonly("patch", &v1::Version::patch)
      .def_readonly("build", &v1::Version::build)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::Version& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.major, x.minor, x.patch, x.build);
        },
        [](py::tuple xs) -> v1::Version { // __setstate__
            if (xs.size() != 4) {
                throw std::runtime_error("file_version pickle: invalid state");
            }
            v1::Version x;
            x.major = xs[0].cast<uint32_t>();
            x.minor = xs[1].cast<uint32_t>();
            x.patch = xs[2].cast<uint32_t>();
            x.build = xs[3].cast<uint32_t>();
            return x;
        }
      ))
      .def("__copy__", [](const v1::Version& x){ return v1::Version(x); })
      .def("__deepcopy__", [](const v1::Version& x, py::dict){ return v1::Version(x); })
      .def("__repr__", [](const v1::Version& x) { return print(x); });


    // 1) cnt + evt file
    py::class_<ctkpy_writer> rw(m, "writer_reflib", py::module_local());
    rw.def(py::init<const std::string&, v1::RiffType>(), py::arg("fname"), py::arg("type") = v1::RiffType::riff64 )
      .def_readwrite("param", &ctkpy_writer::header)
      .def_readwrite("info", &ctkpy_writer::recording_info)
      .def("add_electrode", &ctkpy_writer::add_ar_electrode)
      .def("add_electrode", &ctkpy_writer::add_v4_electrode)
      .def("add_electrode", &ctkpy_writer::add_electrode)
      .def("row_major", &ctkpy_writer::row_major)
      .def("column_major", &ctkpy_writer::column_major)
      .def("close", &ctkpy_writer::close, "Constructs the output cnt file")
      .def("trigger", &ctkpy_writer::add_trigger)
      .def("trigger", &ctkpy_writer::add_trigger_ctkpy)
      .def("triggers", &ctkpy_writer::add_triggers)
      .def("triggers", &ctkpy_writer::add_triggers_ctkpy)
      .def("impedance", &ctkpy_writer::add_impedance)
      .def("impedances", &ctkpy_writer::add_impedances)
      .def("video", &ctkpy_writer::add_video)
      .def("videos", &ctkpy_writer::add_videos)
      .def("epoch", &ctkpy_writer::add_epoch)
      .def("epochs", &ctkpy_writer::add_epochs)
      .def("embed", &ctkpy_writer::embed)
      .def("__repr__", [](const ctkpy_writer& x) { return print(x.header); });

    py::class_<ctkpy_reader> rr(m, "reader_reflib", py::module_local());
    rr.def(py::init<const std::string&>())
      .def_readwrite("param", &ctkpy_reader::header)
      .def_readwrite("info", &ctkpy_reader::recording_info)
      .def_readwrite("triggers", &ctkpy_reader::triggers)
      .def_readwrite("impedances", &ctkpy_reader::impedances)
      .def_readwrite("videos", &ctkpy_reader::videos)
      .def_readwrite("epochs", &ctkpy_reader::epochs)
      .def_property_readonly("sample_count", &ctkpy_reader::sample_count)
      .def("row_major", &ctkpy_reader::row_major)
      .def("column_major", &ctkpy_reader::column_major)
      .def_property_readonly("epoch_count", &ctkpy_reader::epoch_count)
      .def("epoch_row_major", &ctkpy_reader::epoch_row_major)
      .def("epoch_column_major", &ctkpy_reader::epoch_column_major)
      .def("epoch_compressed", &ctkpy_reader::epoch_compressed)
      .def_readonly("embedded", &ctkpy_reader::embedded)
      .def("extract_embedded", &ctkpy_reader::extract_embedded_file)
      .def("__repr__", [](const ctkpy_reader& x) { return print(x.header); });


    // 2) evt file only
    py::class_<v1::EventReader> er(m, "event_reader", py::module_local());
    er.def(py::init<const std::string&>())
      .def_property_readonly("count_impedances", &v1::EventReader::impedanceCount)
      .def_property_readonly("count_videos", &v1::EventReader::videoCount)
      .def_property_readonly("count_epochs", &v1::EventReader::epochCount)
      .def("impedance", &v1::EventReader::impedanceEvent)
      .def("video", &v1::EventReader::videoEvent)
      .def("epoch", &v1::EventReader::epochEvent)
      .def_property_readonly("impedances", &v1::EventReader::impedanceEvents)
      .def_property_readonly("videos", &v1::EventReader::videoEvents)
      .def_property_readonly("epochs", &v1::EventReader::epochEvents);

    py::class_<v1::EventWriter> ew(m, "event_writer", py::module_local());
    ew.def(py::init<const std::string&>())
      .def("impedance", &v1::EventWriter::addImpedance)
      .def("video", &v1::EventWriter::addVideo)
      .def("epoch", &v1::EventWriter::addEpoch)
      .def("impedances", &v1::EventWriter::addImpedances)
      .def("videos", &v1::EventWriter::addVideos)
      .def("epochs", &v1::EventWriter::addEpochs)
      .def("close", &v1::EventWriter::close, "Constructs the output evt file");


    // 3) pyeep interface
    py::class_<libeep_reader> lr(m, "cnt_in", py::module_local());
    lr.def(py::init<const std::string&>())
      .def("get_channel_count", &libeep_reader::get_channel_count)
      .def("get_channel", &libeep_reader::get_channel)
      .def("get_sample_frequency", &libeep_reader::get_sample_frequency)
      .def("get_sample_count", &libeep_reader::get_sample_count)
      .def("get_samples", &libeep_reader::get_samples)
      .def("get_trigger_count", &libeep_reader::get_trigger_count)
      .def("get_trigger", &libeep_reader::get_trigger);

    m.def("read_cnt", &read_cnt, "Opens a CNT file for reading");

    py::class_<libeep_writer> lw(m, "cnt_out", py::module_local());
    lw.def(py::init<const std::string&, int, const std::vector<channel_v4_tuple>&, int>())
      .def("add_samples", &libeep_writer::add_samples)
      .def("close", &libeep_writer::close, "Constructs the output cnt file");

    m.def("write_cnt", &write_cnt, "Opens a CNT file for writing");

    // 4) compression
    py::class_<enc_matrix<v1::CompressReflib>> cr(m, "compress_reflib", py::module_local());
    cr.def(py::init<>())
      .def_property("sensors", nullptr, &enc_matrix<v1::CompressReflib>::sensors)
      .def_property("order", nullptr, &enc_matrix<v1::CompressReflib>::order)
      .def("reserve", &enc_matrix<v1::CompressReflib>::reserve)
      .def("column_major", &enc_matrix<v1::CompressReflib>::column_major)
      .def("row_major", &enc_matrix<v1::CompressReflib>::row_major);

    py::class_<dec_matrix<v1::DecompressReflib>> dr(m, "decompress_reflib", py::module_local());
    dr.def(py::init<>())
      .def_property("sensors", nullptr, &dec_matrix<v1::DecompressReflib>::sensors)
      .def_property("order", nullptr, &dec_matrix<v1::DecompressReflib>::sensors)
      .def("reserve", &dec_matrix<v1::DecompressReflib>::reserve)
      .def("column_major", &dec_matrix<v1::DecompressReflib>::column_major)
      .def("row_major", &dec_matrix<v1::DecompressReflib>::row_major);

    py::class_<enc_matrix<v1::CompressInt16>> ci16(m, "compress_i16", py::module_local());
    ci16.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressInt16>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressInt16>::order)
        .def("reserve", &enc_matrix<v1::CompressInt16>::reserve)
        .def("column_major", &enc_matrix<v1::CompressInt16>::column_major)
        .def("row_major", &enc_matrix<v1::CompressInt16>::row_major);

    py::class_<dec_matrix<v1::DecompressInt16>> di16(m, "decompress_i16", py::module_local());
    di16.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressInt16>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressInt16>::sensors)
        .def("reserve", &dec_matrix<v1::DecompressInt16>::reserve)
        .def("column_major", &dec_matrix<v1::DecompressInt16>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressInt16>::row_major);

    py::class_<enc_matrix<v1::CompressInt32>> ci32(m, "compress_i32", py::module_local());
    ci32.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressInt32>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressInt32>::order)
        .def("reserve", &enc_matrix<v1::CompressInt32>::reserve)
        .def("column_major", &enc_matrix<v1::CompressInt32>::column_major)
        .def("row_major", &enc_matrix<v1::CompressInt32>::row_major);

    py::class_<dec_matrix<v1::DecompressInt32>> di32(m, "decompress_i32", py::module_local());
    di32.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressInt32>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressInt32>::sensors)
        .def("reserve", &dec_matrix<v1::DecompressInt32>::reserve)
        .def("column_major", &dec_matrix<v1::DecompressInt32>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressInt32>::row_major);

    py::class_<enc_matrix<v1::CompressInt64>> ci64(m, "compress_i64", py::module_local());
    ci64.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressInt64>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressInt64>::order)
        .def("reserve", &enc_matrix<v1::CompressInt64>::reserve)
        .def("column_major", &enc_matrix<v1::CompressInt64>::column_major)
        .def("row_major", &enc_matrix<v1::CompressInt64>::row_major);

    py::class_<dec_matrix<v1::DecompressInt64>> di64(m, "decompress_i64", py::module_local());
    di64.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressInt64>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressInt64>::sensors)
        .def("reserve", &dec_matrix<v1::DecompressInt64>::reserve)
        .def("column_major", &dec_matrix<v1::DecompressInt64>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressInt64>::row_major);


    py::class_<enc_matrix<v1::CompressUInt16>> cu16(m, "compress_u16", py::module_local());
    cu16.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressUInt16>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressUInt16>::order)
        .def("reserve", &enc_matrix<v1::CompressUInt16>::reserve)
        .def("column_major", &enc_matrix<v1::CompressUInt16>::column_major)
        .def("row_major", &enc_matrix<v1::CompressUInt16>::row_major);

    py::class_<dec_matrix<v1::DecompressUInt16>> du16(m, "decompress_u16", py::module_local());
    du16.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressUInt16>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressUInt16>::sensors)
        .def("reserve", &dec_matrix<v1::DecompressUInt16>::reserve)
        .def("column_major", &dec_matrix<v1::DecompressUInt16>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressUInt16>::row_major);

    py::class_<enc_matrix<v1::CompressUInt32>> cu32(m, "compress_u32", py::module_local());
    cu32.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressUInt32>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressUInt32>::order)
        .def("reserve", &enc_matrix<v1::CompressUInt32>::reserve)
        .def("column_major", &enc_matrix<v1::CompressUInt32>::column_major)
        .def("row_major", &enc_matrix<v1::CompressUInt32>::row_major);

    py::class_<dec_matrix<v1::DecompressUInt32>> du32(m, "decompress_u32", py::module_local());
    du32.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressUInt32>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressUInt32>::sensors)
        .def("reserve", &dec_matrix<v1::DecompressUInt32>::reserve)
        .def("column_major", &dec_matrix<v1::DecompressUInt32>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressUInt32>::row_major);

    py::class_<enc_matrix<v1::CompressUInt64>> cu64(m, "compress_u64", py::module_local());
    cu64.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressUInt64>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressUInt64>::order)
        .def("reserve", &enc_matrix<v1::CompressUInt64>::reserve)
        .def("column_major", &enc_matrix<v1::CompressUInt64>::column_major)
        .def("row_major", &enc_matrix<v1::CompressUInt64>::row_major);

    py::class_<dec_matrix<v1::DecompressUInt64>> du64(m, "decompress_u64", py::module_local());
    du64.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressUInt64>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressUInt64>::sensors)
        .def("reserve", &dec_matrix<v1::DecompressUInt64>::reserve)
        .def("column_major", &dec_matrix<v1::DecompressUInt64>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressUInt64>::row_major);
}

