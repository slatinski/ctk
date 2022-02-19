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
#include "file/cnt_epoch.h"
#include "date/date.h"

namespace ctk { namespace api {

    namespace v1 {

        Trigger::Trigger()
        : sample{ std::numeric_limits<int64_t>::max() } {
            std::fill(begin(code), end(code), char{ 0 });
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
            os << x.sample << " " << ctk::impl::as_string(x.code);
            return os;
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
        , subject_dob{ ctk::impl::tm2timepoint(ctk::impl::make_tm()) } {
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
            if (x.subject_dob != ctk::impl::tm2timepoint(ctk::impl::make_tm())) { os << "subject date of birth "; print(os, x.subject_dob); os << " "; }
            if (!x.comment.empty()) { os << "comment [" << x.comment; os << "] "; }
            return os;
        }


        Electrode::Electrode()
        : iscale{ 1 }
        , rscale{ 1 / default_scaling_factor() } {
        }

        Electrode::Electrode(const std::string& l, const std::string& r, const std::string& u)
        : label{ l }
        , reference{ r }
        , unit{ u }
        , iscale{ 1 }
        , rscale{ 1 / default_scaling_factor() } {
            // compatibility: uV is stored in some files as µV so that the first character is not valid utf8
            if (!unit.empty() && unit[0] == -75) {
                // TODO: log
                unit[0] = 'u';
            }
        }

        Electrode::Electrode(const std::string& l, const std::string& r, const std::string& u, double iscale, double rscale)
        : label{ l }
        , reference{ r }
        , unit{ u }
        , iscale{ iscale }
        , rscale{ rscale } {
            // compatibility: uV is stored in some files as µV so that the first character is not valid utf8
            if (!unit.empty() && unit[0] == -75) {
                // TODO: log
                unit[0] = 'u';
            }
        }

        auto operator<<(std::ostream& os, const Electrode& x) -> std::ostream& {
            os << x.label << "-" << x.reference
               << " " << x.unit
               << " " << ctk::impl::d2s(x.iscale, 13)
               << " " << ctk::impl::d2s(x.rscale, 13);

            if (!x.type.empty()) {
               os << ", " << x.type;
            }

            if (!x.status.empty()) {
               os << ", " << x.status;
            }
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
        : start_time{ dcdate2timepoint({ 0, 0 }) }
        , sampling_frequency{ 0 }
        , epoch_length{ 1024 } {
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
            os << x.label << ": " << x.file_name;
            return os;
        }

        Version::Version()
        : major{ CTK_MAJOR }
        , minor{ CTK_MINOR }
        , patch{ CTK_PATCH }
        , build{ CTK_BUILD } {
        }

        auto operator<<(std::ostream& os, const Version& x) -> std::ostream& {
            os << x.major << "." << x.minor << "." << x.patch << "." << x.build;
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

        EventVideo::EventVideo(std::chrono::system_clock::time_point stamp, double duration, int32_t code)
        : stamp{ stamp }
        , duration{ duration }
        , trigger_code{ code } {
        }

        EventVideo::EventVideo(const DcDate& time, double duration, int32_t code)
        : stamp{ dcdate2timepoint(time) }
        , duration{ duration }
        , trigger_code{ code } {
        }



        EventEpoch::EventEpoch()
        : stamp{ dcdate2timepoint({ 0, 0 }) }
        , duration{ 0 }
        , offset{ 0 }
        , trigger_code{ std::numeric_limits<int32_t>::min() } {
        }

        EventEpoch::EventEpoch(std::chrono::system_clock::time_point stamp, double duration, double offset, int32_t code)
        : stamp{ stamp }
        , duration{ duration }
        , offset{ offset }
        , trigger_code{ code } {
        }

        EventEpoch::EventEpoch(const DcDate& time, double duration, double offset, int32_t code)
        : stamp{ dcdate2timepoint(time) }
        , duration{ duration }
        , offset{ offset }
        , trigger_code{ code } {
        }

    } /* namespace v1 */


    constexpr const date::year_month_day dcdate_epoch{ date::December/30/1899 };
    constexpr const double seconds_per_day{ 60 * 60 * 24 };


    static
    auto days2seconds(double x) -> double {
        return x * seconds_per_day;
    }

    static
    auto seconds2days(double x) -> double {
        return x / seconds_per_day;
    }

    static
    auto to_sys_clock(double x) -> double {
        return x * std::chrono::system_clock::period::den;
    }

    static
    auto from_sys_clock(double x) -> double {
        return x / std::chrono::system_clock::period::den;
    }


    static
    auto regular(const v1::DcDate& x) -> v1::DcDate {
        if (x.fraction < 0) {
            throw ctk::api::v1::ctk_limit{ "regular: negative sub seconds" };
        }
        const double i_fraction{ floor(x.fraction) };

        const double seconds{ days2seconds(x.date) + i_fraction };
        const double subseconds{ x.fraction - i_fraction };
        const double days{ seconds2days(seconds) };
        if (!std::isfinite(days) || !std::isfinite(subseconds)) {
            throw ctk::api::v1::ctk_limit{ "regular: infinite dcdate" };
        }
        assert(std::fabs(subseconds) < 1);

        return { days, subseconds };
    }


    // performs the computation and comparison on doubles in order to avoid signed integer overflow.
    // because of that the precision around the extremes suffers.
    static
    auto clock_guard(const ctk::api::v1::DcDate& x) -> bool {
        using namespace std::chrono;
        using dsec = duration<double, std::ratio<1>>;
        using dsys = duration<double, system_clock::period>;
        using sys_tp = time_point<system_clock, dsys>;
        using T = system_clock::rep;

        constexpr const sys_tp epoch{ date::sys_days{ dcdate_epoch } };
        constexpr const sys_tp tmin{ dsys{ static_cast<double>(std::numeric_limits<T>::min()) } };
        constexpr const sys_tp tmax{ dsys{ static_cast<double>(std::numeric_limits<T>::max()) } };
        constexpr const dsys min_slack{ 1h };
        constexpr const dsys max_slack{ date::years{ 70 } + date::days{ 10 } };

        const dsec seconds{ days2seconds(x.date) };
        const dsys subseconds{ to_sys_clock(x.fraction) };
        const sys_tp y{ epoch + seconds + subseconds };
        return ((tmin + min_slack) < y) && (y < (tmax - max_slack));
    }



    auto dcdate2timepoint(v1::DcDate x) -> std::chrono::system_clock::time_point {
        using namespace std::chrono;
        using xseconds = system_clock::duration;

        x = regular(x);
        if (!clock_guard(x)) {
            throw v1::ctk_limit{ "dcdate2timepoint: out of clock range" };
        }

        const double sec{ std::round(days2seconds(x.date)) };
        const double subsec{ std::round(to_sys_clock(x.fraction)) };
        const seconds whole{ static_cast<seconds::rep>(sec) };
        const xseconds fraction{ static_cast<xseconds::rep>(subsec) };

        constexpr const system_clock::time_point epoch{ date::sys_days{ dcdate_epoch } };
        return epoch + whole + fraction;
    }


    auto timepoint2dcdate(std::chrono::system_clock::time_point x) -> v1::DcDate {
        using namespace std::chrono;
        using namespace ctk::impl;
        using xseconds = system_clock::duration;
        using T = xseconds::rep;

        constexpr const system_clock::time_point e{ date::sys_days{ dcdate_epoch } };

        constexpr const T epoch{ e.time_since_epoch().count() };
        const T point{ x.time_since_epoch().count() };
        const xseconds span{ minus(point, epoch, ok{}) }; // guard

        const seconds sec{ floor<seconds>(span) };
        const xseconds subsec{ span - sec };
        const double days{ seconds2days(static_cast<double>(sec.count())) };
        const double subseconds{ from_sys_clock(static_cast<double>(subsec.count())) };

        return { days, subseconds };
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
        using xseconds = system_clock::duration;

        const xseconds x_us{ duration_cast<xseconds>(x.time_since_epoch()) };
        const days x_days{ floor<days>(x_us) };
        const date::year_month_day ymd{ date::sys_days{ x_days } };
        oss << ymd << " ";

        xseconds reminder{ x_us - x_days };
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

