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

#include <limits>
#include <chrono>

#include "arithmetic.h"
#include "api_data.h"


namespace ctk { namespace impl {

    enum vt_e
    {
        vt_empty = 0x0,
        vt_null = 0x1,
        vt_i2 = 0x2,
        vt_i4 = 0x3,
        vt_r4 = 0x4,
        vt_r8 = 0x5,
        vt_bstr = 0x8,
        vt_bool = 0xb,
        vt_variant = 0xc,
        vt_i1 = 0x10,
        vt_u1 = 0x11,
        vt_u2 = 0x12,
        vt_u4 = 0x13,
        //vt_i8 = 0x14,
        //vt_u8 = 0x15,
        vt_array = 0x2000,
        vt_byref = 0x4000,
    };


    // simple variant implementation sufficient for working with the legacy event files.
    // uses strings in order to encode values of different types.
    struct str_variant
    {
        vt_e type;
        bool is_array;
        std::vector<std::string> data;

        str_variant();
        str_variant(int8_t);
        str_variant(int16_t);
        str_variant(int32_t);
        //str_variant(int64_t);
        str_variant(uint8_t);
        str_variant(uint16_t);
        str_variant(uint32_t);
        //str_variant(uint64_t);
        str_variant(float);
        str_variant(double);
        str_variant(bool);
        str_variant(const std::wstring&);

        str_variant(const std::vector<int8_t>&);
        str_variant(const std::vector<int16_t>&);
        str_variant(const std::vector<int32_t>&);
        //str_variant(const std::vector<int64_t>&);
        str_variant(const std::vector<uint8_t>&);
        str_variant(const std::vector<uint16_t>&);
        str_variant(const std::vector<uint32_t>&);
        //str_variant(const std::vector<uint64_t>&);
        str_variant(const std::vector<float>&);
        str_variant(const std::vector<double>&);
        str_variant(const std::vector<bool>&);
        str_variant(const std::vector<std::wstring>&);
    };

    auto is_int8(const str_variant&) -> bool;
    auto is_int16(const str_variant&) -> bool;
    auto is_int32(const str_variant&) -> bool;
    //auto is_int64(const str_variant&) -> bool;
    auto is_uint8(const str_variant&) -> bool;
    auto is_uint16(const str_variant&) -> bool;
    auto is_uint32(const str_variant&) -> bool;
    //auto is_uint64(const str_variant&) -> bool;
    auto is_float(const str_variant&) -> bool;
    auto is_double(const str_variant&) -> bool;
    auto is_bool(const str_variant&) -> bool;
    auto is_wstring(const str_variant&) -> bool;
    auto is_float_array(const str_variant&) -> bool;

    auto as_int8(const str_variant&) -> int8_t;
    auto as_int16(const str_variant&) -> int16_t;
    auto as_int32(const str_variant&) -> int32_t;
    //auto as_int64(const str_variant&) -> int64_t;
    auto as_uint8(const str_variant&) -> uint8_t;
    auto as_uint16(const str_variant&) -> uint16_t;
    auto as_uint32(const str_variant&) -> uint32_t;
    //auto as_uint64(const str_variant&) -> uint64_t;
    auto as_float(const str_variant&) -> float;
    auto as_double(const str_variant&) -> double;
    auto as_bool(const str_variant&) -> bool;
    auto as_wstring(const str_variant&) -> std::wstring;
    auto as_float_array(const str_variant&) -> std::vector<float>;


    struct event_descriptor
    {
        std::string name;
        std::string unit;
        str_variant value;

        event_descriptor();
        event_descriptor(const str_variant& value);
        event_descriptor(const str_variant& value, const std::string& name);
        event_descriptor(const str_variant& value, const std::string& name, const std::string& unit);
    };


    struct guid {
        uint32_t data1;
        uint16_t data2;
        uint16_t data3;
        uint8_t data4[8];

        guid();
    };


    struct base_event
    {
        int32_t visible_id;
        guid unused;
        std::string name;
        std::string user_visible_name;

        int32_t type;
        int32_t state;
        int8_t original;
        double duration;
        double duration_offset;
        std::chrono::system_clock::time_point stamp;
        std::vector<event_descriptor> descriptors;

        base_event();
        base_event(const std::chrono::system_clock::time_point& stamp, int32_t type, const std::string& name, const std::vector<event_descriptor>& descriptors, double duration, double offset);
    };


    struct channel_info
    {
        std::string active;
        std::string reference;
    };


    struct epoch_event
    {
        base_event common;
    };


    struct marker_event
    {
        base_event common;
        channel_info channel;
        std::string description;
        int32_t show_amplitude;
        int8_t show_duration;

        marker_event();
        marker_event(const base_event& common, const std::string& description);
    };


    struct artefact_event
    {
        base_event common;
        channel_info channel;
        std::string description;
    };


    struct spike_event
    {
        base_event common;
        channel_info channel;
        float amplitude_peak;
        int16_t sign;
        int16_t group;
        std::chrono::system_clock::time_point top_date;
    };


    struct seizure_event
    {
        base_event common;
        channel_info channel;
    };


    struct sleep_event
    {
        base_event common;
        int16_t base_level;
        int16_t threshold;
        int16_t min_duration;
        int16_t max_value;
        int16_t epoch_length;
        int32_t epoch_color;
    };


    struct rpeak_event
    {
        base_event common;
        channel_info channel;
        float amplitude_peak;
    };


    struct event_library
    {
        std::vector<epoch_event> epochs;
        std::vector<marker_event> impedances;
        std::vector<marker_event> videos;
        std::vector<marker_event> markers;
        std::vector<artefact_event> artefacts;
        std::vector<seizure_event> seizures;
        std::vector<spike_event> spikes;
        std::vector<sleep_event> sleeps;
        std::vector<rpeak_event> rpeaks;

        int32_t version;
        std::string name;

        static auto default_output_file_version() -> int32_t { return 104; }

        event_library();
    };


    template<typename T>
    auto event_count(const event_library& lib, T type_tag) -> T {
        auto total = lib.epochs.size();
        total += lib.impedances.size();
        total += lib.videos.size();
        total += lib.markers.size();
        total += lib.artefacts.size();
        total += lib.seizures.size();
        total += lib.spikes.size();
        total += lib.sleeps.size();
        total += lib.rpeaks.size();

        return cast(total, type_tag, ok{});
    }


    auto read_archive(FILE*) -> event_library;
    auto write_archive(FILE*, const event_library&) -> void;
    auto add_impedance(marker_event, event_library&) -> void;
    auto add_video(marker_event, event_library&) -> void;
    auto add_marker(marker_event, event_library&) -> void;
    auto add_epoch(epoch_event, event_library&) -> void;

    auto marker2impedance(const marker_event&) -> api::v1::EventImpedance;
    auto impedance2marker(const api::v1::EventImpedance&) -> marker_event;
    auto marker2video(const marker_event&) -> api::v1::EventVideo;
    auto video2marker(const api::v1::EventVideo&) -> marker_event;
    auto epochevent2eventepoch(const epoch_event&) -> api::v1::EventEpoch;
    auto eventepoch2epochevent(const api::v1::EventEpoch&) -> epoch_event;
    auto read_class(FILE*, int32_t& class_tag, std::string&) -> bool;
    auto load_event(FILE*, event_library&, const std::string&) -> void;

    auto write_impedance(FILE*, const marker_event&, int version) -> void;
    auto write_video(FILE*, const marker_event&, int version) -> void;
    auto write_epoch(FILE*, const epoch_event&, int version) -> void;
    auto write_partial_archive(FILE*, const event_library&, uint32_t event_count) -> void;

} /* namespace impl */ } /* namespace ctk */

