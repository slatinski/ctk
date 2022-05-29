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

#include "api_data.h"

#include <cmath>
#include <iomanip>
#include <algorithm>
#include "date/date.h"

#include "exception.h"
#include "arithmetic.h"
#include "file/cnt_epoch.h"
#include "logger.h"

namespace ctk { namespace api {

    using namespace ctk::impl;

    namespace v1 {

        Trigger::Trigger()
        : Sample{ std::numeric_limits<int64_t>::max() } {
        }

        Trigger::Trigger(int64_t s, const std::string& c)
        : Sample{ s }
        , Code{ c } {
        }

        auto operator<<(std::ostream& os, const Trigger& x) -> std::ostream& {
            os << x.Sample << " " << x.Code;
            return os;
        }

        DcDate::DcDate()
        : Date{ 0 }
        , Fraction{ 0 } {
        }

        DcDate::DcDate(double date, double fraction)
        : Date{ date }
        , Fraction{ fraction } {
        }

        auto operator<<(std::ostream& os, const DcDate& x) -> std::ostream& {
            os << x.Date << " " << x.Fraction;
            return os;
        }

        auto operator<<(std::ostream& os, Sex x) -> std::ostream& {
            const uint8_t y{ sex2char(x) };
            os << (y == 0 ? 'U' : y);
            return os;
        }

        auto Sex2Char(Sex x) -> uint8_t {
            return sex2char(x);
        }

        auto Char2Sex(uint8_t x) -> Sex {
            return char2sex(x);
        }


        auto operator<<(std::ostream& os, Handedness x) -> std::ostream& {
            os << hand2char(x);
            return os;
        }

        auto Hand2Char(Handedness x) -> uint8_t {
            return hand2char(x);
        }

        auto Char2Hand(uint8_t x) -> Handedness {
            return char2hand(x);
        }


        Info::Info()
        : SubjectSex{ Sex::Unknown }
        , SubjectHandedness{ Handedness::Unknown }
        , SubjectDob{ tm2timepoint(make_tm()) } {
        }

        auto operator<<(std::ostream& os, const Info& x) -> std::ostream& {
            if (!x.Hospital.empty()) { os << "hospital " << x.Hospital; os << " "; }
            if (!x.TestName.empty()) { os << "test name " << x.TestName; os << " "; }
            if (!x.TestSerial.empty()) { os << "test serial " << x.TestSerial; os << " "; }
            if (!x.Physician.empty()) { os << "physician " << x.Physician; os << " "; }
            if (!x.Technician.empty()) { os << "technician " << x.Technician; os << " "; }
            if (!x.MachineMake.empty()) { os << "machine make " << x.MachineMake; os << " "; }
            if (!x.MachineModel.empty()) { os << "machine model " << x.MachineModel; os << " "; }
            if (!x.MachineSn.empty()) { os << "machine sn " << x.MachineSn; os << " "; }
            if (!x.SubjectName.empty()) { os << "subject name " << x.SubjectName; os << " "; }
            if (!x.SubjectId.empty()) { os << "subject id " << x.SubjectId; os << " "; }
            if (!x.SubjectAddress.empty()) { os << "subject address " << x.SubjectAddress; os << " "; }
            if (!x.SubjectPhone.empty()) { os << "subject phone " << x.SubjectPhone; os << " "; }
            if (x.SubjectSex != Sex::Unknown) { os << "subject sex " << x.SubjectSex; os << " "; }
            if (x.SubjectHandedness != Handedness ::Unknown) { os << "subject handedness " << x.SubjectHandedness; os << " "; }
            if (x.SubjectDob != tm2timepoint(make_tm())) { os << "subject date of birth "; print(os, x.SubjectDob); os << " "; }
            if (!x.Comment.empty()) { os << "comment [" << x.Comment; os << "] "; }
            return os;
        }


        // compatibility: uV is stored in some files as µV so that the first character is not valid utf8
        auto utf8compat_hack(std::string x) -> std::string {
            if (!x.empty() && x[0] == -75) {
                std::ostringstream oss;
                oss << "[Electrode, api_data] replacement " << x << " -> uV";
                ctk_log_trace(oss.str());
                x[0] = 'u';
            }
            return x;
        }


        Electrode::Electrode()
        : Unit{ default_unit() }
        , IScale{ 1 }
        , RScale{ 1 / default_scaling_factor() } {
        }

        Electrode::Electrode(const std::string& active, const std::string& reference)
        : ActiveLabel{ active }
        , Reference{ reference }
        , Unit{ default_unit() }
        , IScale{ 1 }
        , RScale{ 1 / default_scaling_factor() } {
        }

        Electrode::Electrode(const std::string& active, const std::string& reference, const std::string& unit)
        : ActiveLabel{ active }
        , Reference{ reference }
        , Unit{ utf8compat_hack(unit) }
        , IScale{ 1 }
        , RScale{ 1 } {
            if (Unit == default_unit()) {
                RScale = 1 / default_scaling_factor();
            }
        }

        Electrode::Electrode(const std::string& active, const std::string& reference, const std::string& unit, double is, double rs)
        : ActiveLabel{ active }
        , Reference{ reference }
        , Unit{ utf8compat_hack(unit) }
        , IScale{ is }
        , RScale{ rs } {
        }

        auto operator<<(std::ostream& os, const Electrode& x) -> std::ostream& {
            os << x.ActiveLabel;
            if (!x.Reference.empty()) {
                os << "-" << x.Reference;
            }

            os << " " << x.Unit
               << " " << d2s(x.IScale, 13)
               << " " << d2s(x.RScale, 13);

            if (!x.Type.empty()) {
               os << " " << x.Type;
            }

            if (!x.Status.empty()) {
               os << " " << x.Status;
            }
            return os;
        }


        TimeSeries::TimeSeries(std::chrono::system_clock::time_point t, double f, const std::vector<v1::Electrode>& e, int64_t l)
        : StartTime{ t }
        , SamplingFrequency{ f }
        , Electrodes{ e }
        , EpochLength{ l } {
        }

        TimeSeries::TimeSeries(const DcDate& t, double f, const std::vector<v1::Electrode>& e, int64_t l)
        : StartTime{ dcdate2timepoint(t) }
        , SamplingFrequency{ f }
        , Electrodes{ e }
        , EpochLength{ l } {
        }

        TimeSeries::TimeSeries()
        : StartTime{ dcdate2timepoint(DcDate{}) }
        , SamplingFrequency{ 0 }
        , EpochLength{ 1024 } {
        }

        auto operator==(const TimeSeries& x, const TimeSeries& y) -> bool {
            return x.StartTime == y.StartTime
                && std::fabs(x.SamplingFrequency - y.SamplingFrequency) < 1e-6
                && x.Electrodes == y.Electrodes
                && x.EpochLength == y.EpochLength;
        }

        auto operator!=(const TimeSeries& x, const TimeSeries& y) -> bool {
            return !(x == y);
        }

        auto operator<<(std::ostream& os, const TimeSeries& x) -> std::ostream& {
            os << x.SamplingFrequency << " Hz, ";
            print(os, x.StartTime);
            os << " UTC,";

            const size_t size{ x.Electrodes.size() };
            const size_t count{ std::min(size, size_t{ 2 }) };
            for (size_t i{ 0 }; i < count; ++i) {
                os << " ( " << x.Electrodes[i] << " )";
            }

            if (count < size) {
                os << "...";
            }
            return os;
        }

        auto operator<<(std::ostream& os, RiffType x) -> std::ostream& {
            switch(x) {
            case RiffType::riff32:
                os << root_id_riff32();
                break;
            case RiffType::riff64:
                os << root_id_riff64();
                break;
            default: {
                std::ostringstream oss;
                oss << "[operator<<(RiffType), api_data] invalid " << static_cast<int>(x);
                const auto e{ oss.str() };
                ctk_log_critical(e);
                throw CtkBug{ e };
                }
            }
            return os;
        }

        FileVersion::FileVersion()
        : Major{ 0 }
        , Minor{ 0 } {
        }

        FileVersion::FileVersion(uint32_t mj, uint32_t mi)
        : Major{ mj }
        , Minor{ mi } {
        }

        auto operator<<(std::ostream& os, const FileVersion& x) -> std::ostream& {
            os << x.Major << "." << x.Minor;
            return os;
        }

        UserFile::UserFile(const std::string& label, const std::string& file_name)
        : Label{ label }
        , FileName{ file_name } {
        }

        auto operator<<(std::ostream& os, const UserFile& x) -> std::ostream& {
            os << x.Label << ": " << x.FileName;
            return os;
        }


        EventImpedance::EventImpedance()
        : Stamp{ dcdate2timepoint(DcDate{}) } {
        }

        EventImpedance::EventImpedance(std::chrono::system_clock::time_point t, const std::vector<float>& xs)
        : Stamp{ t }
        , Values{ xs } {
        }

        EventImpedance::EventImpedance(const DcDate& t, const std::vector<float>& xs)
        : Stamp{ dcdate2timepoint(t) }
        , Values{ xs } {
        }



        EventVideo::EventVideo()
        : Stamp{ dcdate2timepoint(DcDate{}) }
        , Duration{ 0 }
        , TriggerCode{ std::numeric_limits<int32_t>::min() } {
        }

        EventVideo::EventVideo(std::chrono::system_clock::time_point t, double d, int32_t c)
        : Stamp{ t }
        , Duration{ d }
        , TriggerCode{ c } {
        }

        EventVideo::EventVideo(const DcDate& t, double d, int32_t c)
        : Stamp{ dcdate2timepoint(t) }
        , Duration{ d }
        , TriggerCode{ c } {
        }



        EventEpoch::EventEpoch()
        : Stamp{ dcdate2timepoint(DcDate{}) }
        , Duration{ 0 }
        , Offset{ 0 }
        , TriggerCode{ std::numeric_limits<int32_t>::min() } {
        }

        EventEpoch::EventEpoch(std::chrono::system_clock::time_point t, double d, double o, int32_t c)
        : Stamp{ t }
        , Duration{ d }
        , Offset{ o }
        , TriggerCode{ c } {
        }

        EventEpoch::EventEpoch(const DcDate& t, double d, double o, int32_t c)
        : Stamp{ dcdate2timepoint(t) }
        , Duration{ d }
        , Offset{ o }
        , TriggerCode{ c } {
        }


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
            if (x.Fraction < 0) {
                std::ostringstream oss;
                oss << "[regular, api_data] negative subseconds " << x;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw ctk::api::v1::CtkLimit{ e };
            }
            const double i_fraction{ floor(x.Fraction) };

            const double seconds{ days2seconds(x.Date) + i_fraction };
            const double subseconds{ x.Fraction - i_fraction };
            const double days{ seconds2days(seconds) };
            if (!std::isfinite(days) || !std::isfinite(subseconds)) {
                std::ostringstream oss;
                oss << "[regular, api_data] infinite component " << x;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw ctk::api::v1::CtkLimit{ e };
            }
            assert(std::fabs(subseconds) < 1);

            return { days, subseconds };
        }


        // performs the computation and comparison on doubles in order to avoid signed integer overflow.
        // because of that the precision around the extremes is reduced.
        static
        auto clock_guard(const ctk::api::v1::DcDate& x) -> bool {
            using namespace std::chrono;
            using dsec = duration<double, std::ratio<1>>;
            using dsys = duration<double, system_clock::period>;
            using sys_tp = time_point<system_clock, dsys>;
            using T = system_clock::rep;

            constexpr const sys_tp t_min{ dsys{ static_cast<double>(std::numeric_limits<T>::min()) } };
            constexpr const sys_tp t_max{ dsys{ static_cast<double>(std::numeric_limits<T>::max()) } };
            constexpr const sys_tp e{ date::sys_days{ dcdate_epoch } }; // negative: 1899/12/30 < 1970/1/1
            constexpr const dsys slack{ 1h };
            constexpr const sys_tp e_max{ t_max + e.time_since_epoch() - slack };
            constexpr const sys_tp e_min{ t_min + slack };

            const dsec seconds{ days2seconds(x.Date) };
            const dsys subseconds{ to_sys_clock(x.Fraction) };
            const sys_tp y{ e + seconds + subseconds };

            return e_min < y && y < e_max;
        }



        auto dcdate2timepoint(v1::DcDate x) -> std::chrono::system_clock::time_point {
            using namespace std::chrono;
            using xseconds = system_clock::duration;

            x = regular(x);
            if (!clock_guard(x)) {
                std::ostringstream oss;
                oss << "[dcdate2timepoint, api_data] out of clock range " << x;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw ctk::api::v1::CtkLimit{ e };
            }

            const double sec{ std::round(days2seconds(x.Date)) };
            const double subsec{ std::round(to_sys_clock(x.Fraction)) };
            const seconds whole{ static_cast<seconds::rep>(sec) };
            const xseconds fraction{ static_cast<xseconds::rep>(subsec) };

            constexpr const system_clock::time_point epoch{ date::sys_days{ dcdate_epoch } };
            return epoch + whole + fraction;
        }


        auto timepoint2dcdate(std::chrono::system_clock::time_point x) -> v1::DcDate {
            using namespace std::chrono;
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

    } /* namespace v1 */


} /* namespace api */ } /* namespace ctk */

