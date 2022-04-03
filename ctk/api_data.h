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
#include <vector>
#include <chrono>


namespace ctk { namespace api {

namespace v1 {

    namespace sizes {
        // up to that many bytes can be round-tripped
        static constexpr const unsigned evt_trigger_code{ 8 };
        static constexpr const unsigned eeph_electrode_active{ 10 };
        static constexpr const unsigned eeph_electrode_unit{ 10 };
        static constexpr const unsigned eeph_electrode_reference{ 9 };
        static constexpr const unsigned eeph_electrode_status{ 9 };
        static constexpr const unsigned eeph_electrode_type{ 9 };
    } // namespace sizes

    struct Trigger
    {
        int64_t Sample;
        std::string Code;

        Trigger();
        Trigger(int64_t, const std::string&);
        Trigger(const Trigger&) = default;
        Trigger(Trigger&&) = default;
        auto operator=(const Trigger&) -> Trigger& = default;
        auto operator=(Trigger&&) -> Trigger& = default;
        ~Trigger() = default;

        friend auto operator==(const Trigger&, const Trigger&) -> bool = default;
        friend auto operator!=(const Trigger&, const Trigger&) -> bool = default;
    };
    auto operator<<(std::ostream&, const Trigger&) -> std::ostream&;



    struct DcDate
    {
        double Date;     // amount of seconds since 30 Dec 1899 divided by the numer of seconds per day (86400)
        double Fraction; // subsecond amount

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
    auto dcdate2timepoint(v1::DcDate) -> std::chrono::system_clock::time_point;
    auto timepoint2dcdate(std::chrono::system_clock::time_point) -> v1::DcDate;
    auto print(std::ostream&, std::chrono::system_clock::time_point) -> std::ostream&;


    enum class Sex { Female, Male, Unknown };
    auto operator<<(std::ostream&, Sex) -> std::ostream&;
    auto Sex2Char(Sex) -> uint8_t;
    auto Char2Sex(uint8_t) -> Sex;


    enum class Handedness { Left, Mixed, Right, Unknown };
    auto operator<<(std::ostream&, Handedness) -> std::ostream&;
    auto Hand2Char(Handedness) -> uint8_t;
    auto Char2Hand(uint8_t) -> Handedness;


    struct Info
    {
        std::string Hospital;
        std::string TestName;
        std::string TestSerial;
        std::string Physician;
        std::string Technician;
        std::string MachineMake;
        std::string MachineModel;
        std::string MachineSn;
        std::string SubjectName;
        std::string SubjectId;
        std::string SubjectAddress;
        std::string SubjectPhone;
        Sex SubjectSex;
        Handedness SubjectHandedness;
        std::chrono::system_clock::time_point SubjectDob; // utc implied, stored as struct tm - truncated to seconds precision
        std::string Comment;

        Info();
        Info(const Info&) = default;
        Info(Info&&) = default;
        auto operator=(const Info&) -> Info& = default;
        auto operator=(Info&&) -> Info& = default;
        ~Info() = default;

        friend auto operator==(const Info&, const Info&) -> bool = default;
        friend auto operator!=(const Info&, const Info&) -> bool = default;
    };
    auto operator<<(std::ostream&, const Info&) -> std::ostream&;


    struct Electrode
    {
        std::string ActiveLabel;
        std::string Reference;
        std::string Unit; // default: uV
        std::string Status;
        std::string Type;
        double IScale; // user customization point: instrument scaling. default 1
        double RScale; // range scaling. default 1/256 (3.9nV LSB, 16.75V p2p for 32-bit signed integrals)

        static auto default_scaling_factor() -> double { return 256; }
        static auto default_unit() -> std::string { return "uV"; }

        Electrode(const std::string& label, const std::string& reference);
        Electrode(const std::string& label, const std::string& reference, const std::string& unit);
        Electrode(const std::string& label, const std::string& reference, const std::string& unit, double iscale, double rscale);
        Electrode();
        Electrode(const Electrode&) = default;
        Electrode(Electrode&&) = default;
        auto operator=(const Electrode&) -> Electrode& = default;
        auto operator=(Electrode&&) -> Electrode& = default;
        ~Electrode() = default;

        friend auto operator==(const Electrode&, const Electrode&) -> bool = default;
        friend auto operator!=(const Electrode&, const Electrode&) -> bool = default;
    };
    auto operator<<(std::ostream&, const Electrode&) -> std::ostream&;


    struct TimeSeries
    {
        std::chrono::system_clock::time_point StartTime; // utc implied
        double SamplingFrequency; // Hz
        std::vector<Electrode> Electrodes;
        int64_t EpochLength;

        TimeSeries(std::chrono::system_clock::time_point, double, const std::vector<Electrode>&, int64_t);
        TimeSeries(const DcDate&, double, const std::vector<Electrode>&, int64_t);
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
        uint32_t Major;
        uint32_t Minor;

        FileVersion();
        FileVersion(uint32_t, uint32_t);
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
        // 4 characters long.
        // can not be any of "eeph", "info", "evt ", "raw3", "rawf", "stdd", "tfh ", "tfd ", "refh", "imp ", "nsh ", "vish", "egih", "egig", "egiz", "binh", "xevt", "xseg", "xsen", "xtrg"
        std::string Label;
        std::string FileName;

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


    struct EventImpedance
    {
        std::chrono::system_clock::time_point Stamp; // utc implied
        std::vector<float> Values; // Ohm, usually one per electrode

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
        std::chrono::system_clock::time_point Stamp; // utc implied
        double Duration;
        int32_t TriggerCode;
        std::wstring ConditionLabel;
        std::string Description;
        std::wstring VideoFile;

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
        std::chrono::system_clock::time_point Stamp; // utc implied
        double Duration;
        double Offset;
        int32_t TriggerCode;
        std::wstring ConditionLabel;

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

} /* namespace api*/ } /* namespace ctk */
