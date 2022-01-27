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

#pragma once

#include <string>
#include <array>
#include <vector>
#include <chrono>
#include <ostream>


namespace ctk { namespace api {

namespace v1 {

    inline constexpr const size_t evt_label_size{ 8 };

    struct Trigger
    {
        int64_t sample;
        // label 8 bytes, '\0' 1 byte, 1 byte waisted in order to avoid odd structure size
        std::array<char, evt_label_size + 2> code;

        Trigger();
        Trigger(int64_t sample, const std::string& code);
        Trigger(int64_t sample, const std::array<char, evt_label_size>& code);
        Trigger(const Trigger&) = default;
        Trigger(Trigger&&) = default;
        auto operator=(const Trigger&) -> Trigger& = default;
        auto operator=(Trigger&&) -> Trigger& = default;
        ~Trigger() = default;

        friend auto operator==(const Trigger&, const Trigger&) -> bool;
        friend auto operator!=(const Trigger&, const Trigger&) -> bool;
    };
    auto operator<<(std::ostream&, const Trigger&) -> std::ostream&;

    auto trigger_label(const std::array<char, evt_label_size + 2>&) -> std::string;


    struct DcDate
    {
        double date; // amount of seconds since 30 Dec 1899 divided by the numer of seconds per day (86400)
        double fraction; // subsecond amount

        DcDate();
        DcDate(double date, double fraction);
        DcDate(const DcDate&) = default;
        DcDate(DcDate&&) = default;
        auto operator=(const DcDate&) -> DcDate& = default;
        auto operator=(DcDate&&) -> DcDate& = default;
        ~DcDate() = default;

        friend auto operator==(const DcDate&, const DcDate&) -> bool = default;
        friend auto operator!=(const DcDate&, const DcDate&) -> bool = default;
    };
    auto operator<<(std::ostream&, const DcDate&) -> std::ostream&;


    enum class Sex { unknown, male, female };
    auto operator<<(std::ostream&, Sex) -> std::ostream&;
    auto sex2char(Sex) -> uint8_t;
    auto char2sex(uint8_t) -> Sex;


    enum class Handedness { unknown, left, right, mixed };
    auto operator<<(std::ostream&, Handedness) -> std::ostream&;
    auto handedness2char(Handedness) -> uint8_t;
    auto char2handedness(uint8_t) -> Handedness;


    struct Info
    {
        std::string hospital;
        std::string test_name;
        std::string test_serial;
        std::string physician;
        std::string technician;
        std::string machine_make;
        std::string machine_model;
        std::string machine_sn;
        std::string subject_name;
        std::string subject_id;
        std::string subject_address;
        std::string subject_phone;
        Sex subject_sex;
        Handedness subject_handedness;
        tm subject_dob;
        std::string comment;

        Info();
        Info(const Info&) = default;
        Info(Info&&) = default;
        auto operator=(const Info&) -> Info& = default;
        auto operator=(Info&&) -> Info& = default;
        ~Info() = default;

        friend auto operator==(const Info&, const Info&) -> bool;
        friend auto operator!=(const Info&, const Info&) -> bool;
    };
    auto operator<<(std::ostream&, const Info&) -> std::ostream&;


    struct Electrode
    {
        std::string label;
        std::string reference;
        std::string unit;
        std::string status;
        std::string type;
        double iscale;
        double rscale;

        Electrode(const std::string& label, const std::string& reference, const std::string& unit, double iscale, double rscale);
        Electrode();
        Electrode(const Electrode&) = default;
        Electrode(Electrode&&) = default;
        auto operator=(const Electrode&) -> Electrode& = default;
        auto operator=(Electrode&&) -> Electrode& = default;
        ~Electrode() = default;

        friend
        auto operator==(const Electrode& x, const Electrode& y) -> bool {
            return x.label == y.label
                && x.reference == y.reference
                && x.unit == y.unit
                && x.status == y.status
                && x.type == y.type
                && x.iscale == y.iscale
                && x.rscale == y.rscale;
        }

        friend
        auto operator!=(const Electrode& x, const Electrode& y) -> bool {
            return !(x == y);
        }
    };
    auto operator<<(std::ostream&, const Electrode&) -> std::ostream&;


    struct TimeSeries
    {
        std::chrono::system_clock::time_point start_time; // utc
        double sampling_frequency;
        std::vector<v1::Electrode> electrodes;
        int64_t epoch_length;
        // compression level?

        TimeSeries(std::chrono::system_clock::time_point, double, const std::vector<v1::Electrode>&, int64_t);
        TimeSeries(const DcDate&, double, const std::vector<v1::Electrode>&, int64_t);
        TimeSeries();
        TimeSeries(const TimeSeries&) = default;
        TimeSeries(TimeSeries&&) = default;
        auto operator=(const TimeSeries&) -> TimeSeries& = default;
        auto operator=(TimeSeries&&) -> TimeSeries& = default;
        ~TimeSeries() = default;

        friend auto operator==(const TimeSeries&, const TimeSeries&) -> bool;
        friend auto operator!=(const TimeSeries&, const TimeSeries&) -> bool;
    };
    auto operator<<(std::ostream&, const TimeSeries&) -> std::ostream&;


    enum class RiffType{ riff32, riff64 };
    auto operator<<(std::ostream&, RiffType) -> std::ostream&;


    struct FileVersion
    {
        uint32_t major;
        uint32_t minor;

        FileVersion();
        FileVersion(uint32_t major, uint32_t minor);
        FileVersion(const FileVersion&) = default;
        FileVersion(FileVersion&&) = default;
        auto operator=(const FileVersion&) -> FileVersion& = default;
        auto operator=(FileVersion&&) -> FileVersion& = default;
        ~FileVersion() = default;

        friend auto operator==(const FileVersion&, const FileVersion&) -> bool = default;
        friend auto operator!=(const FileVersion&, const FileVersion&) -> bool = default;
    };
    auto operator<<(std::ostream&, const FileVersion&) -> std::ostream&;

    struct UserFile
    {
        // riff restriction: 4 characters.
        // libeep restriction: cannot be any of: "eeph", "info", "evt ", "raw3", "rawf", "stdd", "tfh ", "tfd ", "refh" or "imp ".
        std::string label;
        std::string file_name;

        UserFile() = default;
        UserFile(const std::string& label, const std::string& file_name);
        UserFile(const UserFile&) = default;
        UserFile(UserFile&&) = default;
        auto operator=(const UserFile&) -> UserFile& = default;
        auto operator=(UserFile&&) -> UserFile& = default;
        ~UserFile() = default;
        friend auto operator==(const UserFile&, const UserFile&) -> bool = default;
        friend auto operator!=(const UserFile&, const UserFile&) -> bool = default;
    };
    auto operator<<(std::ostream&, const UserFile&) -> std::ostream&;

    struct Version
    {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
        uint32_t build;

        Version();
        Version(const Version&) = default;
        Version(Version&&) = default;
        auto operator=(const Version&) -> Version& = default;
        auto operator=(Version&&) -> Version& = default;
        ~Version() = default;
    };
    auto operator<<(std::ostream&, const Version&) -> std::ostream&;


    struct EventImpedance
    {
        std::chrono::system_clock::time_point stamp;
        std::vector<float> values; // in Ohm

        EventImpedance();
        EventImpedance(std::chrono::system_clock::time_point, const std::vector<float>&);
        EventImpedance(const DcDate&, const std::vector<float>&);
        EventImpedance(const EventImpedance&) = default;
        auto operator=(const EventImpedance&) -> EventImpedance& = default;
        auto operator=(EventImpedance&&) -> EventImpedance& = default;
        ~EventImpedance() = default;
        friend auto operator==(const EventImpedance&, const EventImpedance&) -> bool = default;
        friend auto operator!=(const EventImpedance&, const EventImpedance&) -> bool = default;
    };


    struct EventVideo
    {
        std::chrono::system_clock::time_point stamp;
        double duration;
        int32_t trigger_code;
        std::wstring condition_label;
        std::string description;
        std::wstring video_file;

        EventVideo();
        EventVideo(std::chrono::system_clock::time_point, double, int32_t);
        EventVideo(const DcDate&, double, int32_t);
        EventVideo(const EventVideo&) = default;
        auto operator=(const EventVideo&) -> EventVideo& = default;
        auto operator=(EventVideo&&) -> EventVideo& = default;
        ~EventVideo() = default;
        friend auto operator==(const EventVideo&, const EventVideo&) -> bool = default;
        friend auto operator!=(const EventVideo&, const EventVideo&) -> bool = default;
    };


    struct EventEpoch
    {
        std::chrono::system_clock::time_point stamp;
        double duration;
        double offset;
        int32_t trigger_code;
        std::wstring condition_label;

        EventEpoch();
        EventEpoch(std::chrono::system_clock::time_point, double duration, double offset, int32_t);
        EventEpoch(const DcDate&, double duration, double offset, int32_t);
        EventEpoch(const EventEpoch&) = default;
        auto operator=(const EventEpoch&) -> EventEpoch& = default;
        auto operator=(EventEpoch&&) -> EventEpoch& = default;
        ~EventEpoch() = default;
        friend auto operator==(const EventEpoch&, const EventEpoch&) -> bool = default;
        friend auto operator!=(const EventEpoch&, const EventEpoch&) -> bool = default;
    };


} /* namespace v1 */


    auto dcdate2timepoint(const v1::DcDate&) -> std::chrono::system_clock::time_point;
    auto timepoint2dcdate(std::chrono::system_clock::time_point) -> v1::DcDate;

    auto print(std::ostream&, std::chrono::system_clock::time_point) -> std::ostream&;
    auto print(std::ostream&, const v1::DcDate&) -> std::ostream&;

} /* namespace api*/ } /* namespace ctk */
