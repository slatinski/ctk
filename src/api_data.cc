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

#include <cmath>
#include <iomanip>
#include <algorithm>
#include "api_data.h"
#include "exception.h"
#include "arithmetic.h"
#include "ctk_version.h"
#include "container/file_epoch.h"
#include "date/date.h"

namespace ctk { namespace api {

    namespace v1 {

        Trigger::Trigger()
        : sample{ std::numeric_limits<int64_t>::max() } {
            std::fill(begin(code), end(code), 0);
        }

        Trigger::Trigger(int64_t s, const std::string& c)
        : sample{ s }
        , code{ ctk::impl::as_code(c) } {
        }

        Trigger::Trigger(int64_t s, const std::array<char, evt_label_size>& c)
        : sample{ s } {
            static_assert(sizeof(c) < sizeof(code));
            auto next{ std::copy(begin(c), end(c), begin(code)) };
            *next = 0;
        }

        auto operator==(const Trigger& x, const Trigger& y) -> bool {
            return x.sample == y.sample && ctk::impl::as_string(x.code) == ctk::impl::as_string(y.code);
        }

        auto operator!=(const Trigger& x, const Trigger& y) -> bool {
            return !(x == y);
        }

        auto operator<<(std::ostream& os, const Trigger& x) -> std::ostream& {
            os << "sample " << x.sample << ", code " << ctk::impl::as_string(x.code);
            return os;
        }

        auto trigger_label(const std::array<char, evt_label_size + 2>& x) -> std::string {
            return ctk::impl::as_string(x);
        }

        DcDate::DcDate()
        : date{ 0 }
        , fraction{ 0 } {
        }

        DcDate::DcDate(double date, double fraction)
        : date{ date }
        , fraction{ fraction } {
        }

        auto operator<<(std::ostream& os, const DcDate& x) -> std::ostream& {
            return print(os, x);
        }

        auto operator<<(std::ostream& os, Sex x) -> std::ostream& {
            os << ctk::impl::sex2ch(x);
            return os;
        }

        auto sex2char(Sex x) -> uint8_t {
            return ctk::impl::sex2ch(x);
        }

        auto char2sex(uint8_t x) -> Sex {
            return ctk::impl::ch2sex(x);
        }


        auto operator<<(std::ostream& os, Handedness x) -> std::ostream& {
            os << ctk::impl::hand2ch(x);
            return os;
        }

        auto handedness2char(Handedness x) -> uint8_t {
            return ctk::impl::hand2ch(x);
        }

        auto char2handedness(uint8_t x) -> Handedness {
            return ctk::impl::ch2hand(x);
        }


        Info::Info()
        : subject_sex{ Sex::unknown }
        , subject_handedness{ Handedness::unknown }
        , subject_dob{ ctk::impl::make_dob() } {
        }

        auto operator==(const Info& x, const Info& y) -> bool {
            return x.hospital == y.hospital
                && x.test_name == y.test_name
                && x.test_serial == y.test_serial
                && x.physician == y.physician
                && x.technician == y.technician
                && x.machine_make == y.machine_make
                && x.machine_model == y.machine_model
                && x.machine_sn == y.machine_sn
                && x.subject_name == y.subject_name
                && x.subject_id == y.subject_id
                && x.subject_address == y.subject_address
                && x.subject_phone == y.subject_phone
                && x.subject_sex == y.subject_sex
                && x.subject_handedness == y.subject_handedness
                && ctk::impl::is_equal(x.subject_dob, y.subject_dob)
                && x.comment == y.comment;
        }

        auto operator!=(const Info& x, const Info& y) -> bool {
            return !(x == y);
        }

        auto operator<<(std::ostream& os, const Info& x) -> std::ostream& {
            if (!x.hospital.empty()) { os << "hospital " << x.hospital; os << " "; }
            if (!x.test_name.empty()) { os << "test name " << x.test_name; os << " "; }
            if (!x.test_serial.empty()) { os << "test serial " << x.test_serial; os << " "; }
            if (!x.physician.empty()) { os << "physician " << x.physician; os << " "; }
            if (!x.technician.empty()) { os << "technician " << x.technician; os << " "; }
            if (!x.machine_make.empty()) { os << "machine make " << x.machine_make; os << " "; }
            if (!x.machine_model.empty()) { os << "machine model " << x.machine_model; os << " "; }
            if (!x.machine_sn.empty()) { os << "machine sn " << x.machine_sn; os << " "; }
            if (!x.subject_name.empty()) { os << "subject name " << x.subject_name; os << " "; }
            if (!x.subject_id.empty()) { os << "subject id " << x.subject_id; os << " "; }
            if (!x.subject_address.empty()) { os << "subject address " << x.subject_address; os << " "; }
            if (!x.subject_phone.empty()) { os << "subject phone " << x.subject_phone; os << " "; }
            if (x.subject_sex != Sex::unknown) { os << "subject sex " << x.subject_sex; os << " "; }
            if (x.subject_handedness != Handedness ::unknown) { os << "subject handedness " << x.subject_handedness; os << " "; }
            if (x.subject_dob.tm_sec != 0 || x.subject_dob.tm_min != 0 || x.subject_dob.tm_hour != 0 ||
                x.subject_dob.tm_mday != 0 || x.subject_dob.tm_mon != 0 || x.subject_dob.tm_year != 0 ||
                x.subject_dob.tm_yday != 0 || x.subject_dob.tm_isdst != 0) {
                os << std::asctime(&x.subject_dob);
            }
            if (!x.comment.empty()) { os << "comment [" << x.comment; os << "] "; }
            return os;
        }


        Electrode::Electrode(const std::string& label, const std::string& reference, const std::string& unit, double iscale, double rscale)
        : label{ label }
        , reference{ reference }
        , unit{ unit }
        , iscale{ iscale }
        , rscale{ rscale } {
        }

        Electrode::Electrode()
        : iscale{ 1 } // multiplicative identity
        , rscale{ 1 } {
        }

        auto operator<<(std::ostream& os, const Electrode& x) -> std::ostream& {
            os << x.label << "-" << x.reference
               << " [" << x.unit << "]"
               << ", iscale " << ctk::impl::d2s(x.iscale, 13)
               << ", rscale " << ctk::impl::d2s(x.rscale, 13)
               << ", type " << x.type
               << ", status " << x.status;
            return os;
        }


        TimeSeries::TimeSeries(std::chrono::system_clock::time_point t, double f, const std::vector<v1::Electrode>& e, int64_t l)
        : start_time{ t }
        , sampling_frequency{ f }
        , electrodes{ e }
            , epoch_length{ l } {
        }

        TimeSeries::TimeSeries(const DcDate& t, double f, const std::vector<v1::Electrode>& e, int64_t l)
        : start_time{ dcdate2timepoint(t) }
        , sampling_frequency{ f }
        , electrodes{ e }
        , epoch_length{ l } {
        }

        TimeSeries::TimeSeries()
        : sampling_frequency{ 0 }
        , epoch_length{ 256 } {
        }

        auto operator==(const TimeSeries& x, const TimeSeries& y) -> bool {
            return x.start_time == y.start_time
                && std::fabs(x.sampling_frequency - y.sampling_frequency) < 1e-6
                && x.electrodes == y.electrodes
                && x.epoch_length == y.epoch_length;
        }

        auto operator!=(const TimeSeries& x, const TimeSeries& y) -> bool {
            return !(x == y);
        }

        auto operator<<(std::ostream& os, const TimeSeries& x) -> std::ostream& {
            os << "epoch length " << x.epoch_length << ", sampling frequency " << x.sampling_frequency;
            os << ", start time "; print(os, x.start_time); os << "\n";
            for (const auto& e : x.electrodes) {
                os << e << "\n";
            }
            return os;
        }

        auto operator<<(std::ostream& os, RiffType x) -> std::ostream& {
            switch(x) {
            case RiffType::riff32:
                os << ctk::impl::root_id_riff32();
                break;
            case RiffType::riff64:
                os << ctk::impl::root_id_riff64();
                break;
            default: throw ctk_bug{ "operator<<(RiffType): invalid" };
            }
            return os;
        }

        FileVersion::FileVersion()
        : major{ 0 }
        , minor{ 0 } {
        }

        FileVersion::FileVersion(uint32_t major, uint32_t minor)
        : major{ major }
        , minor{ minor } {
        }

        auto operator<<(std::ostream& os, const FileVersion& x) -> std::ostream& {
            os << x.major << "." << x.minor;
            return os;
        }

        UserFile::UserFile(const std::string& label, const std::string& file_name)
        : label{ label }
        , file_name{ file_name } {
        }

        auto operator<<(std::ostream& os, const UserFile& x) -> std::ostream& {
            os << x.label << "." << x.file_name;
            return os;
        }

        Version::Version()
        : major{ CTK_MAJOR }
        , minor{ CTK_MINOR }
        , patch{ CTK_PATCH }
        , build{ CTK_BUILD } {
        }

        auto operator<<(std::ostream& os, const Version& x) -> std::ostream& {
            os << x.major << "." << x.minor << x.patch << "." << x.build;
            return os;
        }


        EventImpedance::EventImpedance()
        : stamp{ dcdate2timepoint({ 0, 0 }) } {
        }

        EventImpedance::EventImpedance(std::chrono::system_clock::time_point stamp, const std::vector<float>& values)
        : stamp{ stamp }
        , values{ values } {
        }

        EventImpedance::EventImpedance(const DcDate& time, const std::vector<float>& values)
        : stamp{ dcdate2timepoint(time) }
        , values{ values } {
        }



        EventVideo::EventVideo()
        : stamp{ dcdate2timepoint({ 0, 0 }) }
        , duration{ 0 }
        , trigger_code{ std::numeric_limits<int32_t>::min() } {
        }

        EventEpoch::EventEpoch()
        : stamp{ dcdate2timepoint({ 0, 0 }) }
        , duration{ 0 }
        , offset{ 0 },
        trigger_code{ std::numeric_limits<int32_t>::min() } {
        }

    } /* namespace v1 */


    constexpr const std::chrono::system_clock::time_point excel_epoch{ date::sys_days{ date::year{ 1899 }/12/30 } };
    constexpr const double seconds_per_day{ 60 * 60 * 24 };


    static
    auto regular(const v1::DcDate& x) -> v1::DcDate {
        const double whole{ x.date * seconds_per_day };
        const double additional{ 0 <= x.fraction ? std::floor(x.fraction) : std::ceil(x.fraction) };
        const double seconds{ whole + additional };
        const double subseconds{ x.fraction - additional };
        const double days_since_epoch{ seconds / seconds_per_day };
        if (!std::isfinite(days_since_epoch) || !std::isfinite(subseconds)) {
            throw ctk::api::v1::ctk_limit{ "regular: infinite dcdate" };
        }
        assert(std::fabs(subseconds) < 1);

        return { days_since_epoch, subseconds };
    }


    template<typename N, typename Precision>
    auto to_timespan(v1::DcDate x, N factor, Precision) -> Precision {
        using T = typename Precision::rep;

        x = regular(x);
        const double scaled_base{ std::round(x.date * seconds_per_day) };
        if (!std::isfinite(scaled_base)) {
            throw ctk::api::v1::ctk_limit{ "to_timespan: infinite seconds" };
        }

        const double scaled_fraction{ std::round(x.fraction * static_cast<double>(factor)) };
        if (!std::isfinite(scaled_fraction)) {
            throw ctk::api::v1::ctk_limit{ "to_timespan: infinite fraction" };
        }

        const T base_sec{ static_cast<T>(scaled_base) };
        const T base_subsec{ ctk::impl::multiply(base_sec, factor, ctk::impl::ok{}) };
        const T fraction_subsec{ static_cast<T>(scaled_fraction) };
        const T result{ ctk::impl::plus(base_subsec, fraction_subsec, ctk::impl::ok{}) };
        return Precision{ result };
    }


    auto dcdate2timespan(const v1::DcDate& x) -> std::chrono::nanoseconds {
        return to_timespan(x, std::nano::den, std::chrono::nanoseconds{});
    }


    auto timespan2dcdate(std::chrono::nanoseconds x) -> v1::DcDate {
        using namespace std::chrono;

        const seconds s{ floor<seconds>(x) };
        const nanoseconds subsecond{ x - s };
        assert(subsecond < 1s);
        const double days_since_epoch{ static_cast<double>(s.count()) / seconds_per_day };
        const double fraction{ static_cast<double>(subsecond.count()) / std::nano::den };

        return { days_since_epoch, fraction };
    }


    auto dcdate2timepoint(const v1::DcDate& x) -> std::chrono::system_clock::time_point {
        using namespace std::chrono;
#ifdef _WIN32
        using Precision = microseconds;
        constexpr const auto factor{ std::micro::den };
#else
        using Precision = nanoseconds;
        constexpr const auto factor{ std::nano::den };
#endif
        using T = Precision::rep;

        constexpr const T epoch_subsec{ duration_cast<seconds>(excel_epoch.time_since_epoch()).count() * factor };
        const T span_subsec{ to_timespan(x, factor, Precision{}).count() };
        const Precision result{ ctk::impl::plus(epoch_subsec, span_subsec, ctk::impl::ok{}) };

        return system_clock::time_point{ result };
    }


    auto timepoint2dcdate(std::chrono::system_clock::time_point x) -> v1::DcDate {
        using namespace std::chrono;
        using T = nanoseconds::rep;

        constexpr const T epoch_ns{ duration_cast<nanoseconds>(excel_epoch.time_since_epoch()).count() };
        const T x_ns{ duration_cast<nanoseconds>(x.time_since_epoch()).count() };
        const nanoseconds span{ ctk::impl::minus(x_ns, epoch_ns, ctk::impl::ok{}) };

        return timespan2dcdate(span);
    }


    auto print_duration(std::ostream& oss, std::chrono::nanoseconds x, std::chrono::nanoseconds /* type tag */) -> std::ostream& {
        oss << std::setfill('0') << std::setw(12) << x.count();
        return oss;
    }


    auto print_duration(std::ostream& oss, std::chrono::nanoseconds x, std::chrono::microseconds /* type tag */) -> std::ostream& {
        oss << std::setfill('0') << std::setw(9) << std::chrono::duration_cast<std::chrono::microseconds>(x).count();
        return oss;
    }


    auto print(std::ostream& oss, std::chrono::system_clock::time_point x) -> std::ostream& {
        using namespace std::chrono;

        const nanoseconds x_us{ duration_cast<nanoseconds>(x.time_since_epoch()) };
        const days x_days{ floor<days>(x_us) };
        const date::year_month_day ymd{ date::sys_days{ x_days } };
        oss << ymd << " ";

        nanoseconds reminder{ x_us - x_days };
        const hours h{ floor<hours>(reminder) };
        reminder -= h;
        oss << std::setfill('0') << std::setw(2) << h.count() << ":";

        const minutes m{ floor<minutes>(reminder) };
        reminder -= m;
        oss << std::setfill('0') << std::setw(2) << m.count() << ":";

        const seconds s{ floor<seconds>(reminder) };
        reminder -= s;
        oss << std::setfill('0') << std::setw(2) << s.count() << ":";

        assert(reminder < 1s);
        return print_duration(oss, reminder, system_clock::time_point::duration{});
    }


    auto print(std::ostream& oss, const v1::DcDate& x) -> std::ostream& {
        return print(oss, dcdate2timepoint(x));
    }


} /* namespace api */ } /* namespace ctk */

