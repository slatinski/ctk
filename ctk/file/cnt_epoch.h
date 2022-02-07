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

#include <vector>
#include <array>
#include <string>
#include <filesystem>
#include <cassert>

#include "type_wrapper.h"
#include "api_data.h"
#include "file/io.h"
#include "file/ctk_part.h"

namespace ctk { namespace impl {


    struct ep_content
    {
        measurement_count length;
        std::vector<int64_t> offsets;

        ep_content(measurement_count length, const std::vector<int64_t>& offsets);
        ep_content() = default;
        ep_content(const ep_content&) = default;
        ep_content(ep_content&&) = default;
        ~ep_content() = default;

        auto operator=(const ep_content&) -> ep_content& = default;
        auto operator=(ep_content&&) -> ep_content& = default;

        friend auto operator==(const ep_content&, const ep_content&) -> bool = default;
        friend auto operator!=(const ep_content&, const ep_content&) -> bool = default;
    };



    struct cnt_field_sizes
    {
        virtual ~cnt_field_sizes() = default;
        virtual auto clone() -> cnt_field_sizes* = 0;

        virtual auto root_id() const -> std::string = 0;

        // riff size
        virtual auto entity_size() const -> size_t = 0;
        virtual auto write_entity(FILE*, int64_t) const -> void = 0;
        virtual auto read_entity(FILE*) const -> int64_t = 0;
        virtual auto read_ep(FILE*, const file_range&) const -> ep_content = 0;

        // evt size
        virtual auto read_triggers(FILE*, const file_range&) const -> std::vector<api::v1::Trigger> = 0;
        virtual auto write_triggers(FILE*, const std::vector<api::v1::Trigger>&) const -> void = 0;
        virtual auto write_trigger(FILE*, const api::v1::Trigger&) const -> void = 0;
    };
    using riff_ptr = std::unique_ptr<cnt_field_sizes>;


    struct chunk
    {
        label_type id;
        label_type label; // meaningfull only if the chunk is root or list
        riff_ptr riff;
        file_range storage;

        explicit chunk(api::v1::RiffType);
        chunk(const chunk&);
        chunk(chunk&&) = default;
        ~chunk() = default;

        auto operator=(const chunk&) -> chunk&;
        auto operator=(chunk&&) -> chunk& = default;

        friend auto swap(chunk&, chunk&) -> void;
        friend auto operator==(const chunk&, const chunk&) -> bool;
        friend auto operator!=(const chunk&, const chunk&) -> bool;
    };
    auto operator<<(std::ostream&, const chunk&) -> std::ostream&;



    struct user_content
    {
        std::string label;
        file_range storage;

        user_content(const std::string&, const file_range&);
        user_content() = default;
        user_content(const user_content&) = default;
        user_content(user_content&&) = default;
        ~user_content() = default;

        auto operator=(const user_content&) -> user_content& = default;
        auto operator=(user_content&&) -> user_content& = default;

        friend auto operator==(const user_content&, const user_content&) -> bool = default;
        friend auto operator!=(const user_content&, const user_content&) -> bool = default;
    };
    auto operator<<(std::ostream&, const user_content&) -> std::ostream&;


    struct amorph // cnt
    {
        // time signal(s)
        measurement_count sample_count;
        api::v1::TimeSeries header;
        std::vector<int16_t> order;
        std::vector<file_range> epoch_ranges;
        file_range trigger_range;

        // administrative
        api::v1::Info information;
        api::v1::FileVersion version;
        std::string history;

        // user
        std::vector<user_content> user;

        //std::vector<store> stores;

        amorph();
        amorph(const amorph&) = default;
        amorph(amorph&&) = default;
        ~amorph() = default;

        auto operator=(const amorph&) -> amorph& = default;
        auto operator=(amorph&&) -> amorph& = default;

        friend auto operator==(const amorph&, const amorph&) -> bool = default;
        friend auto operator!=(const amorph&, const amorph&) -> bool = default;
    };
    auto operator<<(std::ostream&, const amorph&) -> std::ostream&;


    struct compressed_epoch
    {
        measurement_count length;
        std::vector<uint8_t> data;

        compressed_epoch(measurement_count, const std::vector<uint8_t>&);
        compressed_epoch();
        compressed_epoch(const compressed_epoch&) = default;
        compressed_epoch(compressed_epoch&&) = default;
        ~compressed_epoch() = default;

        auto operator=(const compressed_epoch&) -> compressed_epoch& = default;
        auto operator=(compressed_epoch&&) -> compressed_epoch& = default;

        friend auto operator==(const compressed_epoch&, const compressed_epoch&) -> bool = default;
        friend auto operator!=(const compressed_epoch&, const compressed_epoch&) -> bool = default;
    };
    auto operator<<(std::ostream&, const compressed_epoch&) -> std::ostream&;


    struct tagged_file
    {
        file_tag id;
        std::filesystem::path file_name;

        tagged_file(file_tag, const std::filesystem::path&);
        tagged_file();
        tagged_file(const tagged_file&) = default;
        tagged_file(tagged_file&&) = default;
        ~tagged_file() = default;

        auto operator=(const tagged_file&) -> tagged_file& = default;
        auto operator=(tagged_file&&) -> tagged_file& = default;
    };
    auto operator<<(std::ostream&, const tagged_file&) -> std::ostream&;



    struct file_token
    {
        tagged_file tag;
        label_type chunk_id;
    };



    class epoch_writer_flat // signal_writer
    {
        struct own_file
        {
            file_ptr f;
            file_token t;
        };

        measurement_count samples;
        measurement_count epoch_size;
        std::chrono::system_clock::time_point start_time; // utc
        std::vector<file_range> epoch_ranges;
        FILE* f_ep;
        FILE* f_data;
        FILE* f_sample_count;
        FILE* f_triggers;
        FILE* f_info;
        FILE* f_history;
        riff_ptr riff;
        std::filesystem::path fname;
        size_t open_files;
        std::vector<own_file> tokens;

        auto add_file(std::filesystem::path, file_tag, std::string) -> FILE*;
        auto close_last() -> void;

    public:

        epoch_writer_flat(const std::filesystem::path& cnt, const api::v1::TimeSeries& x, api::v1::RiffType s);
        epoch_writer_flat(const epoch_writer_flat&) = delete;
        epoch_writer_flat(epoch_writer_flat&&) = default;
        ~epoch_writer_flat() = default;

        auto operator=(const epoch_writer_flat&) -> epoch_writer_flat& = delete;
        auto operator=(epoch_writer_flat&&) -> epoch_writer_flat& = default;

        auto append(const compressed_epoch&) -> void;
        auto append(const api::v1::Trigger&) -> void;
        auto append(const std::vector<api::v1::Trigger>& v) -> void;
        auto info(const api::v1::Info&) -> void;
        auto history(const std::string&) -> void;
        auto flush() -> void;
        auto close() -> void;

        auto file_tokens() const -> std::vector<tagged_file>;
        auto file_name() const -> std::filesystem::path;
        auto epoch_length() const -> measurement_count;
        auto sample_count() const -> measurement_count;
    };



    struct riff_text
    {
        chunk c;
        std::string s;

        riff_text(const chunk&, const std::string&);
        riff_text(const riff_text&) = default;
        riff_text(riff_text&&) = default;
        ~riff_text() = default;

        auto operator=(const riff_text&) -> riff_text& = default;
        auto operator=(riff_text&&) -> riff_text& = default;

        friend auto operator==(const riff_text&, const riff_text&) -> bool = default;
        friend auto operator!=(const riff_text&, const riff_text&) -> bool = default;
    };
    auto content2chunk(FILE*, const riff_text&) -> void;


    struct riff_file
    {
        chunk c;
        std::filesystem::path fname;
        int64_t offset;

        riff_file(const chunk& c, const std::filesystem::path& fname, int64_t offset);
        riff_file(const riff_file&) = default;
        riff_file(riff_file&&) = default;
        ~riff_file() = default;

        auto operator=(const riff_file&) -> riff_file& = default;
        auto operator=(riff_file&&) -> riff_file& = default;

        friend auto operator==(const riff_file&, const riff_file&) -> bool = default;
        friend auto operator!=(const riff_file&, const riff_file&) -> bool = default;
    };
    auto content2chunk(FILE*, const riff_file&) -> void;

    struct riff_list;
    auto content2chunk(FILE*, const riff_list&) -> void;


    class riff_node
    {
        struct abstraction
        {
            virtual ~abstraction() = default;
            virtual auto write(FILE* f) const -> void = 0;
        };

        template<typename T>
        struct model : public abstraction
        {
            T x;
            model(T x) : x{ x } {}
            virtual ~model() = default;

            virtual auto write(FILE* f) const -> void final {
                content2chunk(f, x);
            }
        };

    public:

        template<typename T>
        // T is a type accompanied by a free function with the signature: auto content2chunk(FILE*, const T&) -> void
        riff_node(T x)
        : p{ std::make_shared<model<T>>(x) } {
            assert(p);
        }

        auto write(FILE* f) const -> void {
            p->write(f);
        }

    private:

        std::shared_ptr<const abstraction> p;
    };


    struct riff_list
    {
        chunk c;
        std::vector<riff_node> subnodes;

        riff_list(const chunk&);
        riff_list(const riff_list&) = default;
        riff_list(riff_list&&) = default;
        ~riff_list() = default;

        auto operator=(const riff_list&) -> riff_list& = default;
        auto operator=(riff_list&&) -> riff_list& = default;

        friend auto operator==(const riff_list&, const riff_list&) -> bool = default;
        friend auto operator!=(const riff_list&, const riff_list&) -> bool = default;

        auto push_back(const riff_node& x) -> void;
        auto back() -> decltype(subnodes.back());
    };



    class epoch_reader_common
    {
        FILE* f_data;
        FILE* f_triggers;
        const amorph* data;
        const cnt_field_sizes* riff;

    public:

        epoch_reader_common(FILE*, FILE*, const amorph&, const cnt_field_sizes*, int64_t offset);
        epoch_reader_common(const epoch_reader_common&) = delete;
        epoch_reader_common(epoch_reader_common&&) = default;
        ~epoch_reader_common() = default;

        auto operator=(const epoch_reader_common&) -> epoch_reader_common& = delete;
        auto operator=(epoch_reader_common&&) -> epoch_reader_common& = default;

        auto count() const -> epoch_count;
        auto epoch(epoch_count i) const -> compressed_epoch;
        auto epoch(epoch_count i, std::nothrow_t) const -> compressed_epoch;
        auto has_triggers() const -> bool;
        auto triggers() const -> std::vector<api::v1::Trigger>;
        auto epoch_length() const -> measurement_count;
        auto sample_count() const -> measurement_count;
        auto sampling_frequency() const -> double;
        auto description() const -> api::v1::TimeSeries;
        auto order() const -> std::vector<int16_t>;
        auto channel_count() const -> sensor_count;
        auto channels() const -> std::vector<api::v1::Electrode>;
        auto info_content() const -> std::string;
        auto information() const -> api::v1::Info;
        auto cnt_type() const -> api::v1::RiffType;
        auto file_version() const -> api::v1::FileVersion;
        auto segment_start_time() const -> api::v1::DcDate;
        auto history() const -> std::string;
    };


    class epoch_reader_flat
    {
        std::vector<tagged_file> tokens;
        file_ptr f_data;
        file_ptr f_triggers;
        std::filesystem::path file_name;
        riff_ptr riff;
        amorph a;
        epoch_reader_common common;

        auto get_name(file_tag id) const -> std::filesystem::path;
        auto cnt_type() const -> api::v1::RiffType;
        auto init() -> amorph;

    public:

        epoch_reader_flat(const std::filesystem::path& cnt);
        epoch_reader_flat(const std::filesystem::path& cnt, const std::vector<tagged_file>& available);
        epoch_reader_flat(const epoch_reader_flat&);
        epoch_reader_flat(epoch_reader_flat&&) = default;
        ~epoch_reader_flat() = default;

        //auto operator=(const epoch_reader_flat&) -> epoch_reader_flat&;
        auto operator=(epoch_reader_flat&&) -> epoch_reader_flat& = default;

        auto data() const -> const epoch_reader_common&;
        auto writer_map() const -> riff_list; // reflib
        auto writer_map_extended() const -> riff_list; // extended
        
        auto data_file_name() const -> std::filesystem::path;
        auto trigger_file_name() const -> std::filesystem::path;
        auto ep_file_name() const -> std::filesystem::path;
        auto chan_file_name() const -> std::filesystem::path;
    };


    class epoch_reader_riff
    {
        file_ptr f;
        std::filesystem::path file_name;
        riff_ptr riff;
        amorph a;
        epoch_reader_common common;

        auto cnt_type() const -> api::v1::RiffType;
        auto init() -> amorph;

    public:

        explicit epoch_reader_riff(const std::filesystem::path& cnt);
        epoch_reader_riff(const epoch_reader_riff& x);
        epoch_reader_riff(epoch_reader_riff&&) = default;
        ~epoch_reader_riff() = default;

        //auto operator=(const epoch_reader_riff&) -> epoch_reader_riff&;
        auto operator=(epoch_reader_riff&&) -> epoch_reader_riff& = default;

        auto data() const -> const epoch_reader_common&;
        auto embedded_files() const -> std::vector<std::string>;
        auto extract_embedded_file(const std::string& label, const std::filesystem::path& output) const -> bool;
    };


    auto d2s(double x, int precision) -> std::string;
    auto ascii_sampling_frequency(double) -> std::string;

    auto sex2ch(ctk::api::v1::Sex x) -> uint8_t;
    auto ch2sex(uint8_t x) -> ctk::api::v1::Sex;

    auto hand2ch(ctk::api::v1::Handedness x) -> uint8_t;
    auto ch2hand(uint8_t x) -> ctk::api::v1::Handedness;

    auto make_tm() -> tm; // compatibility: returns 1970-01-01 if not set (instead of 1899-12-30)
    auto timepoint2tm(std::chrono::system_clock::time_point) -> tm;
    auto tm2timepoint(tm) -> std::chrono::system_clock::time_point;

    auto is_root(const chunk& x) -> bool;
    auto is_root_or_list(const chunk& x) -> bool;
    auto root_chunk(api::v1::RiffType t) -> chunk;
    auto list_chunk(api::v1::RiffType t, const std::string& label) -> chunk;
    auto data_chunk(api::v1::RiffType t, const std::string& label) -> chunk;

    auto root_id_riff32() -> std::string;
    auto root_id_riff64() -> std::string;

    auto fname_data(std::filesystem::path) -> std::filesystem::path;
    auto fname_ep(std::filesystem::path) -> std::filesystem::path;
    auto fname_chan(std::filesystem::path) -> std::filesystem::path;
    auto fname_sample_count(std::filesystem::path) -> std::filesystem::path;
    auto fname_electrodes(std::filesystem::path) -> std::filesystem::path;
    auto fname_sampling_frequency(std::filesystem::path) -> std::filesystem::path;
    auto fname_triggers(std::filesystem::path) -> std::filesystem::path;
    auto fname_info(std::filesystem::path) -> std::filesystem::path;
    auto fname_cnt_type(std::filesystem::path) -> std::filesystem::path;
    auto fname_history(std::filesystem::path) -> std::filesystem::path;
    auto fname_time_series_header(std::filesystem::path) -> std::filesystem::path;
    auto fname_flat(std::filesystem::path) -> std::filesystem::path;
    auto delete_files(const std::vector<std::filesystem::path>&) -> bool;

    auto make_eeph_content(const amorph&) -> std::string;
    auto make_info_content(const amorph&) -> std::string;
    auto make_info_content(const api::v1::DcDate&, const api::v1::Info&) -> std::string;
    auto make_electrodes_content(const std::vector<api::v1::Electrode>&) -> std::string;
    auto parse_electrodes(const std::string&, bool libeep) -> std::vector<api::v1::Electrode>;
    auto parse_info(const std::string&) -> std::tuple<api::v1::DcDate, api::v1::Info, bool>;

    auto is_valid(const ctk::api::v1::Electrode&) -> bool;
    auto write_electrodes(FILE*, const std::vector<api::v1::Electrode>&) -> void;
    auto read_electrodes(FILE*) -> std::vector<api::v1::Electrode>;

    auto validate(const api::v1::TimeSeries&) -> void;


} /* namespace impl */ } /* namespace ctk */

