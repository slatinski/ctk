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

#include "api_c.h"

#include <sstream>
#include <cmath>

#include "api_bindings.h"
#include "arithmetic.h"
#include "logger.h"
#include "file/cnt_epoch.h" // for tm2timepoint

namespace {

    using namespace std::chrono;
    using namespace ctk::api::v1;
    using namespace ctk::impl;

    auto timepoint2timespec(system_clock::time_point x) -> struct timespec {
        const auto span{ x.time_since_epoch() };
        const auto s{ floor<seconds>(span) };
        const auto ns{ duration_cast<nanoseconds>(span - s) };
        return timespec{ static_cast<time_t>(s.count()), static_cast<long>(ns.count()) };
    }

    auto timespec2timepoint(const struct timespec& x) -> system_clock::time_point {
        using Precision = system_clock::duration;

        const seconds s{ x.tv_sec };
        const nanoseconds ns{ x.tv_nsec };
        return system_clock::time_point{ duration_cast<Precision>(s + ns) };
    }


    auto string2range(const std::string& x, char* y, size_t ysize) -> void {
        if (y == nullptr || ysize == 0) {
            return;
        }

        const size_t xsize{ x.size() };
        if (xsize == 0) {
            *y = 0;
            return;
        }

        size_t shortest{ std::min(xsize, ysize) };
        assert(shortest != 0);
        if (shortest == ysize) {
            --shortest; // trailing 0
        }
        const ptrdiff_t length{ ctk::impl::cast(shortest, ptrdiff_t{}, ctk::impl::ok{}) };

        const auto first{ begin(x) };
        const auto next{ std::copy(first, first + length, y) };
        *next = 0;
    }


    template<typename T>
    auto zero_guard(T* x, const char* func) -> void {
        if (x) {
            return;
        }

        std::ostringstream oss;
        oss << "[" << func << ", api_c] input pointer is zero";
        const auto e{ oss.str() };
        ctk_log_error(e);
        throw CtkLimit{ e };
    }

    template<typename T>
    auto array_guard(T* x, size_t size, const char* func) -> void {
        if (x && size != 0) {
            return;
        }

        std::ostringstream oss;
        oss << "[" << func << ", api_c] empty array";
        const auto e{ oss.str() };
        ctk_log_error(e);
        throw CtkLimit{ e };
    }

    auto index_guard(size_t i, size_t size, const char* func) -> void {
        if (i < size) {
            return;
        }

        std::ostringstream oss;
        oss << "[" << func << ", api_c] invalid index " << (i + 1) << "/" << size;
        const auto e{ oss.str() };
        ctk_log_error(e);
        throw CtkLimit{ e };
    }

    enum class rec { hospital,  physician, technician, s_id, s_name, s_addr, s_phone, s_sex, s_hand, s_dob, m_make, m_model, m_sn, t_name, t_serial, comment };


    class ReflibWriterNothrow
    {
        WriterReflib writer;

    public:

        ReflibWriterNothrow(const std::filesystem::path& cnt, RiffType riff)
        : writer{ cnt, riff } {
        }

        auto close() -> int {
            try {
                writer.Close();
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto electrode(const char* active, const char* ref) -> int {
            try {
                validate_writer_phase(writer.Phase(), WriterPhase::Setup, "WriterReflibNothrow::electrode short");

                std::string label;
                if (active) {
                    label = active;
                }

                std::string reference;
                if (ref) {
                    reference = ref;
                }

                const Electrode e{ label, reference };
                ctk::impl::validate(e);
                writer.ParamEeg.Electrodes.push_back(e);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto electrode(const char* active, const char* ref, const char* u, double iscale, double rscale) -> int {
            try {
                validate_writer_phase(writer.Phase(), WriterPhase::Setup, "WriterReflibNothrow::electrode long");

                std::string label;
                if (active) {
                    label = active;
                }

                std::string reference;
                if (ref) {
                    reference = ref;
                }

                std::string unit;
                if (u) {
                    unit = u;
                }

                const Electrode e{ label, reference, unit, iscale, rscale };
                ctk::impl::validate(e);
                writer.ParamEeg.Electrodes.push_back(e);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto sampling_frequency(double x) -> int {
            try {
                validate_writer_phase(writer.Phase(), WriterPhase::Setup, "WriterReflibNothrow::sampling_frequency");

                if (!std::isfinite(x) || x <= 0) { // TODO duplication
                    ctk_log_error("[WriterReflibNothrow::sampling_frequency, api_c] non positive");
                    return EXIT_FAILURE;
                }

                writer.ParamEeg.SamplingFrequency = x;
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto epoch_length(int64_t x) -> int {
            try {
                validate_writer_phase(writer.Phase(), WriterPhase::Setup, "WriterReflibNothrow::epoch_length");

                if (x < 1) { // TODO: duplication
                    ctk_log_error("[WriterReflibNothrow::epoch_length, api_c] non positive");
                    return EXIT_FAILURE;
                }
                writer.ParamEeg.EpochLength = x;
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto start_time(const struct timespec* x) -> int {
            try {
                validate_writer_phase(writer.Phase(), WriterPhase::Setup, "WriterReflibNothrow::start_time");
                zero_guard(x, "WriterReflibNothrow::start_time");

                writer.ParamEeg.StartTime = timespec2timepoint(*x);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto column_major(const double* matrix, size_t size) -> int {
            try {
                array_guard(matrix, size, "WriterReflibNothrow::column_major");

                std::vector<double> xs(size);
                std::copy(matrix, matrix + size, begin(xs));

                writer.cnt_ptr()->ColumnMajor(xs);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto column_major_int32(const int32_t* matrix, size_t size) -> int {
            try {
                array_guard(matrix, size, "WriterReflibNothrow::column_major_int32");

                std::vector<int32_t> xs(size);
                std::copy(matrix, matrix + size, begin(xs));

                writer.cnt_ptr()->ColumnMajorInt32(xs);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto row_major(const double* matrix, size_t size) -> int {
            try {
                array_guard(matrix, size, "WriterReflibNothrow::row_major");

                std::vector<double> xs(size);
                std::copy(matrix, matrix + size, begin(xs));

                writer.cnt_ptr()->RowMajor(xs);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto row_major_int32(const int32_t* matrix, size_t size) -> int {
            try {
                array_guard(matrix, size, "WriterReflibNothrow::row_major_int32");

                std::vector<int32_t> xs(size);
                std::copy(matrix, matrix + size, begin(xs));

                writer.cnt_ptr()->RowMajorInt32(xs);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto v4(const float* matrix, size_t size) -> int {
            try {
                array_guard(matrix, size, "WriterReflibNothrow::v4");

                std::vector<float> xs(size);
                std::copy(matrix, matrix + size, begin(xs));

                writer.cnt_ptr()->LibeepV4(xs);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto trigger(int64_t sample, const char* code) -> int {
            try {
                zero_guard(code, "ReflibWriterNothrow::trigger code");

                writer.cnt_ptr()->AddTrigger({ sample, std::string{ code } });
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto impedance(const struct timespec* t, const float* data, size_t size) -> int {
            try {
                zero_guard(t, "ReflibWriterNothrow::impedance stamp");
                const auto stamp{ timespec2timepoint(*t) };

                array_guard(data, size, "WriterReflibNothrow::impedance data");
                std::vector<float> impedances(size);
                std::copy(data, data + size, begin(impedances));

                writer.evt_ptr()->AddImpedance({ stamp, impedances });
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto video(const struct timespec* t, double duration, int32_t trigger_code) -> int {
            try {
                zero_guard(t, "ReflibWriterNothrow::video stamp");
                const auto stamp{ timespec2timepoint(*t) };

                writer.evt_ptr()->AddVideo({ stamp, duration, trigger_code });
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto epoch(const struct timespec* t, double duration, double offset, int32_t trigger_code) -> int {
            try {
                zero_guard(t, "ReflibWriterNothrow::epoch stamp");
                const auto stamp{ timespec2timepoint(*t) };

                writer.evt_ptr()->AddEpoch({ stamp, duration, offset, trigger_code });
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto subject(const char* id, const char* name, const char* address, const char* phone, char sex, char handedness, const struct timespec* date_of_birth) -> int {
            try {
                if (id) {
                    writer.RecordingInfo.SubjectId = id;
                }

                if (name) {
                    writer.RecordingInfo.SubjectName = name;
                }

                if (address) {
                    writer.RecordingInfo.SubjectAddress = address;
                }

                if (phone) {
                    writer.RecordingInfo.SubjectPhone = phone;
                }

                writer.RecordingInfo.SubjectSex = Char2Sex(static_cast<uint8_t>(sex));
                writer.RecordingInfo.SubjectHandedness = Char2Hand(static_cast<uint8_t>(handedness));

                if (date_of_birth) {
                    writer.RecordingInfo.SubjectDob = timespec2timepoint(*date_of_birth);
                }

                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto institution(const char* hospital, const char* physician, const char* technician) -> int {
            try {
                if (hospital) {
                    writer.RecordingInfo.Hospital = hospital;
                }

                if (physician) {
                    writer.RecordingInfo.Physician = physician;
                }

                if (technician) {
                    writer.RecordingInfo.Technician = technician;
                }

                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto equipment(const char* machine_make, const char* machine_model, const char* machine_sn) -> int {
            try {
                if (machine_make) {
                    writer.RecordingInfo.MachineMake = machine_make;
                }

                if (machine_model) {
                    writer.RecordingInfo.MachineModel = machine_model;
                }

                if (machine_sn) {
                    writer.RecordingInfo.MachineSn = machine_sn;
                }

                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto experiment(const char* test_name, const char* test_serial, const char* comment) -> int {
            try {
                if (test_name) {
                    writer.RecordingInfo.TestName = test_name;
                }

                if (test_serial) {
                    writer.RecordingInfo.TestSerial = test_serial;
                }

                if (comment) {
                    writer.RecordingInfo.Comment = comment;
                }

                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto info_str(const char* x, rec type) -> int {
            try {
                zero_guard(x, "ReflibWriterNothrow::info_str");

                switch(type) {
                    case rec::hospital: writer.RecordingInfo.Hospital = x; break;
                    case rec::physician: writer.RecordingInfo.Physician = x; break;
                    case rec::technician: writer.RecordingInfo.Technician = x; break;
                    case rec::s_id: writer.RecordingInfo.SubjectId = x; break;
                    case rec::s_name: writer.RecordingInfo.SubjectName = x; break;
                    case rec::s_addr: writer.RecordingInfo.SubjectAddress = x; break;
                    case rec::s_phone: writer.RecordingInfo.SubjectPhone = x; break;
                    case rec::m_make: writer.RecordingInfo.MachineMake = x; break;
                    case rec::m_model: writer.RecordingInfo.MachineModel = x; break;
                    case rec::m_sn: writer.RecordingInfo.MachineSn = x; break;
                    case rec::t_name: writer.RecordingInfo.TestName = x; break;
                    case rec::t_serial: writer.RecordingInfo.TestSerial = x; break;
                    case rec::comment: writer.RecordingInfo.Comment = x; break;
                    default: return EXIT_FAILURE;
                }

                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto info_sex(char x) -> int {
            try {
                writer.RecordingInfo.SubjectSex = Char2Sex(static_cast<uint8_t>(x));
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto info_hand(char x) -> int {
            try {
                writer.RecordingInfo.SubjectHandedness = Char2Hand(static_cast<uint8_t>(x));
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto info_dob(const struct timespec* x) -> int {
            try {
                zero_guard(x, "ReflibWriterNothrow::info_dob");
                writer.RecordingInfo.SubjectDob = timespec2timepoint(*x);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }
    };

    auto makeReflibWriterNothrow(const char* fname, int riff64) -> ReflibWriterNothrow* {
        try {
            if (!fname) {
                ctk_log_error("[makeReflibWriterNothrow, api_c] zero pointer");
                return nullptr;
            }

            const std::filesystem::path cnt{ fname };
            const RiffType riff{ riff64 ? RiffType::riff64 : RiffType::riff32 };

            return new ReflibWriterNothrow{ cnt, riff };
        }
        catch(const std::exception&) {
            return nullptr;
        }
    }



    enum class elc { label, reference, unit };


    struct ReflibReaderNothrow
    {
        ReaderReflib reader;

        ReflibReaderNothrow(const std::filesystem::path& cnt)
        : reader{ cnt } {
        }

        auto start_time() const -> struct timespec {
            try {
                return timepoint2timespec(reader.ParamEeg.StartTime);
            }
            catch(const std::exception&) {
                return { 0, 0 };
            }
        }


        template<typename T>
        auto check_input(int64_t index, int64_t amount, T* matrix, size_t size, const char* func) -> void {
            array_guard(matrix, size, func);

            if (index < 0) {
                std::ostringstream oss;
                oss << "[" << func << ", api_c] invalid index " << index;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw CtkLimit{ e };
            }

            if (amount < 1 || reader.ParamEeg.Electrodes.empty()) {
                std::ostringstream oss;
                oss << "[" << func << ", api_c] invalid matrix dimensions " << reader.ParamEeg.Electrodes.size() << "x" << amount;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw CtkLimit{ e };
            }
        }

        auto column_major(int64_t i, int64_t samples, double* matrix, size_t size) -> int64_t {
            try {
                check_input(i, samples, matrix, size, "ReflibReaderNothrow::column_major");

                std::vector<double> xs{ reader.RangeColumnMajor(i, samples) };
                if (size < xs.size()) {
                    std::ostringstream oss;
                    oss << "[ReflibReaderNothrow::column_major, api_c] requested " << size << ", available " << xs.size();
                    ctk_log_error(oss.str());
                    return 0;
                }

                std::copy(begin(xs), end(xs), matrix);
                return ctk::impl::cast(xs.size() / reader.ParamEeg.Electrodes.size(), int64_t{}, ctk::impl::ok{});
            }
            catch(const std::exception&) {
                return 0;
            }
        }

        auto row_major(int64_t i, int64_t samples, double* matrix, size_t size) -> int64_t {
            try {
                check_input(i, samples, matrix, size, "ReflibReaderNothrow::row_major");

                std::vector<double> xs{ reader.RangeRowMajor(i, samples) };
                if (size < xs.size()) {
                    std::ostringstream oss;
                    oss << "[ReflibReaderNothrow::row_major, api_c] requested " << size << ", available " << xs.size();
                    ctk_log_error(oss.str());
                    return 0;
                }

                std::copy(begin(xs), end(xs), matrix);
                return ctk::impl::cast(xs.size() / reader.ParamEeg.Electrodes.size(), int64_t{}, ctk::impl::ok{});
            }
            catch(const std::exception&) {
                return 0;
            }
        }

        auto v4(int64_t i, int64_t samples, float* matrix, size_t size) -> int64_t {
            try {
                check_input(i, samples, matrix, size, "ReflibReaderNothrow::v4");

                std::vector<float> xs{ reader.RangeV4(i, samples) };
                if (size < xs.size()) {
                    std::ostringstream oss;
                    oss << "[ReflibReaderNothrow::v4, api_c] requested " << size << ", available " << xs.size();
                    ctk_log_error(oss.str());
                    return 0;
                }

                std::copy(begin(xs), end(xs), matrix);
                return ctk::impl::cast(xs.size() / reader.ParamEeg.Electrodes.size(), int64_t{}, ctk::impl::ok{});
            }
            catch(const std::exception&) {
                return 0;
            }
        }

        auto column_major_int32(int64_t i, int64_t samples, int32_t* matrix, size_t size) -> int64_t {
            try {
                check_input(i, samples, matrix, size, "ReflibReaderNothrow::column_major_int32");

                std::vector<int32_t> xs{ reader.RangeColumnMajorInt32(i, samples) };
                if (size < xs.size()) {
                    std::ostringstream oss;
                    oss << "[ReflibReaderNothrow::column_major_int32, api_c] requested " << size << ", available " << xs.size();
                    ctk_log_error(oss.str());
                    return 0;
                }

                std::copy(begin(xs), end(xs), matrix);
                return ctk::impl::cast(xs.size() / reader.ParamEeg.Electrodes.size(), int64_t{}, ctk::impl::ok{});
            }
            catch(const std::exception&) {
                return 0;
            }
        }

        auto row_major_int32(int64_t i, int64_t samples, int32_t* matrix, size_t size) -> int64_t {
            try {
                check_input(i, samples, matrix, size, "ReflibReaderNothrow::row_major_int32");

                std::vector<int32_t> xs{ reader.RangeRowMajorInt32(i, samples) };
                if (size < xs.size()) {
                    std::ostringstream oss;
                    oss << "[ReflibReaderNothrow::row_major_int32, api_c] requested " << size << ", available " << xs.size();
                    ctk_log_error(oss.str());
                    return 0;
                }

                std::copy(begin(xs), end(xs), matrix);
                return ctk::impl::cast(xs.size() / reader.ParamEeg.Electrodes.size(), int64_t{}, ctk::impl::ok{});
            }
            catch(const std::exception&) {
                return 0;
            }
        }

        auto trigger(size_t i, int64_t* sample, char* code, size_t csize) const -> int {
            try {
                zero_guard(sample, "ReflibReaderNothrow::trigger sample");
                array_guard(code, csize, "ReflibReaderNothrow::trigger code");

                const std::vector<Trigger>& triggers{ reader.Triggers };
                index_guard(i, triggers.size(), "ReflibReaderNothrow::trigger");

                const Trigger& x{ triggers[i] };
                string2range(x.Code, code, csize);
                *sample = x.Sample;
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto impedance_size(size_t i) -> size_t {
            try {
                const std::vector<EventImpedance>& xs{ reader.Impedances };
                index_guard(i, xs.size(), "ReflibReaderNothrow::impedance_size");

                return xs[i].Values.size();
            }
            catch(const std::exception&) {
                return 0;
            }
        }

        auto impedance(size_t i, struct timespec* stamp, float* impedances, size_t isize) const -> int {
            try {
                zero_guard(stamp, "ReflibReaderNothrow::impedance stamp");
                array_guard(impedances, isize, "ReflibReaderNothrow::impedance impedances");

                const std::vector<EventImpedance>& xs{ reader.Impedances };
                index_guard(i, xs.size(), "ReflibReaderNothrow::impedance");

                const EventImpedance& x{ xs[i] };
                if (isize != x.Values.size()) {
                    std::ostringstream oss;
                    oss << "[ReflibReaderNothrow::impedance, api_c] requested " << isize << ", available " << x.Values.size();
                    ctk_log_error(oss.str());
                    return EXIT_FAILURE;
                }

                *stamp = timepoint2timespec(x.Stamp);
                std::copy(begin(x.Values), end(x.Values), impedances);
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto video(size_t i, struct timespec* stamp, double* duration, int32_t* trigger_code) const -> int {
            try {
                zero_guard(stamp, "ReflibReaderNothrow::video stamp");
                zero_guard(duration, "ReflibReaderNothrow::video duration");
                zero_guard(trigger_code, "ReflibReaderNothrow::video trigger_code");

                const std::vector<EventVideo>& xs{ reader.Videos };
                index_guard(i, xs.size(), "ReflibReaderNothrow::video");

                const EventVideo& x{ xs[i] };
                *stamp = timepoint2timespec(x.Stamp);
                *duration = x.Duration;
                *trigger_code = x.TriggerCode;
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto epoch(size_t i, struct timespec* stamp, double* duration, double* offset, int32_t* trigger_code) const -> int {
            try {
                zero_guard(stamp, "ReflibReaderNothrow::epoch stamp");
                zero_guard(duration, "ReflibReaderNothrow::epoch duration");
                zero_guard(offset, "ReflibReaderNothrow::epoch offset");
                zero_guard(trigger_code, "ReflibReaderNothrow::epoch trigger_code");

                const std::vector<EventEpoch>& xs{ reader.Epochs };
                index_guard(i, xs.size(), "ReflibReaderNothrow::epoch");

                const EventEpoch& x{ xs[i] };
                *stamp = timepoint2timespec(x.Stamp);
                *duration = x.Duration;
                *offset = x.Offset;
                *trigger_code = x.TriggerCode;
                return EXIT_SUCCESS;
            }
            catch(const std::exception&) {
                return EXIT_FAILURE;
            }
        }

        auto info(rec type) -> const char* {
            const auto& info{ reader.RecordingInfo };

            switch(type) {
                case rec::hospital: return info.Hospital.data();
                case rec::physician: return info.Physician.data();
                case rec::technician: return info.Technician.data();
                case rec::s_id: return info.SubjectId.data();
                case rec::s_name: return info.SubjectName.data();
                case rec::s_addr: return info.SubjectAddress.data();
                case rec::s_phone: return info.SubjectPhone.data();
                case rec::m_make: return info.MachineMake.data();
                case rec::m_model: return info.MachineModel.data();
                case rec::m_sn: return info.MachineSn.data();
                case rec::t_name: return info.TestName.data();
                case rec::t_serial: return info.TestSerial.data();
                case rec::comment: return info.Comment.data();
                default: return nullptr;
            }
        }

        auto info_sex() const -> char {
            const auto& info{ reader.RecordingInfo };
            return static_cast<char>(Sex2Char(info.SubjectSex));
        }

        auto info_hand() const -> char {
            const auto& info{ reader.RecordingInfo };
            return static_cast<char>(Hand2Char(info.SubjectHandedness));
        }

        auto info_dob() const -> struct timespec {
            try {
                const auto& info{ reader.RecordingInfo };
                return timepoint2timespec(info.SubjectDob);
            }
            catch(const std::exception&) {
                return { 0, 0 };
            }
        }

        auto electrode(size_t i, elc type) -> const char* {
            try {
                const std::vector<Electrode>& electrodes{ reader.ParamEeg.Electrodes };
                index_guard(i, electrodes.size(), "ReflibReaderNothrow::electrode");
                const Electrode& e{ electrodes[i] };

                switch(type) {
                    case elc::label: return e.ActiveLabel.data();
                    case elc::reference: return e.Reference.data();
                    case elc::unit: return e.Unit.data();
                    default: return nullptr;
                }
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        auto electrode_iscale(size_t i) -> double {
            try {
                const std::vector<Electrode>& electrodes{ reader.ParamEeg.Electrodes };
                index_guard(i, electrodes.size(), "ReflibReaderNothrow::electrode_iscale");

                return electrodes[i].IScale;
            }
            catch(const std::exception&) {
                return 0;
            }
        }

        auto electrode_rscale(size_t i) -> double {
            try {
                const std::vector<Electrode>& electrodes{ reader.ParamEeg.Electrodes };
                index_guard(i, electrodes.size(), "ReflibReaderNothrow::electrode_rscale");

                return electrodes[i].RScale;
            }
            catch(const std::exception&) {
                return 0;
            }
        }

    };

    auto makeReflibReaderNothrow(const char* cnt) -> ReflibReaderNothrow* {
        try {
            if (!cnt) {
                ctk_log_error("[makeReflibReaderNothrow, ctk] zero pointer");
                return nullptr;
            }

            return new ReflibReaderNothrow{ std::filesystem::path{ cnt } };
        }
        catch(const std::exception&) {
            return nullptr;
        }
    }

} // anonymous namespace


auto ctk_dcdate2timespec(double day_seconds, double subseconds, struct timespec* x) -> int {
    try {
        zero_guard(x, "ctk_dcdate2timespec output");

        *x = timepoint2timespec(ctk::api::v1::dcdate2timepoint({ day_seconds, subseconds }));
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_timespec2dcdate(const struct timespec* x, double* day_seconds, double* subseconds) -> int {
    try {
        zero_guard(x, "ctk_timespec2dcdate input");
        zero_guard(day_seconds, "ctk_timespec2dcdate output date");
        zero_guard(subseconds, "ctk_timespec2dcdate output fraction");

        const auto y{ ctk::api::v1::timepoint2dcdate(timespec2timepoint(*x)) };
        *day_seconds = y.Date;
        *subseconds = y.Fraction;
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_tm2timespec(const struct tm* x, struct timespec* y) -> int {
    try {
        zero_guard(x, "ctk_tm2timespec input");
        zero_guard(y, "ctk_tm2timespec output");

        *y = timepoint2timespec(ctk::impl::tm2timepoint(*x));
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_timespec2tm(const struct timespec* x, struct tm* y) -> int {
    try {
        zero_guard(x, "ctk_timespec2tm input");
        zero_guard(y, "ctk_timespec2tm output");

        *y = ctk::impl::timepoint2tm(timespec2timepoint(*x));
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}




auto ctk_reflib_writer_make(const char* file_name, int riff64) -> ctk_reflib_writer* {
    try {
        return reinterpret_cast<ctk_reflib_writer*>(makeReflibWriterNothrow(file_name, riff64));
    }
    catch(const std::exception&) {
        return nullptr;
    }
}

auto ctk_reflib_writer_dispose(ctk_reflib_writer* p) -> void {
    delete reinterpret_cast<ReflibWriterNothrow*>(p);
}

auto ctk_reflib_writer_close(ctk_reflib_writer* p) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->close();
}

auto ctk_reflib_writer_electrode_uv(ctk_reflib_writer* p, const char* active, const char* ref) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->electrode(active, ref);
}

auto ctk_reflib_writer_electrode(ctk_reflib_writer* p, const char* active, const char* ref, const char* unit, double iscale, double rscale) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->electrode(active, ref, unit, iscale, rscale);
}

auto ctk_reflib_writer_sampling_frequency(ctk_reflib_writer* p, double x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->sampling_frequency(x);
}

auto ctk_reflib_writer_epoch_length(ctk_reflib_writer* p, int64_t x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->epoch_length(x);
}

auto ctk_reflib_writer_start_time(ctk_reflib_writer* p, const struct timespec* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->start_time(x);
}

auto ctk_reflib_writer_column_major(ctk_reflib_writer* p, const double* matrix, size_t size) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->column_major(matrix, size);
}

auto ctk_reflib_writer_column_major_int32(ctk_reflib_writer* p, const int32_t* matrix, size_t size) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->column_major_int32(matrix, size);
}

auto ctk_reflib_writer_row_major(ctk_reflib_writer* p, const double* matrix, size_t size) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->row_major(matrix, size);
}

auto ctk_reflib_writer_row_major_int32(ctk_reflib_writer* p, const int32_t* matrix, size_t size) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->row_major_int32(matrix, size);
}

auto ctk_reflib_writer_v4(ctk_reflib_writer* p, const float* matrix, size_t size) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->v4(matrix, size);
}


auto ctk_reflib_writer_trigger(ctk_reflib_writer* p, int64_t sample, const char* code) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->trigger(sample, code);
}

auto ctk_reflib_writer_impedance(ctk_reflib_writer* p, const struct timespec* t, const float* data, size_t size) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->impedance(t, data, size);
}

auto ctk_reflib_writer_video(ctk_reflib_writer* p, const struct timespec* t, double duration, int32_t trigger_code) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->video(t, duration, trigger_code);
}

auto ctk_reflib_writer_epoch(ctk_reflib_writer* p, const struct timespec* t, double duration, double offset, int32_t trigger_code) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->epoch(t, duration, offset, trigger_code);
}

auto ctk_reflib_writer_subject(ctk_reflib_writer* p, const char* id, const char* name, const char* address, const char* phone, char sex, char handedness, const struct timespec* date_of_birth) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->subject(id, name, address, phone, sex, handedness, date_of_birth);
}

auto ctk_reflib_writer_institution(ctk_reflib_writer* p, const char* hospital, const char* physician, const char* technician) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->institution(hospital, physician, technician);
}

auto ctk_reflib_writer_equipment(ctk_reflib_writer* p, const char* machine_make, const char* machine_model, const char* machine_sn) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->equipment(machine_make, machine_model, machine_sn);
}

auto ctk_reflib_writer_experiment(ctk_reflib_writer* p, const char* test_name, const char* test_serial, const char* comment) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->experiment(test_name, test_serial, comment);
}

auto ctk_reflib_writer_hospital(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::hospital);
}

auto ctk_reflib_writer_physician(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::physician);
}

auto ctk_reflib_writer_technician(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::technician);
}

auto ctk_reflib_writer_subject_id(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::s_id);
}

auto ctk_reflib_writer_subject_name(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::s_name);
}

auto ctk_reflib_writer_subject_address(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::s_addr);
}

auto ctk_reflib_writer_subject_phone(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::s_phone);
}

auto ctk_reflib_writer_subject_sex(ctk_reflib_writer* p, char x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_sex(x);
}

auto ctk_reflib_writer_subject_handedness(ctk_reflib_writer* p, char x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_hand(x);
}

auto ctk_reflib_writer_subject_dob(ctk_reflib_writer* p, const struct timespec* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_dob(x);
}

auto ctk_reflib_writer_machine_make(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::m_make);
}

auto ctk_reflib_writer_machine_model(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::m_model);
}

auto ctk_reflib_writer_machine_sn(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::m_sn);
}

auto ctk_reflib_writer_test_name(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::t_name);
}

auto ctk_reflib_writer_test_serial(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::t_serial);
}

auto ctk_reflib_writer_comment(ctk_reflib_writer* p, const char* x) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibWriterNothrow*>(p)->info_str(x, rec::comment);
}





auto ctk_reflib_reader_make(const char* file_name) -> ctk_reflib_reader* {
    try {
        return reinterpret_cast<ctk_reflib_reader*>(makeReflibReaderNothrow(file_name));
    }
    catch(const std::exception&) {
        return nullptr;
    }
}

auto ctk_reflib_reader_dispose(ctk_reflib_reader* p) -> void {
    delete reinterpret_cast<ReflibReaderNothrow*>(p);
}

auto ctk_reflib_reader_electrode_count(ctk_reflib_reader* p) -> size_t {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.ParamEeg.Electrodes.size();
}

auto ctk_reflib_reader_electrode_label(ctk_reflib_reader* p, size_t i) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->electrode(i, elc::label);
}

auto ctk_reflib_reader_electrode_reference(ctk_reflib_reader* p, size_t i) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->electrode(i, elc::reference);
}

auto ctk_reflib_reader_electrode_unit(ctk_reflib_reader* p, size_t i) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->electrode(i, elc::unit);
}

auto ctk_reflib_reader_electrode_iscale(ctk_reflib_reader* p, size_t i) -> double {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->electrode_iscale(i);
}

auto ctk_reflib_reader_electrode_rscale(ctk_reflib_reader* p, size_t i) -> double {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->electrode_rscale(i);
}


auto ctk_reflib_reader_start_time(ctk_reflib_reader* p) -> struct timespec {
    if (!p) { return { 0, 0 }; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->start_time();
}

auto ctk_reflib_reader_sampling_frequency(ctk_reflib_reader* p) -> double {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.ParamEeg.SamplingFrequency;
}

auto ctk_reflib_reader_epoch_length(ctk_reflib_reader* p) -> int64_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.ParamEeg.EpochLength;
}

auto ctk_reflib_reader_sample_count(ctk_reflib_reader* p) -> int64_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.SampleCount;
}

auto ctk_reflib_reader_column_major(ctk_reflib_reader* p, int64_t i, int64_t samples, double* matrix, size_t size) -> int64_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->column_major(i, samples, matrix, size);
}

auto ctk_reflib_reader_row_major(ctk_reflib_reader* p, int64_t i, int64_t samples, double* matrix, size_t size) -> int64_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->row_major(i, samples, matrix, size);
}

auto ctk_reflib_reader_v4(ctk_reflib_reader* p, int64_t i, int64_t samples, float* matrix, size_t size) -> int64_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->v4(i, samples, matrix, size);
}

auto ctk_reflib_reader_column_major_int32(ctk_reflib_reader* p, int64_t i, int64_t samples, int32_t* matrix, size_t size) -> int64_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->column_major_int32(i, samples, matrix, size);
}

auto ctk_reflib_reader_row_major_int32(ctk_reflib_reader* p, int64_t i, int64_t samples, int32_t* matrix, size_t size) -> int64_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->row_major_int32(i, samples, matrix, size);
}

auto ctk_reflib_reader_trigger_count(ctk_reflib_reader* p) -> size_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.Triggers.size();
}

auto ctk_reflib_reader_trigger(ctk_reflib_reader* p, size_t i, int64_t* sample, char* code, size_t csize) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->trigger(i, sample, code, csize);
}

auto ctk_reflib_reader_impedance_count(ctk_reflib_reader* p) -> size_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.Impedances.size();
}

auto ctk_reflib_reader_impedance_size(ctk_reflib_reader* p, size_t i) -> size_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->impedance_size(i);
}

auto ctk_reflib_reader_impedance(ctk_reflib_reader* p, size_t i, struct timespec* stamp, float* impedances, size_t isize) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->impedance(i, stamp, impedances, isize);
}

auto ctk_reflib_reader_video_count(ctk_reflib_reader* p) -> size_t {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.Videos.size();
}

auto ctk_reflib_reader_video(ctk_reflib_reader* p, size_t i, struct timespec* stamp, double* duration, int32_t* trigger_code) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->video(i, stamp, duration, trigger_code);
}

auto ctk_reflib_reader_epoch_count(ctk_reflib_reader* p) -> size_t {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->reader.Epochs.size();
}

auto ctk_reflib_reader_epoch(ctk_reflib_reader* p, size_t i, struct timespec* stamp, double* duration, double* offset, int32_t* trigger_code) -> int {
    if (!p) { return EXIT_FAILURE; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->epoch(i, stamp, duration, offset, trigger_code);
}

auto ctk_reflib_reader_hospital(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::hospital);
}

auto ctk_reflib_reader_physician(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::physician);
}

auto ctk_reflib_reader_technician(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::technician);
}

auto ctk_reflib_reader_subject_id(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::s_id);
}

auto ctk_reflib_reader_subject_name(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::s_name);
}

auto ctk_reflib_reader_subject_address(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::s_addr);
}

auto ctk_reflib_reader_subject_phone(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::s_phone);
}

auto ctk_reflib_reader_subject_sex(ctk_reflib_reader* p) -> char {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info_sex();
}

auto ctk_reflib_reader_subject_handedness(ctk_reflib_reader* p) -> char {
    if (!p) { return 0; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info_hand();
}

auto ctk_reflib_reader_subject_dob(ctk_reflib_reader* p) -> struct timespec {
    if (!p) { return { 0, 0 }; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info_dob();
}

auto ctk_reflib_reader_machine_make(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::m_make);
}

auto ctk_reflib_reader_machine_model(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::m_model);
}

auto ctk_reflib_reader_machine_sn(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::m_sn);
}

auto ctk_reflib_reader_test_name(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::t_name);
}

auto ctk_reflib_reader_test_serial(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::t_serial);
}

auto ctk_reflib_reader_comment(ctk_reflib_reader* p) -> const char* {
    if (!p) { return nullptr; }
    return reinterpret_cast<ReflibReaderNothrow*>(p)->info(rec::comment);
}


auto ctk_set_logger(const char* log_type, const char* log_level) -> int {
    try {
        ctk::impl::set_logger(log_type, log_level);
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_log_trace(const char* msg) -> int {
    try {
        ctk::impl::ctk_log_trace(msg);
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_log_debug(const char* msg) -> int {
    try {
        ctk::impl::ctk_log_debug(msg);
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_log_info(const char* msg) -> int {
    try {
        ctk::impl::ctk_log_info(msg);
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_log_warning(const char* msg) -> int {
    try {
        ctk::impl::ctk_log_warning(msg);
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_log_error(const char* msg) -> int {
    try {
        ctk::impl::ctk_log_error(msg);
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

auto ctk_log_critical(const char* msg) -> int {
    try {
        ctk::impl::ctk_log_critical(msg);
        return EXIT_SUCCESS;
    }
    catch(const std::exception&) {
        return EXIT_FAILURE;
    }
}

