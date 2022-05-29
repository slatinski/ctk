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
#include "api_bindings.h"
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

    struct ctkpy_version
    {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
        uint32_t build;

        ctkpy_version()
        : major{ CTK_MAJOR }
        , minor{ CTK_MINOR }
        , patch{ CTK_PATCH }
        , build{ CTK_BUILD } {
        }

        friend auto operator==(const ctkpy_version&, const ctkpy_version&) -> bool = default;
        friend auto operator!=(const ctkpy_version&, const ctkpy_version&) -> bool = default;
    };

    auto operator<<(std::ostream& os, const ctkpy_version& x) -> std::ostream& {
        os << x.major << "."  << x.minor << "."  << x.patch << "."  << x.build;
        return os;
    }


    using trigger_v4_tuple = std::tuple<std::string, int64_t, int64_t, std::string, std::string, std::string>;
    using channel_v4_tuple = std::tuple<std::string, std::string, std::string>; // active label, ref label, unit
    using trigger_tuple = std::tuple<int64_t, std::string>;


    static
    auto triggertuple2v1trigger(const trigger_tuple& x) -> v1::Trigger {
        const auto[s, c]{ x };
        return { s, c };
    }

    static
    auto ch2elc(const channel_v4_tuple& x) -> v1::Electrode {
        const auto[label, reference, unit]{ x };
        const v1::Electrode y{ label, reference, unit };
        ctk::impl::validate(y);
        return y;
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



    struct reader_v4
    {
        v1::ReaderReflib reader;

        explicit
        reader_v4(const std::string& fname)
        : reader{ fname } {
        }

        auto get_sample_count() const -> int64_t {
            return reader.SampleCount;
        }

        auto get_channel_count() const -> size_t {
          return reader.ParamEeg.Electrodes.size();
        }

        auto get_channel(size_t i) const -> channel_v4_tuple {
            const auto size{ reader.ParamEeg.Electrodes.size() };
            if (size <= i) {
                std::ostringstream oss;
                oss << "[cnt_in::get_channel] invalid index " << (i + 1) << "/" << size;
                throw std::runtime_error(oss.str());
            }

            const v1::Electrode& x{ reader.ParamEeg.Electrodes[i] };
            return { x.ActiveLabel, x.Reference, x.Unit };
        }

        auto get_sample_frequency() const -> double {
            return reader.ParamEeg.SamplingFrequency;
        }

        auto get_samples(int64_t i, int64_t amount) -> std::vector<float> {
            return reader.RangeV4(i, amount);
        }

        auto get_trigger_count() const -> size_t {
            return reader.Triggers.size();
        }

        auto get_trigger(size_t i) const -> trigger_v4_tuple {
            const auto size{ reader.Triggers.size() };
            if (size <= i) {
                std::ostringstream oss;
                oss << "[cnt_in::get_trigger] invalid index " << (i + 1) << "/" << size;
                throw std::runtime_error(oss.str());
            }

            const v1::Trigger& trigger{ reader.Triggers[i] };
            const int64_t duration{ 0 };
            const std::string condition, descr, impedances;
            return { trigger.Code, trigger.Sample, duration, condition, descr, impedances };
        }
    };

    static
    auto read_cnt(const std::string& fname) -> std::unique_ptr<reader_v4> {
        return std::make_unique<reader_v4>(fname);
    }


    struct writer_v4
    {
        v1::WriterReflib writer;

        writer_v4(const std::string& fname, double sample_rate, const std::vector<channel_v4_tuple>& channels, int cnt64)
        : writer{ fname, int2riff(cnt64) } {
            writer.ParamEeg.SamplingFrequency = sample_rate;
            writer.ParamEeg.Electrodes = ch2elcs(channels);
            writer.ParamEeg.StartTime = std::chrono::system_clock::now();
        }

        auto add_samples(const std::vector<float>& xs) -> void {
            writer.cnt_ptr()->LibeepV4(xs);
        }

        auto close() -> void {
            writer.Close();
        }
    };

    static
    auto write_cnt(const std::string& fname, double sample_rate, const std::vector<channel_v4_tuple>& channels, int rf64 = 0) -> std::unique_ptr<writer_v4> {
        return std::make_unique<writer_v4>(fname, sample_rate, channels, rf64);
    }


    template<typename T>
    auto from_row_major(const py::array_t<T>& xs) -> std::vector<T> {
        if (xs.ndim() != 2) {
            std::ostringstream oss;
            oss << "[from_row_major] invalid input array dimensions: expected 2, got " << xs.ndim();
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
            oss << "[from_column_major] invalid input array dimensions: expected 2, got " << xs.ndim();
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

        auto sensors(int64_t x) -> bool {
            return compress.Sensors(x);
        }

        auto order(const std::vector<int16_t>& xs) -> bool {
            return compress.Sensors(xs);
        }

        auto column_major(const py::array_t<T>& xs) -> std::vector<uint8_t> {
            const int64_t length{ xs.shape()[0] };
            return compress.RowMajor(from_column_major(xs), length);
        }

        auto row_major(const py::array_t<T>& xs) -> std::vector<uint8_t> {
            const int64_t length{ xs.shape()[1] };
            return compress.RowMajor(from_row_major(xs), length);
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

        auto sensors(int64_t x) -> bool {
            return decompress.Sensors(x);
        }

        auto order(const std::vector<int16_t>& xs) -> bool {
            return decompress.Sensors(xs);
        }

        auto column_major(const std::vector<uint8_t>& xs, int64_t length) -> py::array_t<T> {
            if (length < 1) {
                throw std::runtime_error("[column_major] invalid length");
            }

            // TODO: add height getter to the matrix interface and pass temporary to to_column_major?
            const auto ys{ decompress.ColumnMajor(xs, length) };
            const ssize_t h{ ctk::impl::cast(ctk::impl::vsize(ys) / length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(ys, l, h);
        }

        auto row_major(const std::vector<uint8_t>& xs, int64_t length) -> py::array_t<T> {
            if (length < 1) {
                throw std::runtime_error("[row_major] invalid length");
            }

            // TODO: add height getter to the matrix interface and pass temporary to to_row_major?
            const auto ys{ decompress.RowMajor(xs, length) };
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
        .value("unknown", v1::Sex::Unknown)
        .value("male", v1::Sex::Male)
        .value("female", v1::Sex::Female);

    py::enum_<v1::Handedness>(m, "handedness", py::module_local())
        .value("unknown", v1::Handedness::Unknown)
        .value("left", v1::Handedness::Left)
        .value("right", v1::Handedness::Right)
        .value("mixed", v1::Handedness::Mixed);

    py::class_<v1::FileVersion> fv(m, "file_version", py::module_local());
    fv.def(py::init<uint32_t, uint32_t>())
      .def_readwrite("major", &v1::FileVersion::Major)
      .def_readwrite("minor", &v1::FileVersion::Minor)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::FileVersion& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.Major, x.Minor);
        },
        [](py::tuple xs) -> v1::FileVersion { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("[file_version::__setstate__] invalid");
            }
            return v1::FileVersion{ xs[0].cast<uint32_t>(), xs[1].cast<uint32_t>() };
        }
      ))
      .def("__copy__", [](const v1::FileVersion& x){ return v1::FileVersion(x); })
      .def("__deepcopy__", [](const v1::FileVersion& x, py::dict){ return v1::FileVersion(x); })
      .def("__repr__", [](const v1::FileVersion& x) { return print(x); });

    py::class_<v1::Trigger> t(m, "trigger", py::module_local());
    t.def(py::init<int64_t, const std::string&>())
     .def_readwrite("sample", &v1::Trigger::Sample)
     .def_property("code"
                  , [](const v1::Trigger& self) -> std::string {
                      return self.Code;
                  }
                  , [](v1::Trigger& self, const std::string& x) -> void {
                      if (v1::sizes::evt_trigger_code < x.size()) { // TODO: duplication
                          std::ostringstream oss;
                          oss << "[trigger::code] " << x << " longer than " << v1::sizes::evt_trigger_code << " bytes";
                          const auto msg{ oss.str() };
                          throw std::runtime_error(msg);
                      }
                      self.Code = x;
                  }
      )
     .def(py::self == py::self) // __eq__
     .def(py::self != py::self) // __ne__
     .def(py::pickle(
        [](const v1::Trigger& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.Sample, x.Code);
        },
        [](py::tuple xs) -> v1::Trigger { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("[trigger::__setstate__] invalid");
            }
            return v1::Trigger { xs[0].cast<int64_t>(), xs[1].cast<std::string>() };
        }
      ))
     .def("__copy__", [](const v1::Trigger& x){ return v1::Trigger(x); })
     .def("__deepcopy__", [](const v1::Trigger& x, py::dict){ return v1::Trigger(x); })
     .def("__repr__", [](const v1::Trigger& x) { return print(x); });

    py::class_<v1::Electrode> e(m, "electrode", py::module_local());
    e.def(py::init<const std::string&, const std::string&, const std::string&, double, double>())
     .def_property("label"
                  , [](const v1::Electrode& self) -> std::string {
                      return self.ActiveLabel;
                  }
                  , [](v1::Electrode& self, const std::string& x) -> void {
                      ctk::impl::validate_electrode_label_reflib(x);
                      self.ActiveLabel = x;
                  }
      )
     .def_property("reference"
                  , [](const v1::Electrode& self) -> std::string {
                      return self.Reference;
                  }
                  , [](v1::Electrode& self, const std::string& x) -> void {
                      ctk::impl::validate_electrode_reference_reflib(x);
                      self.Reference = x;
                  }
      )
     .def_property("unit"
                  , [](const v1::Electrode& self) -> std::string {
                      return self.Unit;
                  }
                  , [](v1::Electrode& self, const std::string& x) -> void {
                      ctk::impl::validate_electrode_unit_reflib(x);
                      self.Unit = x;
                  }
      )
     .def_property("status"
                  , [](const v1::Electrode& self) -> std::string {
                      return self.Status;
                  }
                  , [](v1::Electrode& self, const std::string& x) -> void {
                      ctk::impl::validate_electrode_status_reflib(x);
                      self.Status = x;
                  }
      )
     .def_property("type"
                  , [](const v1::Electrode& self) -> std::string {
                      return self.Type;
                  }
                  , [](v1::Electrode& self, const std::string& x) -> void {
                      ctk::impl::validate_electrode_type_reflib(x);
                      self.Type = x;
                  }
      )
     .def_property("iscale"
                  , [](const v1::Electrode& self) -> double {
                      return self.IScale;
                  }
                  , [](v1::Electrode& self, double x) -> void {
                      if (!std::isfinite(x)) { // TODO: duplication
                          throw std::runtime_error("[electrode::iscale] invalid");
                      }
                      self.IScale = x;
                  }
      )
     .def_property("rscale"
                  , [](const v1::Electrode& self) -> double {
                      return self.RScale;
                  }
                  , [](v1::Electrode& self, double x) -> void {
                      if (!std::isfinite(x)) { // TODO: duplication
                          throw std::runtime_error("[electrode::rscale] invalid");
                      }
                      self.RScale = x;
                  }
      )
     .def(py::self == py::self) // __eq__
     .def(py::self != py::self) // __ne__
     .def(py::pickle(
        [](const v1::Electrode& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.ActiveLabel, x.Reference, x.Unit, x.Status, x.Type, x.IScale, x.RScale);
        },
        [](py::tuple xs) -> v1::Electrode { // __setstate__
            if (xs.size() != 7) {
                throw std::runtime_error("[electrode::__setstate__] invalid");
            }

            v1::Electrode y;
            y.ActiveLabel = xs[0].cast<std::string>();
            y.Reference = xs[1].cast<std::string>();
            y.Unit = xs[2].cast<std::string>();
            y.Status = xs[3].cast<std::string>();
            y.Type = xs[4].cast<std::string>();
            y.IScale = xs[5].cast<double>();
            y.RScale = xs[6].cast<double>();
            return y;

        }
      ))
     .def("__copy__", [](const v1::Electrode& x){ return v1::Electrode(x); })
     .def("__deepcopy__", [](const v1::Electrode& x, py::dict){ return v1::Electrode(x); })
     .def("__repr__", [](const v1::Electrode& x) { return print(x); });

    m.def("electrodes", &ch2elcs, "Converts [('active', 'reference', 'unit')] to [ctk.electrode]; if unit is uV, range scale equals 1/256 else 1");

    py::class_<v1::TimeSeries> ts(m, "time_series", py::module_local());
    ts.def(py::init<std::chrono::system_clock::time_point, double, const std::vector<v1::Electrode>&, int64_t>())
      .def_readwrite("epoch_length", &v1::TimeSeries::EpochLength)
      .def_readwrite("sampling_frequency", &v1::TimeSeries::SamplingFrequency)
      .def_readwrite("start_time", &v1::TimeSeries::StartTime)
      .def_readwrite("electrodes", &v1::TimeSeries::Electrodes)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::TimeSeries& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.StartTime, x.SamplingFrequency, x.Electrodes, x.EpochLength);
        },
        [](py::tuple xs) -> v1::TimeSeries { // __setstate__
            if (xs.size() != 4) {
                throw std::runtime_error("[time_series::__setstate__] invalid");
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
     .def_readwrite("hospital", &v1::Info::Hospital)
     .def_readwrite("test_name", &v1::Info::TestName)
     .def_readwrite("test_serial", &v1::Info::TestSerial)
     .def_readwrite("physician", &v1::Info::Physician)
     .def_readwrite("technician", &v1::Info::Technician)
     .def_readwrite("machine_make", &v1::Info::MachineMake)
     .def_readwrite("machine_model", &v1::Info::MachineModel)
     .def_readwrite("machine_sn", &v1::Info::MachineSn)
     .def_readwrite("subject_name", &v1::Info::SubjectName)
     .def_readwrite("subject_id", &v1::Info::SubjectId)
     .def_readwrite("subject_phone", &v1::Info::SubjectPhone)
     .def_readwrite("subject_address", &v1::Info::SubjectAddress)
     .def_readwrite("subject_sex", &v1::Info::SubjectSex)
     .def_readwrite("subject_handedness", &v1::Info::SubjectHandedness)
     .def_readwrite("subject_dob", &v1::Info::SubjectDob)
     .def_readwrite("comment", &v1::Info::Comment)
     .def(py::self == py::self) // __eq__
     .def(py::self != py::self) // __ne__
     .def(py::pickle(
        [](const v1::Info& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.Hospital,
                                  x.TestName,
                                  x.TestSerial,
                                  x.Physician,
                                  x.Technician,
                                  x.MachineMake,
                                  x.MachineModel,
                                  x.MachineSn,
                                  x.SubjectName,
                                  x.SubjectId,
                                  x.SubjectAddress,
                                  x.SubjectPhone,
                                  x.SubjectSex,
                                  x.SubjectHandedness,
                                  x.SubjectDob,
                                  x.Comment);
        },
        [](py::tuple xs) -> v1::Info { // __setstate__
            if (xs.size() != 16) {
                throw std::runtime_error("[info::__setstate__] invalid");
            }
            v1::Info y;
            y.Hospital = xs[0].cast<std::string>();
            y.TestName = xs[1].cast<std::string>();
            y.TestSerial = xs[2].cast<std::string>();
            y.Physician = xs[3].cast<std::string>();
            y.Technician = xs[4].cast<std::string>();
            y.MachineMake = xs[5].cast<std::string>();
            y.MachineModel = xs[6].cast<std::string>();
            y.MachineSn = xs[7].cast<std::string>();
            y.SubjectName = xs[8].cast<std::string>();
            y.SubjectId = xs[9].cast<std::string>();
            y.SubjectAddress = xs[10].cast<std::string>();
            y.SubjectPhone = xs[11].cast<std::string>();
            y.SubjectSex = xs[12].cast<v1::Sex>();
            y.SubjectHandedness = xs[13].cast<v1::Handedness>();
            y.SubjectDob = xs[14].cast<std::chrono::system_clock::time_point>();
            y.Comment = xs[15].cast<std::string>();
            return y;
        }
      ))
     .def("__copy__", [](const v1::Info& x){ return v1::Info(x); })
     .def("__deepcopy__", [](const v1::Info& x, py::dict){ return v1::Info(x); })
     .def("__repr__", [](const v1::Info& x) { return print(x); });

    py::class_<v1::UserFile> uf(m, "user_file", py::module_local());
    uf.def(py::init<const std::string&, const std::string&>())
      .def_readonly("label", &v1::UserFile::Label)
      .def_readwrite("file_name", &v1::UserFile::FileName)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::UserFile& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.Label, x.FileName);
        },
        [](py::tuple xs) -> v1::UserFile { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("[user_file::__setstate__] invalid");
            }
            return v1::UserFile{ xs[0].cast<std::string>(), xs[1].cast<std::string>() };
        }
      ))
      .def("__copy__", [](const v1::UserFile& x){ return v1::UserFile(x); })
      .def("__deepcopy__", [](const v1::UserFile& x, py::dict){ return v1::UserFile(x); })
      .def("__repr__", [](const v1::UserFile& x) { return print(x); });


    py::class_<v1::EventImpedance> ei(m, "event_impedance", py::module_local());
    ei.def(py::init<std::chrono::system_clock::time_point, const std::vector<float>&>())
      .def_readwrite("stamp", &v1::EventImpedance::Stamp)
      .def_readwrite("values", &v1::EventImpedance::Values)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::EventImpedance& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.Stamp, x.Values);
        },
        [](py::tuple xs) -> v1::EventImpedance { // __setstate__
            if (xs.size() != 2) {
                throw std::runtime_error("[event_impedance::__setstate__] invalid");
            }
            return v1::EventImpedance{ xs[0].cast<std::chrono::system_clock::time_point>(), xs[1].cast<std::vector<float>>() };
        }
      ))
      .def("__copy__", [](const v1::EventImpedance& x){ return v1::EventImpedance(x); })
      .def("__deepcopy__", [](const v1::EventImpedance& x, py::dict){ return v1::EventImpedance(x); })
      .def("__repr__", [](const v1::EventImpedance& x) {
          std::ostringstream oss;
          oss << "(" << x.Values.size() << " items)";
          return oss.str();
      });

    py::class_<v1::EventVideo> ev(m, "event_video", py::module_local());
    ev.def(py::init<std::chrono::system_clock::time_point, double, int32_t>())
      .def_readwrite("stamp", &v1::EventVideo::Stamp)
      .def_readwrite("duration", &v1::EventVideo::Duration)
      .def_readwrite("trigger_code", &v1::EventVideo::TriggerCode)
      .def_readwrite("condition_label", &v1::EventVideo::ConditionLabel)
      .def_readwrite("description", &v1::EventVideo::Description)
      .def_readwrite("video_file", &v1::EventVideo::VideoFile)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::EventVideo& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.Stamp, x.Duration, x.TriggerCode, x.ConditionLabel, x.Description, x.VideoFile);
        },
        [](py::tuple xs) -> v1::EventVideo { // __setstate__
            if (xs.size() != 6) {
                throw std::runtime_error("[event_video::__setstate__] invalid");
            }
            v1::EventVideo y;
            y.Stamp = xs[0].cast<std::chrono::system_clock::time_point>();
            y.Duration = xs[1].cast<double>();
            y.TriggerCode = xs[2].cast<int32_t>();
            y.ConditionLabel = xs[3].cast<std::wstring>();
            y.Description = xs[4].cast<std::string>();
            y.VideoFile = xs[5].cast<std::wstring>();
            return y;
        }
      ))
      .def("__copy__", [](const v1::EventVideo& x){ return v1::EventVideo(x); })
      .def("__deepcopy__", [](const v1::EventVideo& x, py::dict){ return v1::EventVideo(x); })
      .def("__repr__", [](const v1::EventVideo& x) {
          std::ostringstream oss;
          oss << "(" << x.Duration << " " << x.TriggerCode << ")";
          return oss.str();
      });

    py::class_<v1::EventEpoch> ee(m, "event_epoch", py::module_local());
    ee.def(py::init<std::chrono::system_clock::time_point, double, double, int32_t>())
      .def_readwrite("stamp", &v1::EventEpoch::Stamp)
      .def_readwrite("duration", &v1::EventEpoch::Duration)
      .def_readwrite("offset", &v1::EventEpoch::Offset)
      .def_readwrite("trigger_code", &v1::EventEpoch::TriggerCode)
      .def_readwrite("condition_label", &v1::EventEpoch::ConditionLabel)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const v1::EventEpoch& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.Stamp, x.Duration, x.Offset, x.TriggerCode, x.ConditionLabel);
        },
        [](py::tuple xs) -> v1::EventEpoch { // __setstate__
            if (xs.size() != 5) {
                throw std::runtime_error("[event_epoch::__setstate__] invalid");
            }
            v1::EventEpoch y;
            y.Stamp = xs[0].cast<std::chrono::system_clock::time_point>();
            y.Duration = xs[1].cast<double>();
            y.Offset = xs[2].cast<double>();
            y.TriggerCode = xs[3].cast<int32_t>();
            y.ConditionLabel = xs[4].cast<std::wstring>();
            return y;
        }
      ))
      .def("__copy__", [](const v1::EventEpoch& x){ return v1::EventEpoch(x); })
      .def("__deepcopy__", [](const v1::EventEpoch& x, py::dict){ return v1::EventEpoch(x); })
      .def("__repr__", [](const v1::EventEpoch& x) {
          std::ostringstream oss;
          oss << "(" << x.Duration << " " << x.Offset << " " << x.TriggerCode << ")";
          return oss.str();
      });


    py::class_<ctkpy_version> lv(m, "lib_version", py::module_local());
    lv.def(py::init<>())
      .def_readonly("major", &ctkpy_version::major)
      .def_readonly("minor", &ctkpy_version::minor)
      .def_readonly("patch", &ctkpy_version::patch)
      .def_readonly("build", &ctkpy_version::build)
      .def(py::self == py::self) // __eq__
      .def(py::self != py::self) // __ne__
      .def(py::pickle(
        [](const ctkpy_version& x) -> py::tuple { // __getstate__
            return py::make_tuple(x.major, x.minor, x.patch, x.build);
        },
        [](py::tuple xs) -> ctkpy_version { // __setstate__
            if (xs.size() != 4) {
                throw std::runtime_error("[lib_version::__setstate__] invalid");
            }
            ctkpy_version x;
            x.major = xs[0].cast<uint32_t>();
            x.minor = xs[1].cast<uint32_t>();
            x.patch = xs[2].cast<uint32_t>();
            x.build = xs[3].cast<uint32_t>();
            return x;
        }
      ))
      .def("__copy__", [](const ctkpy_version& x){ return ctkpy_version(x); })
      .def("__deepcopy__", [](const ctkpy_version& x, py::dict){ return ctkpy_version(x); })
      .def("__repr__", [](const ctkpy_version& x) { return print(x); });


    // 1) cnt + evt file
    py::class_<v1::WriterReflib> rw(m, "writer_reflib", py::module_local());
    rw.def(py::init<const std::string&, v1::RiffType>(), py::arg("fname"), py::arg("type") = v1::RiffType::riff64 )
      .def_readwrite("param", &v1::WriterReflib::ParamEeg)
      .def_readwrite("info", &v1::WriterReflib::RecordingInfo)
      .def("close", &v1::WriterReflib::Close, "Constructs the output cnt/evt files")
      .def("add_electrode", [](v1::WriterReflib& self, const v1::Electrode& x) -> void {
          ctk::impl::validate(x);
          self.ParamEeg.Electrodes.push_back(x);
      })
      .def("add_electrode", [](v1::WriterReflib& self, const channel_v4_tuple& x) -> void {
          self.ParamEeg.Electrodes.push_back(ch2elc(x));
      })
      .def("row_major", [](v1::WriterReflib& self, const py::array_t<double>& xs) -> void {
          self.cnt_ptr()->RowMajor(from_row_major(xs));
      })
      .def("column_major", [](v1::WriterReflib& self, const py::array_t<double>& xs) -> void {
          self.cnt_ptr()->RowMajor(from_column_major(xs));
      })
      .def("trigger", [](v1::WriterReflib& self, const v1::Trigger& x) -> void {
          self.cnt_ptr()->AddTrigger(x);
      })
      .def("trigger", [](v1::WriterReflib& self, const trigger_tuple& x) -> void {
          self.cnt_ptr()->AddTrigger(triggertuple2v1trigger(x));
      })
      .def("triggers", [](v1::WriterReflib& self, const std::vector<v1::Trigger>& xs) -> void {
          self.cnt_ptr()->AddTriggers(xs);
      })
      .def("triggers", [](v1::WriterReflib& self, const std::vector<trigger_tuple>& xs) -> void {
          std::vector<v1::Trigger> ys(xs.size());
          std::transform(begin(xs), end(xs), begin(ys), triggertuple2v1trigger);
          self.cnt_ptr()->AddTriggers(ys);
      })
      .def("impedance", [](v1::WriterReflib& self, const v1::EventImpedance& x) -> void {
          self.evt_ptr()->AddImpedance(x);
      })
      .def("impedances", [](v1::WriterReflib& self, const std::vector<v1::EventImpedance>& xs) -> void {
          self.evt_ptr()->AddImpedances(xs);
      })
      .def("video", [](v1::WriterReflib& self, const v1::EventVideo& x) -> void {
          self.evt_ptr()->AddVideo(x);
      })
      .def("videos", [](v1::WriterReflib& self, const std::vector<v1::EventVideo>& xs) -> void {
          self.evt_ptr()->AddVideos(xs);
      })
      .def("epoch", [](v1::WriterReflib& self, const v1::EventEpoch& x) -> void {
          self.evt_ptr()->AddEpoch(x);
      })
      .def("epochs", [](v1::WriterReflib& self, const std::vector<v1::EventEpoch>& xs) -> void {
          self.evt_ptr()->AddEpochs(xs);
      })
      .def("embed", [](v1::WriterReflib& self, const v1::UserFile& x) -> void {
          self.cnt_ptr()->Embed(x);
      })
      .def("__repr__", [](const v1::WriterReflib& x) { return print(x.ParamEeg); });

    py::class_<v1::ReaderReflib> rr(m, "reader_reflib", py::module_local());
    rr.def(py::init<const std::string&>())
      .def_readonly("cnt_type", &v1::ReaderReflib::Type)
      .def_readonly("sample_count", &v1::ReaderReflib::SampleCount)
      .def_readonly("epoch_count", &v1::ReaderReflib::EpochCount)
      .def_readonly("param", &v1::ReaderReflib::ParamEeg)
      .def_readonly("triggers", &v1::ReaderReflib::Triggers)
      .def_readonly("impedances", &v1::ReaderReflib::Impedances)
      .def_readonly("videos", &v1::ReaderReflib::Videos)
      .def_readonly("epochs", &v1::ReaderReflib::Epochs)
      .def_readonly("info", &v1::ReaderReflib::RecordingInfo)
      .def("row_major", [] (v1::ReaderReflib& self, int64_t i, int64_t length) -> py::array_t<double> {
            auto xs{ self.RangeRowMajor(i, length) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::row_major] can not load range");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because RangeRowMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_row_major(xs, l, h);
      })
      .def("column_major", [] (v1::ReaderReflib& self, int64_t i, int64_t length) -> py::array_t<double> {
            auto xs{ self.RangeColumnMajor(i, length) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::column_major] can not load range");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because RangeColumnMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(xs, l, h);
      })
      .def("epoch_row_major", [] (v1::ReaderReflib& self, int64_t i) -> py::array_t<double> {
            auto xs{ self.EpochRowMajor(i) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::epoch_row_major] can not load epoch");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because EpochRowMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const size_t length{ xs.size() / channels };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_row_major(xs, l, h);
      })
      .def("epoch_column_major", [] (v1::ReaderReflib& self, int64_t i) -> py::array_t<double> {
            auto xs{ self.EpochColumnMajor(i) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::epoch_column_major] can not load epoch");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because EpochColumnMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const size_t length{ xs.size() / channels };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(xs, l, h);
      })
      .def("epoch_compressed", &v1::ReaderReflib::EpochCompressed)
      .def_readwrite("embedded", &v1::ReaderReflib::Embedded)
      .def("extract_embedded", &v1::ReaderReflib::ExtractEmbedded)
      .def("__repr__", [](const v1::ReaderReflib& x) { return print(x.ParamEeg); });


    py::class_<v1::ReaderReflibUnpacked> rru(m, "reader_reflib_unpacked", py::module_local());
    rru.def(py::init<const std::string&>())
      .def_readonly("cnt_type", &v1::ReaderReflibUnpacked::Type)
      .def_readonly("sample_count", &v1::ReaderReflibUnpacked::SampleCount)
      .def_readonly("epoch_count", &v1::ReaderReflibUnpacked::EpochCount)
      .def_readonly("param", &v1::ReaderReflibUnpacked::ParamEeg)
      .def_readonly("triggers", &v1::ReaderReflibUnpacked::Triggers)
      .def_readonly("impedances", &v1::ReaderReflibUnpacked::Impedances)
      .def_readonly("videos", &v1::ReaderReflibUnpacked::Videos)
      .def_readonly("epochs", &v1::ReaderReflibUnpacked::Epochs)
      .def_readonly("info", &v1::ReaderReflibUnpacked::RecordingInfo)
      .def("row_major", [] (v1::ReaderReflibUnpacked& self, int64_t i, int64_t length) -> py::array_t<double> {
            auto xs{ self.RangeRowMajor(i, length) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::row_major] can not load range");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because RangeRowMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_row_major(xs, l, h);
      })
      .def("column_major", [] (v1::ReaderReflibUnpacked& self, int64_t i, int64_t length) -> py::array_t<double> {
            auto xs{ self.RangeColumnMajor(i, length) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::column_major] can not load range");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because RangeColumnMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(xs, l, h);
      })
      .def("epoch_row_major", [] (v1::ReaderReflibUnpacked& self, int64_t i) -> py::array_t<double> {
            auto xs{ self.EpochRowMajor(i) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::epoch_row_major] can not load epoch");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because EpochRowMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const size_t length{ xs.size() / channels };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_row_major(xs, l, h);
      })
      .def("epoch_column_major", [] (v1::ReaderReflibUnpacked& self, int64_t i) -> py::array_t<double> {
            auto xs{ self.EpochColumnMajor(i) };
            if (xs.empty()) {
                throw std::runtime_error("[reader_reflib::epoch_column_major] can not load epoch");
            }

            assert(!self.ParamEeg.Electrodes.empty()); // because EpochColumnMajor succeeded

            const size_t channels{ self.ParamEeg.Electrodes.size() };
            const size_t length{ xs.size() / channels };
            const ssize_t l{ ctk::impl::cast(length, ssize_t{}, ctk::impl::ok{}) };
            const ssize_t h{ ctk::impl::cast(channels, ssize_t{}, ctk::impl::ok{}) };
            return to_column_major(xs, l, h);
      })
      .def("epoch_compressed", &v1::ReaderReflibUnpacked::EpochCompressed)
      .def("__repr__", [](const v1::ReaderReflibUnpacked& x) { return print(x.ParamEeg); });


    // 2) evt file only
    py::class_<v1::EventReader> er(m, "event_reader", py::module_local());
    er.def(py::init<const std::string&>())
      .def_property_readonly("count_impedances", &v1::EventReader::ImpedanceCount)
      .def_property_readonly("count_videos", &v1::EventReader::VideoCount)
      .def_property_readonly("count_epochs", &v1::EventReader::EpochCount)
      .def("impedance", &v1::EventReader::ImpedanceEvent)
      .def("video", &v1::EventReader::VideoEvent)
      .def("epoch", &v1::EventReader::EpochEvent)
      .def("impedances", &v1::EventReader::ImpedanceEvents)
      .def("videos", &v1::EventReader::VideoEvents)
      .def("epochs", &v1::EventReader::EpochEvents);

    py::class_<v1::EventWriter> ew(m, "event_writer", py::module_local());
    ew.def(py::init<const std::string&>())
      .def("impedance", &v1::EventWriter::AddImpedance)
      .def("video", &v1::EventWriter::AddVideo)
      .def("epoch", &v1::EventWriter::AddEpoch)
      .def("impedances", &v1::EventWriter::AddImpedances)
      .def("videos", &v1::EventWriter::AddVideos)
      .def("epochs", &v1::EventWriter::AddEpochs)
      .def("close", &v1::EventWriter::Close, "Constructs the output evt file");


    // 3) pyeep interface
    py::class_<reader_v4> lr(m, "cnt_in", py::module_local());
    lr.def(py::init<const std::string&>())
      .def("get_channel_count", &reader_v4::get_channel_count)
      .def("get_channel", &reader_v4::get_channel)
      .def("get_sample_frequency", &reader_v4::get_sample_frequency)
      .def("get_sample_count", &reader_v4::get_sample_count)
      .def("get_samples", &reader_v4::get_samples)
      .def("get_trigger_count", &reader_v4::get_trigger_count)
      .def("get_trigger", &reader_v4::get_trigger);

    m.def("read_cnt", &read_cnt, "Opens a CNT file for reading");

    py::class_<writer_v4> lw(m, "cnt_out", py::module_local());
    lw.def(py::init<const std::string&, double, const std::vector<channel_v4_tuple>&, int>())
      .def("add_samples", &writer_v4::add_samples)
      .def("close", &writer_v4::close, "Constructs the output cnt file");

    m.def("write_cnt", &write_cnt, "Opens a CNT file for writing");

    // 4) compression
    py::class_<enc_matrix<v1::CompressReflib>> cr(m, "compress_reflib", py::module_local());
    cr.def(py::init<>())
      .def_property("sensors", nullptr, &enc_matrix<v1::CompressReflib>::sensors)
      .def_property("order", nullptr, &enc_matrix<v1::CompressReflib>::order)
      .def("column_major", &enc_matrix<v1::CompressReflib>::column_major)
      .def("row_major", &enc_matrix<v1::CompressReflib>::row_major);

    py::class_<dec_matrix<v1::DecompressReflib>> dr(m, "decompress_reflib", py::module_local());
    dr.def(py::init<>())
      .def_property("sensors", nullptr, &dec_matrix<v1::DecompressReflib>::sensors)
      .def_property("order", nullptr, &dec_matrix<v1::DecompressReflib>::sensors)
      .def("column_major", &dec_matrix<v1::DecompressReflib>::column_major)
      .def("row_major", &dec_matrix<v1::DecompressReflib>::row_major);

    py::class_<enc_matrix<v1::CompressInt16>> ci16(m, "compress_i16", py::module_local());
    ci16.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressInt16>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressInt16>::order)
        .def("column_major", &enc_matrix<v1::CompressInt16>::column_major)
        .def("row_major", &enc_matrix<v1::CompressInt16>::row_major);

    py::class_<dec_matrix<v1::DecompressInt16>> di16(m, "decompress_i16", py::module_local());
    di16.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressInt16>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressInt16>::sensors)
        .def("column_major", &dec_matrix<v1::DecompressInt16>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressInt16>::row_major);

    py::class_<enc_matrix<v1::CompressInt32>> ci32(m, "compress_i32", py::module_local());
    ci32.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressInt32>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressInt32>::order)
        .def("column_major", &enc_matrix<v1::CompressInt32>::column_major)
        .def("row_major", &enc_matrix<v1::CompressInt32>::row_major);

    py::class_<dec_matrix<v1::DecompressInt32>> di32(m, "decompress_i32", py::module_local());
    di32.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressInt32>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressInt32>::sensors)
        .def("column_major", &dec_matrix<v1::DecompressInt32>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressInt32>::row_major);

    py::class_<enc_matrix<v1::CompressInt64>> ci64(m, "compress_i64", py::module_local());
    ci64.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressInt64>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressInt64>::order)
        .def("column_major", &enc_matrix<v1::CompressInt64>::column_major)
        .def("row_major", &enc_matrix<v1::CompressInt64>::row_major);

    py::class_<dec_matrix<v1::DecompressInt64>> di64(m, "decompress_i64", py::module_local());
    di64.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressInt64>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressInt64>::sensors)
        .def("column_major", &dec_matrix<v1::DecompressInt64>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressInt64>::row_major);


    py::class_<enc_matrix<v1::CompressUInt16>> cu16(m, "compress_u16", py::module_local());
    cu16.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressUInt16>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressUInt16>::order)
        .def("column_major", &enc_matrix<v1::CompressUInt16>::column_major)
        .def("row_major", &enc_matrix<v1::CompressUInt16>::row_major);

    py::class_<dec_matrix<v1::DecompressUInt16>> du16(m, "decompress_u16", py::module_local());
    du16.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressUInt16>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressUInt16>::sensors)
        .def("column_major", &dec_matrix<v1::DecompressUInt16>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressUInt16>::row_major);

    py::class_<enc_matrix<v1::CompressUInt32>> cu32(m, "compress_u32", py::module_local());
    cu32.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressUInt32>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressUInt32>::order)
        .def("column_major", &enc_matrix<v1::CompressUInt32>::column_major)
        .def("row_major", &enc_matrix<v1::CompressUInt32>::row_major);

    py::class_<dec_matrix<v1::DecompressUInt32>> du32(m, "decompress_u32", py::module_local());
    du32.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressUInt32>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressUInt32>::sensors)
        .def("column_major", &dec_matrix<v1::DecompressUInt32>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressUInt32>::row_major);

    py::class_<enc_matrix<v1::CompressUInt64>> cu64(m, "compress_u64", py::module_local());
    cu64.def(py::init<>())
        .def_property("sensors", nullptr, &enc_matrix<v1::CompressUInt64>::sensors)
        .def_property("order", nullptr, &enc_matrix<v1::CompressUInt64>::order)
        .def("column_major", &enc_matrix<v1::CompressUInt64>::column_major)
        .def("row_major", &enc_matrix<v1::CompressUInt64>::row_major);

    py::class_<dec_matrix<v1::DecompressUInt64>> du64(m, "decompress_u64", py::module_local());
    du64.def(py::init<>())
        .def_property("sensors", nullptr, &dec_matrix<v1::DecompressUInt64>::sensors)
        .def_property("order", nullptr, &dec_matrix<v1::DecompressUInt64>::sensors)
        .def("column_major", &dec_matrix<v1::DecompressUInt64>::column_major)
        .def("row_major", &dec_matrix<v1::DecompressUInt64>::row_major);
}

