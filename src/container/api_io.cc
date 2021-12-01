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

#include <cstring>

#include "container/api_io.h"
#include "compress/leb128.h"
#include "container/io.h"
#include "container/file_epoch.h" // for as_string(trigger)
#include "date/date.h"

namespace ctk { namespace impl {

    auto tm2timepoint(const std::tm& x) -> std::chrono::system_clock::time_point {
        using namespace std::chrono;

        if (x.tm_mon < 0 || 11 < x.tm_mon ||
            x.tm_mday < 1 || 31 < x.tm_mday ||
            x.tm_hour < 0 || 23 < x.tm_hour ||
            x.tm_min < 0 || 59 < x.tm_min ||
            x.tm_sec < 0 || 60 < x.tm_sec) {
            assert(false); // invalid input
        }

        const int y{ x.tm_year + 1900 }; // TODO: guard
        const unsigned m{ static_cast<unsigned>(x.tm_mon + 1) };
        const unsigned d{ static_cast<unsigned>(x.tm_mday) };
        date::year_month_day ymd{ date::year{ y }, date::month{ m }, date::day{ d } };
        system_clock::time_point base{ date::sys_days{ ymd } };

        hours hours{ x.tm_hour };
        minutes minutes{ x.tm_min };
        seconds seconds{ x.tm_sec };

        return base + hours + minutes + seconds;
    }

    auto timepoint2tm(std::chrono::system_clock::time_point x) -> std::tm {
        using namespace std::chrono;

        std::tm result;
        std::memset(&result, 0, sizeof(result));

        constexpr const std::chrono::system_clock::time_point tm_epoch{ date::sys_days{ date::December/31/1899 } };
        if (x < tm_epoch) {
            // invalid input
            return result;
        }

        const seconds s{ duration_cast<seconds>(x.time_since_epoch()) };
        const date::days ds{ duration_cast<date::days>(s) };
        const date::year_month_day ymd{ date::sys_days{ ds } };
        const seconds hh_mm_ss = s - ds;

        const hours hh{ duration_cast<hours>(hh_mm_ss) };
        const minutes mm{ duration_cast<minutes>(hh_mm_ss - hh) };
        const seconds ss{ duration_cast<seconds>(hh_mm_ss - hh - mm) };

        if (!ymd.ok() ||
            hh.count() < 0 || 23 < hh.count() ||
            mm.count() < 0 || 59 < mm.count() ||
            ss.count() < 0 || 60 < ss.count()) {
            return result;
        }

        const auto y{ static_cast<int>(ymd.year()) };
        const auto m{ static_cast<unsigned>(ymd.month()) };
        const auto d{ static_cast<unsigned>(ymd.day()) };

        result.tm_year = y - 1900; // TODO: guard
        result.tm_mon = static_cast<int>(m) - 1;
        result.tm_mday = static_cast<int>(d);
        result.tm_hour = static_cast<int>(hh.count());
        result.tm_min = static_cast<int>(mm.count());
        result.tm_sec = static_cast<int>(ss.count());
        result.tm_isdst = -1;
        return result;
    }



    enum object_tags : uint8_t {
          collection
        , time_point
        , trigger
        , dcdate
        , sex
        , handedness
        , info
        , electrode
        , time_signal
        , time_series_header
        , length
    };


    template<typename T>
    auto write_uleb128(FILE* f, T x) -> void {
        const auto bytes{ encode_uleb128_v(x) };
        write(f, begin(bytes), end(bytes));
    }

    template<typename T>
    auto read_uleb128(FILE* f, T/*typei tag*/) -> T {
        constexpr const leb128::uleb unsigned_leb;
        leb128::lebstate<T> state;
        bool more{ true };
        uint8_t input;

        do {
            input = read(f, uint8_t{});
            more = leb128::decode_byte(input, state, unsigned_leb);
        } while(more);

        return state.x;
    }


    static
    auto write(FILE* f, const std::string& xs) -> void {
        write_uleb128(f, xs.size());
        write(f, begin(xs), end(xs));
    }

    static
    auto read(FILE* f, std::string xs) -> std::string {
        const size_t size{ read_uleb128(f, size_t{}) };

        xs.resize(size);
        read(f, begin(xs), end(xs));
        return xs;
    }

    auto write(FILE* f, const api::v1::Trigger& x) -> void {
        const auto sample{ encode_sleb128_v(x.sample) };

        write(f, object_tags::trigger);
        write(f, begin(sample), end(sample));
        write(f, ctk::impl::as_string(x.code));
    }

    auto write(FILE* f, const api::v1::DcDate& x) -> void {
        write(f, object_tags::dcdate);
        write(f, x.date);
        write(f, x.fraction);
    }

    auto write(FILE* f, const api::v1::Sex& x) -> void {
        write(f, object_tags::sex);
        write(f, api::v1::sex2char(x));
    }

    auto write(FILE* f, const api::v1::Handedness& x) -> void {
        write(f, object_tags::handedness);
        write(f, api::v1::handedness2char(x));
    }

    auto write(FILE* f, std::chrono::system_clock::time_point x) -> void {
        const int64_t ns{ std::chrono::duration_cast<std::chrono::nanoseconds>(x.time_since_epoch()).count() };

        write(f, object_tags::time_point);
        write(f, ns);
    }

    auto write(FILE* f, const std::tm& x) -> void {
        write(f, tm2timepoint(x));
    }

    auto write_info(FILE* f, const api::v1::Info& x) -> void {
        write(f, object_tags::info);
        write(f, x.hospital);
        write(f, x.test_name);
        write(f, x.test_serial);
        write(f, x.physician);
        write(f, x.technician);
        write(f, x.machine_make);
        write(f, x.machine_model);
        write(f, x.machine_sn);
        write(f, x.subject_name);
        write(f, x.subject_id);
        write(f, x.subject_address);
        write(f, x.subject_phone);
        write(f, x.subject_sex);
        write(f, x.subject_handedness);
        write(f, x.subject_dob);
        write(f, x.comment);
    }

    static
    auto write_electrode(FILE* f, const api::v1::Electrode& x) -> void {
        write(f, object_tags::electrode);
        write(f, x.label);
        write(f, x.reference);
        write(f, x.unit);
        write(f, x.status);
        write(f, x.type);
        write(f, x.iscale);
        write(f, x.rscale);
    }

    static
    auto read_electrode(FILE* f) -> api::v1::Electrode {
        api::v1::Electrode x;

        const uint8_t tag{ read(f, uint8_t{}) };
        if (tag != static_cast<uint8_t>(object_tags::electrode)) {
            std::cerr << "\nread_collection_size: expected " << unsigned(object_tags::electrode) << ", got " << unsigned(tag) << "\n";
            throw api::v1::ctk_data{ "read_collection_size: not an electrode" };
        }

        const std::string tag_string;
        constexpr const double tag_double{ 0 };

        x.label = read(f, tag_string);
        x.reference = read(f, tag_string);
        x.unit = read(f, tag_string);
        x.status = read(f, tag_string);
        x.type = read(f, tag_string);
        x.iscale = read(f, tag_double);
        x.rscale = read(f, tag_double);
        return x;
    }

    static
    auto write_collection_size(FILE* f, size_t size) -> void {
        const auto xs{ encode_uleb128_v(size) };

        write(f, object_tags::collection);
        write(f, begin(xs), end(xs));
    }

    static
    auto read_collection_size(FILE* f) -> size_t {
        const uint8_t tag{ read(f, uint8_t{}) };
        if (tag != static_cast<uint8_t>(object_tags::collection)) {
            std::cerr << "\nread_collection_size: expected " << unsigned(object_tags::collection) << ", got " << unsigned(tag) << "\n";
            throw api::v1::ctk_data{ "read_collection_size: not a collection" };
        }

        return read_uleb128(f, size_t{});
    }

    auto write_electrodes(FILE* f, const std::vector<api::v1::Electrode>& xs) -> void {
        write_collection_size(f, xs.size());
        for (const auto& x : xs) {
            write_electrode(f, x);
        }
    }

    auto read_electrodes(FILE* f) -> std::vector<api::v1::Electrode> {
        const size_t size{ read_collection_size(f) };
        std::vector<api::v1::Electrode> xs(size);
        for (size_t i{ 0 }; i < size; ++i) {
            xs[i] = read_electrode(f);
        }
        return xs;
    }

    auto write_time_series(FILE* f, const api::v2::TimeSeries& x) -> void {
        const auto epoch_length{ encode_sleb128_v(x.epoch_length) };

        write(f, object_tags::time_signal);
        write(f, x.start_time);
        write(f, x.sampling_frequency);
        write(f, begin(epoch_length), end(epoch_length));
        write_electrodes(f, x.electrodes);
        // TODO: encoding parameters: format, size, sidedness
    }

    auto write_time_series(FILE* f, const api::v1::TimeSignal& x) -> void {
        write(f, api::v2::TimeSeries{ x });
    }

    auto write_time_series_header(FILE* f, const api::v2::TimeSeries& x, int64_t sample_count, size_t input_data_size, bool is_signed) -> void {
        const uint8_t data_size{ static_cast<uint8_t>(input_data_size) };
        const uint8_t signum{ static_cast<uint8_t>(is_signed) };

        write(f, object_tags::time_series_header);
        write(f, sample_count);
        write_time_series(f, x);
        write(f, data_size);
        write(f, signum);
    }

    auto patch_time_series_header(FILE* f, int64_t sample_count) -> void {
        seek(f, sizeof(object_tags), SEEK_SET);
        write(f, sample_count);
    }

} /* namespace impl */ } /* namespace ctk */

