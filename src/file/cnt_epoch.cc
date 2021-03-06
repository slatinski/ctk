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

#include "file/cnt_epoch.h"

#include <cmath>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <chrono>
#include "date/date.h"

#include "compress/matrix.h"
#include "file/leb128.h"
#include "ctk_version.h"
#include "logger.h"


namespace ctk { namespace impl {

    file_range::file_range()
    : fpos{ 0 }
    , size{ 0 } {
    }

    file_range::file_range(int64_t fpos, int64_t size)
    : fpos{ fpos }
    , size{ size } {
    }

    auto operator<<(std::ostream& os, const file_range& x) -> std::ostream& {
        os << "fpos " << x.fpos << ", size " << x.size;
        return os;
    }



    static
    auto field_size(const chunk& x) -> size_t {
        return x.riff->entity_size();
    }

    static
    auto root_id(const chunk& x) -> label_type {
        return as_label(x.riff->root_id());
    }

    static
    auto empty_chunk(const chunk& x) -> chunk {
        chunk empty{ x };
        empty.id = 0;
        empty.label = 0;
        empty.storage = { 0, 0 };
        return empty;
    }

    auto root_chunk(api::v1::RiffType t) -> chunk {
        chunk result{ t };
        result.id = root_id(result);
        result.label = as_label("CNT");
        return result;
    }

    auto list_chunk(api::v1::RiffType t, const std::string& label) -> chunk {
        chunk result{ t };
        result.id = as_label("LIST");
        result.label = as_label(label);
        return result;
    }

    auto data_chunk(api::v1::RiffType t, const std::string& label) -> chunk {
        chunk result{ t };
        result.id = as_label(label);
        return result;
    }

    static
    auto match(label_type l, const char* p) -> bool {
        return l == as_label(p);
    }

    auto is_root(const chunk& x) -> bool {
        return x.id == root_id(x);
    }

    auto is_list(const chunk& x) -> bool {
        return match(x.id, "LIST");
    }

    auto is_root_or_list(const chunk& x) -> bool {
        return is_list(x) || is_root(x);
    }

    static
    auto is_raw3(const chunk& x) -> bool {
        return match(x.label, "raw3");
    }

    static
    auto is_data(const chunk& x) -> bool {
        return match(x.id, "data");
    }

    static
    auto is_chan(const chunk& x) -> bool {
        return match(x.id, "chan");
    }

    static
    auto is_ep(const chunk& x) -> bool {
        return match(x.id, "ep");
    }

    static
    auto is_raw3_chan(const chunk& x, const chunk& parent) -> bool {
        return is_raw3(parent) && is_chan(x);
    }

    static
    auto is_raw3_ep(const chunk& x, const chunk& parent) -> bool {
        return is_raw3(parent) && is_ep(x);
    }

    static
    auto is_raw3_data(const chunk& x, const chunk& parent) -> bool {
        return is_raw3(parent) && is_data(x);
    }

    static
    auto is_eeph(const chunk& x) -> bool {
        return match(x.id, "eeph");
    }

    static
    auto is_evt(const chunk& x) -> bool {
        return match(x.id, "evt");
    }

    static
    auto is_info(const chunk& x) -> bool {
        return match(x.id, "info");
    }

    static
    auto is_average(const chunk& x) -> bool {
        return match(x.label, "rawf");
    }

    static
    auto is_stddev(const chunk& x) -> bool {
        return match(x.label, "stdd");
    }

    static
    auto is_wavelet(const chunk& x) -> bool {
        return match(x.label, "tfd");
    }

    static
    auto label_size() -> int64_t {
        return static_cast<int64_t>(sizeof(label_type));
    }

    static
    auto header_size(const chunk& x) -> int64_t {
        return label_size() + static_cast<int64_t>(field_size(x));
    }

    static
    auto is_even(int64_t x) -> bool {
        return (x & 1) == 0;
    }

    static
    auto make_even(int64_t x) -> int64_t {
        if (is_even(x)) {
            return x;
        }

        return plus(x, int64_t{ 1 }, ok{});
    }

    static
    auto chunk_payload(const chunk& x) -> file_range {
        assert(is_even(x.storage.fpos));

        if (x.storage.size == 0) {
            return { 0, 0 };
        }

        const auto fpos{ x.storage.fpos + header_size(x) };
        return { fpos, x.storage.size };
    }


    static
    auto flat_payload(const std::filesystem::path& fname) -> file_range {
        const int64_t fsize{ content_size(fname) - part_header_size };
        assert(0 <= fsize);
        return { part_header_size, fsize };
    }




    ep_content::ep_content(measurement_count length, const std::vector<int64_t>& offsets)
    : length{ length }
    , offsets{ offsets } {
    }



    static
    auto read_ep_riff(FILE* f, const chunk& ep) -> ep_content {
        return ep.riff->read_ep(f, chunk_payload(ep));
    }


    static
    auto read_ep_flat(const std::filesystem::path& fname, api::v1::RiffType t) -> ep_content {
        const chunk ep{ t };

        file_ptr f{ open_r(fname) };
        return ep.riff->read_ep(f.get(), flat_payload(fname));
    }


    // converts the offsets from the begining of the payload area in the data chunk to absolute file positions
    static
    auto offsets2ranges(const file_range& data, const std::vector<int64_t>& offsets) -> std::vector<file_range> {
        if (data.size < 1 || offsets.empty()) {
            std::ostringstream oss;
            oss << "[offsets2ranges, cnt_epoch] size < 1 || offsets == 0: size " << data.size << ", offsets " << offsets.size();
            const auto e{ oss.str() };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        std::vector<file_range> ranges(offsets.size());
        const size_t border{ offsets.size() - 1 };

        for (size_t i{ 0 }; i < border; ++i) {
            if (offsets[i + 1] <= offsets[i]) {
                std::ostringstream oss;
                oss << "[offsets2ranges, cnt_epoch] invalid compressed size " << (offsets[i + 1] - offsets[i]) << ", epoch " << i;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw api::v1::CtkData{ e };
            }

            const auto fpos{ data.fpos + offsets[i] };
            const auto length{ offsets[i + 1] - offsets[i] };
            if (data.size < length) {
                std::ostringstream oss;
                oss << "[offsets2ranges, cnt_epoch] epoch " << i
                    << " exceeds the size of the data chunk: " << data.size
                    << " < " << length;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw api::v1::CtkData{ e };
            }

            ranges[i] = { fpos, length };
        }

        if (data.size <= offsets[border]) {
            std::ostringstream oss;
            oss << "[offsets2ranges, cnt_epoch] epoch " << border
                << " empty or exceeds the size of the data chunk: " << data.size
                << " <= " << offsets[border];
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
        ranges[border] = { data.fpos + offsets[border], data.size - offsets[border] };

        return ranges;
    }


    template<typename T>
    auto read_ep_content(FILE* f, const file_range& x, T type_tag) -> ep_content {
        const size_t items{ cast(x.size, size_t{}, guarded{}) / sizeof(T) };
        if (items < 2) {
            const std::string e{ "[read_ep_content, cnt_epoch] no compressed epoch data" };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        if (!seek(f, x.fpos, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[read_ep_content, cnt_epoch] can not seek to file position " << x.fpos;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        const T l{ read(f, type_tag) };
        const measurement_count epoch_length{ cast(l, measurement_count::value_type{}, ok{}) };

        std::vector<T> xs(items - 1 /* epoch length */);
        read(f, begin(xs), end(xs));

        std::vector<int64_t> offsets(xs.size());
        std::transform(begin(xs), end(xs), begin(offsets), [](T y) -> int64_t { return cast(y, int64_t{}, ok{}); });
        return { epoch_length, offsets };
    }


    template<typename I>
    auto as_code(I first, I last) -> std::array<char, api::v1::sizes::evt_trigger_code> {
        std::array<char, api::v1::sizes::evt_trigger_code> result{ 0 };
        const auto size_input{ std::distance(first, last) };
        const auto size_label{ static_cast<ptrdiff_t>(api::v1::sizes::evt_trigger_code) };
        const auto amount{ std::min(size_input, size_label) };
        auto next{ std::copy(first, first + amount, begin(result)) };
        if (next != end(result)) {
            *next = 0;
        }
        return result;
    }

    static
    auto as_code(const std::string& xs) -> std::array<char, api::v1::sizes::evt_trigger_code> {
        return as_code(begin(xs), end(xs));
    }

    static
    auto as_string(const std::array<char, api::v1::sizes::evt_trigger_code>& xs) -> std::string {
        const auto first{ begin(xs) };
        const auto last{ end(xs) };
        const auto found{ std::find(first, last, 0) };
        const auto length{ static_cast<size_t>(std::distance(first, found)) };
        const auto amount{ std::min(length, static_cast<size_t>(api::v1::sizes::evt_trigger_code)) };

        std::string result;
        result.resize(amount);
        std::copy(first, first + amount, begin(result));
        return result;
    }

    template<typename T>
    auto read_evt_content(FILE* f, const file_range& x, T type_tag) -> std::vector<api::v1::Trigger> {
        const size_t items{ cast(x.size, size_t{}, guarded{}) / (api::v1::sizes::evt_trigger_code + sizeof(T)) };

        if (!seek(f, x.fpos, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[read_evt_content, cnt_epoch] can not seek to file position " << x.fpos;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        std::vector<api::v1::Trigger> result;
        result.reserve(items);

        int64_t sample;
        std::array<char, api::v1::sizes::evt_trigger_code> code;

        for (size_t i{ 0 }; i < items; ++i) {
            sample = cast(read(f, type_tag), int64_t{}, ok{});
            read(f, begin(code), end(code));

            result.emplace_back(sample, as_string(code));
        }

        return result;
    }

    template<typename T>
    auto write_evt_record(FILE* f, const api::v1::Trigger& x, T type_tag) -> void {
        assert(f);

        write(f, cast(x.Sample, type_tag, ok{}));

        const auto code{ as_code(x.Code) };
        const auto first{ begin(code) };
        const auto last{ end(code) };
        write(f, first, last);
    }


    template<typename T>
    auto write_evt_content(FILE* f, const std::vector<api::v1::Trigger>& xs, T type_tag) -> void {
        for (const auto& x : xs) {
            write_evt_record(f, x, type_tag);
        }
    }


    template<typename SizeType, typename EvtType>
    struct riff_type : public cnt_field_sizes
    {
        label_type id; // RIFF/RF64

        explicit
        riff_type(const std::string& s)
        : id{ as_label(s) } {
        }

        virtual
        ~riff_type() = default;

        virtual
        auto clone() -> cnt_field_sizes* override final {
            return new riff_type{ as_string(id) };
        }

        virtual
        auto root_id() const -> std::string override final {
            return as_string(id);
        }

        virtual
        auto entity_size() const -> size_t override final {
            return sizeof(SizeType);
        }

        virtual
        auto write_entity(FILE* f, int64_t x) const -> void override final {
            write(f, cast(x, SizeType{}, ok{}));
        }

        virtual
        auto maybe_read_entity(FILE* f) const -> std::optional<int64_t> override final {
            const auto maybe_x{ maybe_read(f, SizeType{}) };
            if (!maybe_x) {
                return std::nullopt;
            }

            return maybe_cast(*maybe_x, int64_t{});
        }

        virtual
        auto read_ep(FILE* f, const file_range& x) const -> ep_content override final {
            return read_ep_content(f, x, SizeType{});
        }

        virtual
        auto read_triggers(FILE* f, const file_range& x) const -> std::vector<api::v1::Trigger> override final {
            return read_evt_content(f, x, EvtType{});
        }

        virtual
        auto write_triggers(FILE* f, const std::vector<api::v1::Trigger>& xs) const -> void override final {
            return write_evt_content(f, xs, EvtType{});
        }

        virtual
        auto write_trigger(FILE* f, const api::v1::Trigger& x) const -> void override final {
            return write_evt_record(f, x, EvtType{});
        }
    };

    auto root_id_riff32() -> std::string {
        return "RIFF";
    }

    auto root_id_riff64() -> std::string {
        return "RF64";
    }


    static
    auto string2riff(const std::string& x) -> ctk::api::v1::RiffType {
        if (x == root_id_riff32()) {
            return ctk::api::v1::RiffType::riff32;
        }
        else if (x == root_id_riff64()) {
            return ctk::api::v1::RiffType::riff64;
        }

        std::ostringstream oss;
        oss << "[string2riff, cnt_epoch] unknown type " << x
            << ", expected " << root_id_riff32()
            << " or " << root_id_riff64();
        const auto e{ oss.str() };
        ctk_log_error(e);
        throw api::v1::CtkData{ e };
    }

    static
    auto make_cnt_field_size(api::v1::RiffType x) -> riff_ptr {
        switch(x) {
            case api::v1::RiffType::riff32: {
                using size_type = uint32_t;
                using evt_type = int32_t;
                using riff32_type = riff_type<size_type, evt_type>;
                return std::make_unique<riff32_type>(root_id_riff32());
            }
            case api::v1::RiffType::riff64: {
                using size_type = uint64_t;
                using evt_type = uint64_t;
                using riff64_type = riff_type<size_type, evt_type>;
                return std::make_unique<riff64_type>(root_id_riff64());
            }
            default: break;
        }

        std::ostringstream oss;
        oss << "[make_cnt_field_size, cnt_epoch] unknown type " << x
            << ", expected " << api::v1::RiffType::riff32
            << " or " << api::v1::RiffType::riff64;
        const auto e{ oss.str() };
        ctk_log_critical(e);
        throw api::v1::CtkBug{ e };
    }


    chunk::chunk(api::v1::RiffType t)
    : id{ 0 }
    , label{ 0 }
    , riff{ make_cnt_field_size(t) } {
    }

    chunk::chunk(const chunk& x)
    : id{ x.id }
    , label{ x.label }
    , riff{ x.riff->clone() }
    , storage{ x.storage } {
    }

    auto chunk::operator=(const chunk& x) -> chunk& {
        chunk y{ x };
        swap(*this, y);
        return *this;
    }

    auto swap(chunk& x, chunk& y) -> void {
        using std::swap;

        swap(x.riff, y.riff);
        swap(x.id, y.id);
        swap(x.label, y.label);
        swap(x.storage, y.storage);
    }

    auto operator==(const chunk& x, const chunk& y) -> bool {
        return x.riff->root_id() == y.riff->root_id() && x.id == y.id && x.label == y.label && x.storage == y.storage;
    }

    auto operator!=(const chunk& x, const chunk& y) -> bool {
        return !(x == y);
    }

    auto operator<<(std::ostream& os, const chunk& x) -> std::ostream& {
        os << "id " << as_string(x.id)
           << ", label " << as_string(x.label)
           << " (" << string2riff(x.riff->root_id())
           << ", storage " << x.storage << ")";
        return os;
    }



    user_content::user_content(const std::string& label, const file_range& storage)
    : label{ label }
    , storage{ storage } {
    }

    auto operator<<(std::ostream& os, const user_content& x) -> std::ostream& {
        os << x.label << ": " << x.storage;
        return os;
    }

    amorph::amorph()
    : sample_count{ 0 } {
    }

    compressed_epoch::compressed_epoch()
    : length{ 0 } {
    }

    compressed_epoch::compressed_epoch(measurement_count length, const std::vector<uint8_t>& data)
    : length{ length }
    , data{ data } {
    }

    auto operator<<(std::ostream& os, const compressed_epoch&) -> std::ostream& {
        return os;
    }


    tagged_file::tagged_file()
    : id{ file_tag::length /* invalid */} {
    }

    tagged_file::tagged_file(file_tag id, const std::filesystem::path& file_name)
    : id{ id }
    , file_name{ file_name } {
    }

    auto operator<<(std::ostream& os, const tagged_file& x) -> std::ostream& {
        os << x.id << ": " << x.file_name.string();
        return os;
    }



    static
    auto offsets2ranges_riff(const chunk& raw3_data, const std::vector<int64_t>& offsets) -> std::vector<file_range> {
        return offsets2ranges(chunk_payload(raw3_data), offsets);
    }

    static
    auto offsets2ranges_flat(const std::filesystem::path& data, const std::vector<int64_t>& offsets) -> std::vector<file_range> {
        return offsets2ranges(flat_payload(data), offsets);
    }


    static
    auto maybe_read_chunk(FILE* f, chunk scratch) -> std::optional<chunk> {
        const int64_t maybe_fpos{ maybe_tell(f) };
        if (maybe_fpos < 0) {
            return std::nullopt;
        }
        scratch.storage.fpos = maybe_fpos;

        const auto maybe_id = maybe_read(f, label_type{});
        if (!maybe_id) {
            return std::nullopt;
        }
        scratch.id = *maybe_id;

        const auto maybe_size{ scratch.riff->maybe_read_entity(f) };
        if (!maybe_size) {
            return std::nullopt;
        }
        scratch.storage.size = *maybe_size;

        if (is_root_or_list(scratch)) {
            const auto maybe_label = maybe_read(f, label_type{});
            if (!maybe_label) {
                return std::nullopt;
            }
            scratch.label = *maybe_label;
        }
        else {
            scratch.label = as_label("");
        }

        assert(is_even(scratch.storage.fpos));
        return scratch;
    }


    static
    auto read_root(FILE* f, api::v1::RiffType t) -> chunk {
        const auto maybe_chunk{ maybe_read_chunk(f, chunk{ t }) };
        if (!maybe_chunk) {
            std::ostringstream oss;
            oss << "[read_root, cnt_epoch] can not read chunk of type " << t;
            ctk_log_error(oss.str());
            throw api::v1::CtkData{ oss.str() };
        }

        return *maybe_chunk;
    }


    static
    auto parse_int(const std::string& line) -> int64_t {
        if (line.empty()) {
            const std::string e{ "[parse_int, cnt_epoch] no input" };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
        return std::stoll(line);
    }


    static
    auto parse_double(const std::string& line) -> double {
        if (line.empty()) {
            const std::string e{ "[parse_double, cnt_epoch] no input" };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        const auto result{ std::stod(line) };
        if (!std::isfinite(result)) {
            std::ostringstream oss;
            oss << "[parse_double, cnt_epoch] infinite " << result;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        return result;
    }

    static
    auto ascii_parseable(double x) -> bool {
        std::ostringstream oss;
        oss << x;
        const auto d{ parse_double(oss.str()) };
        return d == x;
    }

    auto d2s(double x, int p) -> std::string {
        std::ostringstream oss;
        // TODO: use scientific to prevent locale problems?
        oss << std::setprecision(p) << x;
        return oss.str();
    }

    auto ascii_sampling_frequency(double x) -> std::string {
        // TODO: understand the limits of buggy roundtrips
        //
        // std::string x = "255.9949866127";
        // double num = std::stod(x);
        // std::ostringstream os;
        // os << std::setprecision(11) << num;
        // std::string y = os.str();
        // x "255.9949866127"
        // y "255.99498661"   (setprecision(11))
        // y "255.994986613"  (setprecision(12))
        // y "255.9949866127" (setprecision(13))
        return d2s(x, 13);
    }


    static
    auto load_line(const std::string &input, size_t i, size_t length) -> std::pair<std::string, size_t> {
        if (input.size() <= i) {
            return { "", std::string::npos };
        }

        const auto eol{ input.find('\n', i) };
        const auto size{ std::min(std::min(eol - i, length - 1/* \n */), input.size() - i) };
        const auto next{ i + size + 1/* \n */ };
        return { input.substr(i, size), next };
    }


    static
    auto optional_electrode_field(const std::array<std::string, 3>& x, std::string key) -> std::string {
        for (const auto& line : x) {
            const auto i{ line.find(key) };
            if (i == std::string::npos || i != 0) {
                continue;
            }
            assert(key.size() <= line.size());

            const auto key_size{ key.size() };
            const auto length{ std::min(line.size() - key_size, size_t{ api::v1::sizes::eeph_electrode_status }) }; // compatibility
            return line.substr(i + key_size, length);
        }

        return "";
    }


    auto parse_electrodes(const std::string& input, bool libeep) -> std::vector<api::v1::Electrode> {
        constexpr const size_t length{ 128 };
        auto[line, i]{ load_line(input, 0, length) };

        api::v1::Electrode e;
        std::vector<api::v1::Electrode> electrodes;

        while (!line.empty() && line[0] != '[') {
            if (line[0] == ';') {
                std::tie(line, i) = load_line(input, i, length);
                continue;
            }

            std::string active, unit;
            double iscale, rscale;

            std::istringstream iss{ line };
            iss >> active >> iscale >> rscale >> unit;
            if (iss.fail() || active.empty() || unit.empty()) {
                std::string msg;
                if (iss.fail()) {
                    std::ostringstream oss;
                    oss << "[parse_electrodes] can not read (label iscale rscale unit) from (" << line << ")";
                    msg = oss.str();
                }
                else if(active.empty()) {
                    msg = "[parse_electrodes] empty active channel label";
                }
                else if(unit.empty()) {
                    msg = "[parse_electrodes] empty unit";
                }
                ctk_log_error(msg);
                throw api::v1::CtkData{ msg };
            }
            // compatibility
            active.resize(std::min(active.size(), size_t{ api::v1::sizes::eeph_electrode_active }));
            unit.resize(std::min(unit.size(), size_t{ api::v1::sizes::eeph_electrode_unit }));

            std::array<std::string, 3> nonobligatory;
            iss >> nonobligatory[0] >> nonobligatory[1] >> nonobligatory[2];

            e.ActiveLabel = active;
            e.Unit = unit;
            e.Reference = optional_electrode_field(nonobligatory, "REF:");
            e.Status = optional_electrode_field(nonobligatory, "STAT:");
            e.Type = optional_electrode_field(nonobligatory, "TYPE:");
            e.IScale = iscale;
            e.RScale = rscale;

            // compatibility
            if (libeep) {
                /* No (valid) label but exactly 5 columns enables    */
                /*   workaround for old files: it must be a reflabel */
                if (e.Reference.empty() && !nonobligatory[0].empty() && nonobligatory[1].empty() && nonobligatory[2].empty()) {
                    e.Reference = nonobligatory[0];
                    e.Reference.resize(std::min(e.Reference.size(), size_t{ api::v1::sizes::eeph_electrode_reference }));
                }
            }

            electrodes.push_back(e);

            std::tie(line, i) = load_line(input, i, length);
        }

        return electrodes;
    }


    static
    auto clock_guard(const date::year_month_day& x) -> bool {
        using namespace std::chrono;
        using dsecs = date::sys_time<duration<double>>;
        using Precision = system_clock::duration;

        constexpr const system_clock::time_point dmin{ date::sys_time<Precision>::min() };
        constexpr const system_clock::time_point dmax{ date::sys_time<Precision>::max() };
        const auto y{ date::sys_days{ x } };
        return dsecs{ dmin } <= y && y <= dsecs{ dmax };
    }


    static
    auto in_clock_range(int year, unsigned month, unsigned day) -> bool {
        const date::year yyyy{ year };
        const date::month mm{ month };
        const date::day dd{ day };
        if (!yyyy.ok() || !mm.ok() || !dd.ok()) {
            return false;
        }

        const auto ymd{ yyyy/mm/dd };
        if (!ymd.ok()) {
            return false;
        }

        return clock_guard(ymd);
    }


    static
    auto print_tm(const tm& x) -> std::string {
        std::ostringstream oss;
        oss << x.tm_year << "/" << x.tm_mon << "/" << x.tm_mday << " " << x.tm_hour << ":" << x.tm_min << ":" << x.tm_sec;
        return oss.str();
    }


    static
    auto is_valid(const tm& x) -> std::pair<bool, std::string> {
        constexpr const int min_year{ static_cast<int>(date::year::min()) };
        constexpr const int max_year{ static_cast<int>(date::year::max()) };
        const auto [year, status]{ signed_addition(x.tm_year, 1900) };
        if (status != arithmetic_error::none) {
            std::ostringstream oss;
            oss << "[is_valid tm, cnt_epoch]" << print_tm(x) << ": can not add 1900 to " << x.tm_year;
            const auto e{ oss.str() };
            return { false, e };
        }

        if (year < min_year  || max_year < year) {
            std::ostringstream oss;
            oss << "[is_valid tm, cnt_epoch]" << print_tm(x) << ": invalid year " << year << " (min " << min_year << ", max " << max_year << ")";
            const auto e{ oss.str() };
            return { false, e };
        }

        if (x.tm_mon < 0 || 11 < x.tm_mon) {
            std::ostringstream oss;
            oss << "[is_valid tm, cnt_epoch]" << print_tm(x) << ": invalid month " << x.tm_mon << " (min 0, max 11)";
            const auto e{ oss.str() };
            return { false, e };
        }

        if (x.tm_mday < 1 || 31 < x.tm_mday) {
            std::ostringstream oss;
            oss << "[is_valid tm, cnt_epoch]" << print_tm(x) << ": invalid day " << x.tm_mday << " (min 1, max 31)";
            const auto e{ oss.str() };
            return { false, e };
        }

        if (x.tm_hour < 0 || 23 < x.tm_hour) {
            std::ostringstream oss;
            oss << "[is_valid tm, cnt_epoch]" << print_tm(x) << ": invalid hour " << x.tm_hour << " (min 0, max 23)";
            const auto e{ oss.str() };
            return { false, e };
        }

        if (x.tm_min < 0 || 59 < x.tm_min) {
            std::ostringstream oss;
            oss << "[is_valid tm, cnt_epoch]" << print_tm(x) << ": invalid minute " << x.tm_min << " (min 0, max 59)";
            const auto e{ oss.str() };
            return { false, e };
        }

        if (x.tm_sec < 0 || 60 < x.tm_sec) {
            std::ostringstream oss;
            oss << "[is_valid tm, cnt_epoch]" << print_tm(x) << ": invalid second " << x.tm_sec << " (min 0, max 60)";
            const auto e{ oss.str() };
            return { false, e };
        }

        return { true, "" };
    }

    auto validate(const tm& x) -> void {
        const auto[valid, error]{ is_valid(x) };
        if (!valid) {
            ctk_log_error(error);
            throw api::v1::CtkData{ error };
        }
    }


    auto make_tm() -> tm {
        constexpr const time_t t{ 0 };
        const tm* x{ gmtime(&t) };
        if (!x) {
            const std::string e{ "[make_tm, cnt_epoch] can not invoke gmtime(0)" };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }
        tm y{ *x };

        y.tm_isdst = -1;
        validate(y);
        return y;
    }


    auto timepoint2tm(std::chrono::system_clock::time_point x) -> tm {
        using namespace std::chrono;

        const seconds x_s{ floor<seconds>(x.time_since_epoch()) };
        const days x_days{ floor<days>(x_s) };
        const date::year_month_day ymd{ date::sys_days{ x_days } };
        if (!ymd.ok()) {
            const auto yyyy{ static_cast<int>(ymd.year()) };
            const auto mm{ static_cast<unsigned>(ymd.month()) };
            const auto dd{ static_cast<unsigned>(ymd.day()) };
            std::ostringstream oss;
            oss << "[timepoint2tm, cnt_epoch] invalid date " << yyyy << "/" << mm << "/" << dd;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        seconds reminder{ x_s - x_days };
        const hours h{ floor<hours>(reminder) };
        reminder -= h;

        const minutes m{ floor<minutes>(reminder) };
        reminder -= m;

        const seconds s{ reminder };
        if (h < 0h || 23h < h || m < 0min || 59min < m || s < 0s || 59s < s) {
            std::ostringstream oss;
            oss << "[timepoint2tm, cnt_epoch] invalid time "
                << h.count() << ":" << m.count() << ":" << s.count()
                << ", valid hours 0-23, minutes 0-59, seconds 0-59";
            const auto e{ oss.str() };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        tm y{ make_tm() };
        y.tm_year = minus(static_cast<int>(ymd.year()), 1900, ok{});
        y.tm_mon = static_cast<int>(static_cast<unsigned>(ymd.month()) - 1U);
        y.tm_mday = static_cast<int>(static_cast<unsigned>(ymd.day()));
        y.tm_hour = static_cast<int>(h.count());
        y.tm_min = static_cast<int>(m.count());
        y.tm_sec = static_cast<int>(s.count());
        validate(y);
        return y;
    }

    auto tm2timepoint(tm x) -> std::chrono::system_clock::time_point {
        using namespace std::chrono;

        validate(x);
        const int sy{ plus(x.tm_year, 1900, ok{}) };
        const int sm{ plus(x.tm_mon, 1, ok{}) };
        if (!in_clock_range(sy, cast(sm, unsigned{}, ok{}), cast(x.tm_mday, unsigned{}, ok{}))) {
            std::ostringstream oss;
            oss << "[tm2timepoint, cnt_epoch] not in system clock range " << print_tm(x);
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        const auto yyyy{ date::year{ sy } };
        const auto mm{ date::month{ cast(sm, unsigned{}, ok{}) } };
        const auto dd{ date::day{ cast(x.tm_mday, unsigned{}, ok{}) } };
        system_clock::time_point days{ date::sys_days{ yyyy/mm/dd } };

        const hours h{ x.tm_hour };
        const minutes m{ x.tm_min };
        const seconds s{ x.tm_sec };
        return days + h + m + s;
    }


    static
    auto parse_info_dob(const std::string &line) -> std::chrono::system_clock::time_point {
        tm x{ make_tm() };
        if (line.empty()) {
            return tm2timepoint(x);
        }

        std::istringstream iss{ line };
        iss >> x.tm_sec >> x.tm_min >> x.tm_hour >> x.tm_mday >> x.tm_mon >> x.tm_year >> x.tm_wday >> x.tm_yday >> x.tm_isdst;

        if (iss.fail()) {
            ctk_log_warning("[parse_info_dob, cnt_epoch] not all expected fileds are present");
        }

        try {
            return tm2timepoint(x);
        }
        catch(const api::v1::CtkData&) {
            ctk_log_warning("[parse_info_dob, cnt_epoch] invalid date of birth. replacing with default");
            return tm2timepoint(make_tm());
        }
    }

    auto parse_info(const std::string& input) -> std::tuple<api::v1::DcDate, api::v1::Info, bool> {
        api::v1::DcDate start_time;
        api::v1::Info information;
        bool is_ascii{ false };

        constexpr const size_t length{ 256 };
        auto[line, i]{ load_line(input, 0, length) };

        while(i < input.size()) {
            if (line.find("[StartDate]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                start_time.Date = parse_double(line);
                is_ascii = true;
            }
            else if (line.find("[StartFraction]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                start_time.Fraction = parse_double(line);
            }
            else if (line.find("[Hospital]") != std::string::npos) {
                std::tie(information.Hospital, i) = load_line(input, i, length);
            }
            else if (line.find("[TestName]") != std::string::npos) {
                std::tie(information.TestName, i) = load_line(input, i, length);
            }
            else if (line.find("[TestSerial]") != std::string::npos) {
                std::tie(information.TestSerial, i) = load_line(input, i, length);
            }
            else if (line.find("[Physician]") != std::string::npos) {
                std::tie(information.Physician, i) = load_line(input, i, length);
            }
            else if (line.find("[Technician]") != std::string::npos) {
                std::tie(information.Technician, i) = load_line(input, i, length);
            }
            else if (line.find("[MachineMake]") != std::string::npos) {
                std::tie(information.MachineMake, i) = load_line(input, i, length);
            }
            else if (line.find("[MachineModel]") != std::string::npos) {
                std::tie(information.MachineModel, i) = load_line(input, i, length);
            }
            else if (line.find("[MachineSN]") != std::string::npos) {
                std::tie(information.MachineSn, i) = load_line(input, i, length);
            }
            else if (line.find("[SubjectName]") != std::string::npos) {
                std::tie(information.SubjectName, i) = load_line(input, i, length);
            }
            else if (line.find("[SubjectID]") != std::string::npos) {
                std::tie(information.SubjectId, i) = load_line(input, i, length);
            }
            else if (line.find("[SubjectAddress]") != std::string::npos) {
                std::tie(information.SubjectAddress, i) = load_line(input, i, length);
            }
            else if (line.find("[SubjectPhone]") != std::string::npos) {
                std::tie(information.SubjectPhone, i) = load_line(input, i, length);
            }
            else if (line.find("[SubjectSex]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                if (!line.empty()) {
                    information.SubjectSex = char2sex(static_cast<uint8_t>(line[0]));
                }
            }
            else if (line.find("[SubjectHandedness]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                if (!line.empty()) {
                    information.SubjectHandedness = char2hand(static_cast<uint8_t>(line[0]));
                }
            }
            else if (line.find("[SubjectDateOfBirth]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                information.SubjectDob = parse_info_dob(line);
            }
            else if (line.find("[Comment]") != std::string::npos) {
                std::tie(information.Comment, i) = load_line(input, i, length);
            }

            std::tie(line, i) = load_line(input, i, length);
        }

        return { start_time, information, is_ascii };
    }


    struct eeph_data
    {
        double sampling_frequency;
        measurement_count sample_count;
        sensor_count channel_count;
        std::vector<api::v1::Electrode> electrodes;
        api::v1::FileVersion version;
        std::string history;

        eeph_data()
        : sampling_frequency{ 0 }
        , sample_count{ 0 }
        , channel_count{ 0 } {
        }
    };

    static
    auto read_history(const std::string& input) -> std::string {
        std::ostringstream oss;
        constexpr const size_t length{ 2048 };
        auto[line, i]{ load_line(input, 0, length) };

        while (i < input.size() && line != "EOH") {
            oss << line << '\n';
            std::tie(line, i) = load_line(input, i, length);
        }
        return oss.str();
    }


    static
    auto parse_eeph(const std::string& input) -> eeph_data {
        eeph_data result;
        constexpr const size_t length{ 2048 };
        auto[line, i]{ load_line(input, 0, length) };

        while(i < input.size()) {
            if (line.empty()) {
                // compatibility: the value of sections like [Averaged Trials] might be represented as the empty string
                std::tie(line, i) = load_line(input, i, length);
            }

            if (line.find("[Samples]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                result.sample_count = measurement_count{ cast(parse_int(line), measurement_count::value_type{}, ok{}) };
            }
            else if (line.find("[Sampling Rate]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                result.sampling_frequency = parse_double(line);
            }
            else if (line.find("[Basic Channel Data]") != std::string::npos) {
                result.electrodes = parse_electrodes(input.substr(i), result.version.Major < 4);
                i = input.find('[', i + 1);
            }
            else if (line.find("[Channels]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);
                result.channel_count = sensor_count{ cast(parse_int(line), sensor_count::value_type{}, ok{}) };
            }
            else if (line.find("[File Version]") != std::string::npos) {
                std::tie(line, i) = load_line(input, i, length);

                std::istringstream iss{ line };
                char delim;
                uint32_t major, minor;
                iss >> major >> delim >> minor;
                if (!iss.fail() && delim == '.') {
                    result.version = { major, minor };
                }
            }
            else if (line.find("[History]") != std::string::npos) {
                result.history = read_history(input.substr(i));
                i = input.find('[', i + 1);
            }

            std::tie(line, i) = load_line(input, i, length);
        }

        return result;
    }



    static
    auto read_eeph_riff(FILE* f, const chunk& eeph) -> eeph_data {
        const auto x{ chunk_payload(eeph) };
        if (x.size == 0) {
            return eeph_data{};
        }

        if (!seek(f, x.fpos, SEEK_SET)) {
            return eeph_data{};
        }

        std::string data;
        data.resize(static_cast<size_t>(x.size));
        read(f, begin(data), end(data));

        return parse_eeph(data);
    }


    auto call_parse_eeph(const std::string& xs) -> void {
        parse_eeph(xs);
    }


    static
    auto truncate(const std::string& s, size_t length) -> std::string {
        if (length < s.size()) {
            return s.substr(0, length);
        }

        return s;
    }


    static
    auto write_string_bin(FILE* f, const std::string& xs) -> void {
        const int64_t size{ cast(xs.size(), int64_t{}, ok{}) };
        write_leb128(f, size);
        write(f, begin(xs), end(xs));
    }

    static
    auto read_string_bin(FILE* f) -> std::string {
        const int64_t size{ read_leb128(f, int64_t{}) };
        const size_t count{ cast(size, size_t{}, ok{}) };

        std::string xs;
        xs.resize(count);
        read(f, begin(xs), end(xs));
        return xs;
    }

    static
    auto write_electrode_bin(FILE* f, const api::v1::Electrode& x) -> void {
        write_string_bin(f, x.ActiveLabel);
        write_string_bin(f, x.Reference);
        write_string_bin(f, x.Unit);
        write_string_bin(f, x.Status);
        write_string_bin(f, x.Type);
        write(f, x.IScale);
        write(f, x.RScale);
    }

    static
    auto read_electrode_bin(FILE* f) -> api::v1::Electrode {
        api::v1::Electrode x;
        x.ActiveLabel = read_string_bin(f);
        x.Reference = read_string_bin(f);
        x.Unit = read_string_bin(f);
        x.Status = read_string_bin(f);
        x.Type = read_string_bin(f);
        x.IScale = read(f, double{});
        x.RScale = read(f, double{});
        return x;
    }


    auto write_electrodes_bin(FILE* f, const std::vector<api::v1::Electrode>& xs) -> void {
        const int64_t size{ cast(xs.size(), int64_t{}, ok{}) };
        write_leb128(f, size);

        for (const auto& x : xs) {
            write_electrode_bin(f, x);
        }
    }

    auto read_electrodes_bin(FILE* f) -> std::vector<api::v1::Electrode> {
        const int64_t size{ read_leb128(f, int64_t{}) };
        const size_t count{ cast(size, size_t{}, ok{}) };

        std::vector<api::v1::Electrode> xs;
        xs.reserve(count);
        for (size_t i{ 0 }; i < count; ++i) {
            xs.push_back(read_electrode_bin(f));
        }
        return xs;
    }


    static
    auto read_electrodes_flat(const std::filesystem::path& fname) -> std::vector<api::v1::Electrode> {
        file_ptr f{ open_r(fname) };
        skip_part_header(f.get());

        return read_electrodes_bin(f.get());
    }

    auto make_electrodes_content(const std::vector<api::v1::Electrode>& electrodes) -> std::string {
        std::ostringstream oss;

        for (const auto& e : electrodes) {
            oss << truncate(e.ActiveLabel, api::v1::sizes::eeph_electrode_active) << " ";
            oss << d2s(e.IScale, 11) << " ";
            oss << d2s(e.RScale, 11) << " ";
            oss << truncate(e.Unit, api::v1::sizes::eeph_electrode_unit);
            if (!e.Reference.empty()) {
                oss << " REF:" << truncate(e.Reference, api::v1::sizes::eeph_electrode_reference);
            }
            if (!e.Status.empty()) {
                oss << " STAT:" << truncate(e.Status, api::v1::sizes::eeph_electrode_status);
            }
            if (!e.Type.empty()) {
                oss << " TYPE:" << truncate(e.Type, api::v1::sizes::eeph_electrode_type);
            }
            oss << "\n";
        }

        return oss.str();
    }


    auto make_eeph_content(const amorph& data) -> std::string {
        std::ostringstream oss;

        oss << "[File Version]\n" << CTK_FILE_VERSION_MAJOR << "." << CTK_FILE_VERSION_MINOR << "\n";
        oss << "[Sampling Rate]\n" << ascii_sampling_frequency(data.header.SamplingFrequency) << "\n";
        oss << "[Samples]\n" << data.sample_count << "\n";
        oss << "[Channels]\n" << data.header.Electrodes.size() << "\n";
        oss << "[Basic Channel Data]\n";
        oss << make_electrodes_content(data.header.Electrodes);
        oss << "[History]\n" << data.history << "\nEOH\n";

        return oss.str();
    }


    auto make_info_content(const api::v1::DcDate& x, const api::v1::Info& i) -> std::string {
        const size_t length{ 512 };
        std::ostringstream oss;
        oss << "[StartDate]\n" << d2s(x.Date, 21) << "\n";
        oss << "[StartFraction]\n" << d2s(x.Fraction, 21) << "\n";

        if (!i.Hospital.empty()) { oss << "[Hospital]\n" << truncate(i.Hospital, length) << "\n"; }
        if (!i.TestName.empty()) { oss << "[TestName]\n" << truncate(i.TestName, length) << "\n"; }
        if (!i.TestSerial.empty()) { oss << "[TestSerial]\n" << truncate(i.TestSerial, length) << "\n"; }
        if (!i.Physician.empty()) { oss << "[Physician]\n" << truncate(i.Physician, length) << "\n"; }
        if (!i.Technician.empty()) { oss << "[Technician]\n" << truncate(i.Technician, length) << "\n"; }
        if (!i.MachineMake.empty()) { oss << "[MachineMake]\n" << truncate(i.MachineMake, length) << "\n"; }
        if (!i.MachineModel.empty()) { oss << "[MachineModel]\n" << truncate(i.MachineModel, length) << "\n"; }
        if (!i.MachineSn.empty()) { oss << "[MachineSN]\n" << truncate(i.MachineSn, length) << "\n"; }
        if (!i.SubjectName.empty()) { oss << "[SubjectName]\n" << truncate(i.SubjectName, length) << "\n"; }
        if (!i.SubjectId.empty()) { oss << "[SubjectID]\n" << truncate(i.SubjectId, length) << "\n"; }
        if (!i.SubjectAddress.empty()) { oss << "[SubjectAddress]\n" << truncate(i.SubjectAddress, length) << "\n"; }
        if (!i.SubjectPhone.empty()) { oss << "[SubjectPhone]\n" << truncate(i.SubjectPhone, length) << "\n"; }
        if (i.SubjectSex != api::v1::Sex::Unknown) { oss << "[SubjectSex]\n" << sex2char(i.SubjectSex) << "\n"; }
        if (i.SubjectDob != tm2timepoint(make_tm())) {
            const tm dob{ timepoint2tm(i.SubjectDob) };

            oss << "[SubjectDateOfBirth]\n" << dob.tm_sec << " " << dob.tm_min << " " << dob.tm_hour << " " << dob.tm_mday << " " << dob.tm_mon << " " << dob.tm_year << " " << dob.tm_wday << " " << dob.tm_yday << " " << dob.tm_isdst << "\n";

        }

        if (i.SubjectHandedness != api::v1::Handedness::Unknown) { oss << "[SubjectHandedness]\n" << hand2char(i.SubjectHandedness) << "\n"; }
        if (!i.Comment.empty()) { oss << "[Comment]\n" << truncate(i.Comment, length) << "\n"; }

        return oss.str();
    }

    auto make_info_content(const amorph& x) -> std::string {
        return make_info_content(api::v1::timepoint2dcdate(x.header.StartTime), x.information);
    }


    static
    auto read_chan(FILE* f, const file_range& x) -> std::vector<int16_t> {
        if (x.size < 0) {
            std::ostringstream oss;
            oss << "[read_chan, cnt_epoch] negative size " << x.size;
            const auto e{ oss.str() };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        const size_t items{ static_cast<size_t>(x.size) / sizeof(int16_t) };
        if (items == 0) {
            std::ostringstream oss;
            oss << "[read_chan, cnt_epoch] empty, size " << x.size << " bytes";
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        if (!seek(f, x.fpos, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[read_chan, cnt_epoch] invalid file position " << x.fpos;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        std::vector<int16_t> row_order(items);
        read(f, begin(row_order), end(row_order));

        return row_order;
    }

    static
    auto read_chan_riff(FILE* f, const chunk& chan) -> std::vector<int16_t> {
        return read_chan(f, chunk_payload(chan));
    }

    static
    auto read_chan_flat(const std::filesystem::path& fname) -> std::vector<int16_t> {
        file_ptr f{ open_r(fname) };
        return read_chan(f.get(), flat_payload(fname));
    }


    auto read_info(FILE* f, const file_range& x, const api::v1::FileVersion& version) -> std::pair<std::chrono::system_clock::time_point, api::v1::Info> {
        if (x.size == 0) {
            return { api::v1::dcdate2timepoint(api::v1::DcDate{}), api::v1::Info{} };
        }

        if (!seek(f, x.fpos, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[read_info, cnt_epoch] invalid file position " << x.fpos;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        std::string s;
        s.resize(static_cast<size_t>(x.size));
        read(f, begin(s), end(s));

        auto[start_time, i, is_ascii]{ parse_info(s) };

        // compatibility
        if (!is_ascii && version.Major == 0 && version.Minor == 0) {
            if (!seek(f, x.fpos, SEEK_SET)) {
                std::ostringstream oss;
                oss << "[read_info, cnt_epoch] can not seek back to file position " << x.fpos;
                const auto e{ oss.str() };
                ctk_log_critical(e);
                throw api::v1::CtkBug{ e };
            }
            start_time.Date = read(f, double{});
            start_time.Fraction = read(f, double{});
        }

        return { api::v1::dcdate2timepoint(start_time), i };
    }


    static
    auto read_info_riff(FILE* f, const chunk& info, const api::v1::FileVersion& version) -> std::pair<std::chrono::system_clock::time_point, api::v1::Info> {
        return read_info(f, chunk_payload(info), version);
    }

    static
    auto read_info_flat(const std::filesystem::path& fname, const api::v1::FileVersion& version) -> std::pair<std::chrono::system_clock::time_point, api::v1::Info> {
        file_ptr f{ open_r(fname) };
        return read_info(f.get(), flat_payload(fname), version);
    }



    static
    auto read_sample_count_flat(const std::filesystem::path& fname) -> measurement_count {
        using Int = measurement_count::value_type;

        constexpr const int64_t tsize{ sizeof(Int) };
        const int64_t fsize{ content_size(fname) };
        if (fsize < part_header_size + tsize) {
            std::ostringstream oss;
            oss << "[read_sample_count_flat, cnt_epoch] empty, file_size < part_header_size + part_tag_size: " << fsize
                << " < " << part_header_size
                << " + " << tsize;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
        auto payload{ fsize - part_header_size };
        const auto[quot, rem]{ std::div(payload, tsize) };
        if (rem != 0) {
            std::ostringstream oss;
            oss << "[read_sample_count_flat, cnt_epoch] odd payload " << payload << " % " << tsize << " = " << rem << " (!= 0)";
            oss << ", correcting to " << (quot * tsize);
            const auto e{ oss.str() };
            ctk_log_warning(e);

            payload = quot * tsize;
        }
        const auto latest{ part_header_size + payload - tsize };

        file_ptr f{ open_r(fname) };
        if (!seek(f.get(), latest, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[read_sample_count_flat, cnt_epoch] can not seek to file position " << latest;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
        return measurement_count{ read(f.get(), Int{}) };
    }


    static
    auto read_sampling_frequency_flat(const std::filesystem::path& fname) -> double {
        file_ptr f{ open_r(fname) };
        skip_part_header(f.get());
        return read(f.get(), double{});
    }


    static
    auto read_history_flat(const std::filesystem::path& fname) -> std::string {
        file_ptr f{ open_r(fname) };
        skip_part_header(f.get());
        const auto payload{ flat_payload(fname) };

        std::string xs;
        xs.resize(as_sizet(payload.size));
        read(f.get(), begin(xs), end(xs));
        return xs;
    }


    static
    auto sub_chunks(const chunk& parent, FILE* f) -> std::vector<chunk> {
        assert(f);
        assert(is_even(parent.storage.fpos));

        std::vector<chunk> result;
        if (!is_root_or_list(parent)) {
            const std::string e{ "[sub_chunks] no sub chunks in a data chunk" };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        const int64_t hs{ header_size(parent) };
        //const int64_t last{ parent.storage.fpos + hs + make_even(parent.storage.size) };
        int64_t last{ plus(parent.storage.fpos, hs, ok{}) };
        last = plus(last, make_even(parent.storage.size), ok{});
        //int64_t first{ parent.storage.fpos + hs + label_size() };
        int64_t first{ plus(parent.storage.fpos, hs, ok{}) };
        first = plus(first, label_size(), ok{});

        if (!seek(f, first, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[sub_chunks, cnt_epoch] can not seek to payload " << first;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        while (first < last) {
            const auto maybe_next{ maybe_read_chunk(f, parent) };
            if (!maybe_next) {
                ctk_log_warning("[sub_chunks, cnt_epoch] malformed input file. sum(sub-chunks) != data_size(parent chunk)");
                break;
            }
            const chunk next{ *maybe_next };
            result.push_back(next);

            //first = next.storage.fpos + hs + make_even(next.storage.size);
            first = plus(next.storage.fpos, hs, ok{});
            first = plus(first, make_even(next.storage.size), ok{});
            if (!seek(f, first, SEEK_SET)) {
                break;
            }
        }

        return result;
    }


    static
    auto user_chunk(const chunk& x) -> user_content {
        return { as_string(x.id), chunk_payload(x) };
    }


    static
    auto is_valid(const ctk::api::v1::DcDate& x) -> bool {
        return std::isfinite(x.Date) && std::isfinite(x.Fraction);
    }



    enum class status_elc{ ok, empty, trunc, zero, space, brace, semicolon, iscale, rscale };

    static
    auto is_valid_optional_label(const std::string& x, unsigned max_size) -> status_elc {
        if (max_size < x.size()) {
            return status_elc::trunc;
        }

        // stored/parsed as ascii
        if (x.find(' ') != std::string::npos) {
            return status_elc::space;
        }

        if (x.find('\0') != std::string::npos) {
            return status_elc::zero;
        }

        return status_elc::ok;
    }

    static
    auto is_valid_mandatory_label(const std::string& x, unsigned max_size) -> status_elc {
        if (x.empty()) {
            return status_elc::empty;
        }

        return is_valid_optional_label(x, max_size);
    }

    static
    auto is_valid_label_reflib(const std::string& x) -> status_elc {
        auto status{ is_valid_mandatory_label(x, api::v1::sizes::eeph_electrode_active) };
        if (status != status_elc::ok) {
            return status;
        }

        // common with libeep
        if (x[0] == ';') {
            return status_elc::semicolon;
        }

        // specific to my implementation
        if (x[0] == '[') {
            return status_elc::brace;
        }

        return status_elc::ok;
    }

    static
    auto is_valid_unit_reflib(const std::string& x) -> status_elc {
        return is_valid_mandatory_label(x, api::v1::sizes::eeph_electrode_unit);
    }

    static
    auto is_valid_reference_reflib(const std::string& x) -> status_elc {
        return is_valid_optional_label(x, api::v1::sizes::eeph_electrode_reference);
    }

    static
    auto is_valid_status_reflib(const std::string& x) -> status_elc {
        return is_valid_optional_label(x, api::v1::sizes::eeph_electrode_status);
    }

    static
    auto is_valid_type_reflib(const std::string& x) -> status_elc {
        return is_valid_optional_label(x, api::v1::sizes::eeph_electrode_type);
    }

    static
    auto is_valid_electrode(const ctk::api::v1::Electrode& x) -> status_elc {
        auto valid{ is_valid_label_reflib(x.ActiveLabel) };
        if (valid != status_elc::ok) { return valid; };

        valid = is_valid_unit_reflib(x.Unit);
        if (valid != status_elc::ok) { return valid; };

        valid = is_valid_reference_reflib(x.Reference);
        if (valid != status_elc::ok) { return valid; };

        valid = is_valid_status_reflib(x.Status);
        if (valid != status_elc::ok) { return valid; };

        valid = is_valid_type_reflib(x.Type);
        if (valid != status_elc::ok) { return valid; };

        if (!std::isfinite(x.IScale)) { return status_elc::iscale; }
        if (!std::isfinite(x.RScale)) { return status_elc::rscale; }
        return status_elc::ok;
    }

    static
    auto validate_optional_label(const std::string& x, status_elc status, unsigned max_size, std::string name) -> void {
        if (status == status_elc::trunc) {
            std::ostringstream oss;
            oss << "[validate_optional_label, cnt_epoch] " << name
                << " '" << x
                << "' longer than " << max_size << " bytes";
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }
        else if (status == status_elc::space) {
            std::ostringstream oss;
            oss << "[validate_optional_label, cnt_epoch] " << name << " '" << x << "' contains space";
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }
        else if (status == status_elc::zero) {
            std::ostringstream oss;
            oss << "[validate_optional_label, cnt_epoch] " << name << " '" << x << "' contains embedded zero";
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }
    }

    static
    auto validate_mandatory_label(const std::string& x, status_elc status, unsigned max_size, std::string name) -> void {
        validate_optional_label(x, status, max_size, name);

        if (status == status_elc::empty) {
            std::ostringstream oss;
            oss << "[validate_mandatory_label, cnt_epoch] empty " << name;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }
    }

    auto validate_electrode_label_reflib(const std::string& x) -> void {
        const status_elc valid{ is_valid_label_reflib(x) };
        if (valid == status_elc::ok) {
            return;
        }

        validate_mandatory_label(x, valid, api::v1::sizes::eeph_electrode_active, "active label");

        std::string e;
        if (valid == status_elc::brace) {
            std::ostringstream oss;
            oss << "[validate_electrode_label_reflib, cnt_epoch] active label '" << x << "' starts with [";
            e = oss.str();
        }
        else if (valid == status_elc::semicolon) {
            std::ostringstream oss;
            oss << "[validate_electrode_label_reflib, cnt_epoch] active label '" << x << "' starts with ;";
            e = oss.str();
        }
        else {
            const std::string msg{ "[validate_electrode_label_reflib, cnt_epoch] active label unexpected state" };
            ctk_log_critical(msg);
            throw api::v1::CtkBug{ msg };
        }

        ctk_log_error(e);
        throw api::v1::CtkLimit{ e };
    }

    auto validate_electrode_unit_reflib(const std::string& x) -> void {
        validate_mandatory_label(x, is_valid_unit_reflib(x), api::v1::sizes::eeph_electrode_unit, "unit");
    }

    auto validate_electrode_reference_reflib(const std::string& x) -> void {
        validate_optional_label(x, is_valid_reference_reflib(x), api::v1::sizes::eeph_electrode_reference, "reference label");
    }

    auto validate_electrode_status_reflib(const std::string& x) -> void {
        validate_optional_label(x, is_valid_status_reflib(x), api::v1::sizes::eeph_electrode_status, "status");
    }

    auto validate_electrode_type_reflib(const std::string& x) -> void {
        validate_optional_label(x, is_valid_type_reflib(x), api::v1::sizes::eeph_electrode_type, "type");
    }


    auto is_valid(const ctk::api::v1::Electrode& x) -> bool {
        return is_valid_electrode(x) == status_elc::ok;
    }

    auto validate(const api::v1::Electrode& x) -> void {
        validate_electrode_label_reflib(x.ActiveLabel);
        validate_electrode_unit_reflib(x.Unit);
        validate_electrode_reference_reflib(x.Reference);
        validate_electrode_status_reflib(x.Status);
        validate_electrode_type_reflib(x.Type);

        if (!std::isfinite(x.IScale)) {
            const std::string e{ "[validate(Electrode), cnt_epoch] infinite iscale" };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }

        if (!std::isfinite(x.RScale)) {
            const std::string e{ "[validate(Electrode), cnt_epoch] infinite rscale" };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }
    }


    enum class status_ts{ ok, epochl, sfreq, stamp, no_elc, invalid_elc };

    static
    auto valid_time_series(const api::v1::TimeSeries& x) -> status_ts {
        if (x.EpochLength < 1) {
            return status_ts::epochl;
        }

        if (!std::isfinite(x.SamplingFrequency) || x.SamplingFrequency <= 0) {
            return status_ts::sfreq;
        }

        if (!is_valid(api::v1::timepoint2dcdate(x.StartTime))) {
            return status_ts::stamp;
        }

        if (x.Electrodes.empty()) {
            return status_ts::no_elc;
        }

        const auto check_electrode = [](bool acc, const api::v1::Electrode& e) -> bool { return acc && is_valid(e); };

        const auto all_valid{ std::accumulate(begin(x.Electrodes), end(x.Electrodes), true, check_electrode) };
        return all_valid ? status_ts::ok : status_ts::invalid_elc;
    }

    auto validate(const api::v1::TimeSeries& x) -> void {
        const status_ts time_series_status{ valid_time_series(x) };
        switch (time_series_status) {
            case status_ts::ok: return;
            case status_ts::epochl: {
                std::ostringstream oss;
                oss << "[validate(TimeSeries), cnt_epoch] invalid epoch length " << x.EpochLength;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw api::v1::CtkLimit{ e };
            }
            case status_ts::sfreq: {
                std::ostringstream oss;
                oss << "[validate(TimeSeries), cnt_epoch] invalid sampling frequency " << x.SamplingFrequency;
                const auto e{ oss.str() };
                ctk_log_error(e);
                throw api::v1::CtkLimit{ e };
            }
            case status_ts::stamp: {
                const std::string e{ "[validate(TimeSeries), cnt_epoch] invalid start time" };
                ctk_log_error(e);
                throw api::v1::CtkLimit{ e };
            }
            case status_ts::no_elc: {
                const std::string e{ "[validate(TimeSeries), cnt_epoch] no electrodes" };
                ctk_log_error(e);
                throw api::v1::CtkLimit{ e };
            }
            case status_ts::invalid_elc: /* fall trough */;
        }

        for (const api::v1::Electrode& e : x.Electrodes) {
            validate(e);
        }

        const std::string msg{ "[validate(TimeSeries), cnt_epoch] expected exception was not thrown by the electrode validation" };
        ctk_log_critical(msg);
        throw api::v1::CtkBug{ msg };
    }


    enum class status_amorph{ ok, samples, ts, order, order_elc, ranges, fpos, increasing, content };

    static
    auto is_valid(const amorph& x) -> status_amorph {
        if (x.sample_count < 1) {
            return status_amorph::samples;
        }

        if (valid_time_series(x.header) != status_ts::ok) {
            return status_amorph::ts;
        }

        if (!is_valid_row_order(x.order)) {
            return status_amorph::order;
        }

        if (x.order.size() != x.header.Electrodes.size()) {
            return status_amorph::order_elc;
        }
        
        if (x.epoch_ranges.empty()) {
            return status_amorph::ranges;
        }

        if (x.epoch_ranges[0].fpos < 0) {
            return status_amorph::fpos;
        }

        const auto non_increasing = [](const file_range& x, const file_range& y) -> bool { return y.fpos <= x.fpos; };
        const auto sour{ std::adjacent_find(begin(x.epoch_ranges), end(x.epoch_ranges), non_increasing) };
        if (sour != end(x.epoch_ranges)) {
            return status_amorph::increasing;
        }

        const auto has_content = [](bool acc, const file_range& r) -> bool { return acc && 0 < r.size; };
        const bool non_empty{ std::accumulate(begin(x.epoch_ranges), end(x.epoch_ranges), true, has_content) };
        return non_empty ? status_amorph::ok : status_amorph::content;
    }


    /*
    static
    auto guess_data_chunk(label_type id, int64_t fpos, int64_t fsize, chunk& x, chunk* previous) -> void {
        const int64_t riff_header_size{ header_size(x) };

        x.id = id;
        x.storage.fpos = fpos;
        x.storage.size = fsize - riff_header_size - x.storage.fpos;

        if (!previous) {
            return;
        }
        previous->storage.size = fpos - riff_header_size - previous->storage.fpos;
    }


    static
    auto scan_broken_reflib(FILE* f, chunk& chunk_ep, chunk& chunk_chan, chunk& chunk_data, chunk& chunk_eeph, chunk& chunk_info, chunk& chunk_evt) -> void {
        chunk* previous{ nullptr };

        const label_type label_info{ as_label("info") };
        const label_type label_chan{ as_label("chan") };
        const label_type label_data{ as_label("data") };
        const label_type label_ep{ as_label("ep") };
        const label_type label_eeph{ as_label("eeph") };
        const label_type label_evt{ as_label("evt") };
        const int64_t fsize{ file_size(f) };
        constexpr const int64_t label_size{ sizeof(label_type) };

        for (int64_t fpos{ 0 }; fpos < fsize - label_size; fpos += 2) {
            seek(f, fpos, SEEK_SET);
            const auto label{ read(f, label_type{}) };

            if (label == label_info) {
                guess_data_chunk(label_info, fpos, fsize, chunk_info, previous);
                previous = &chunk_info;
            }
            else if (label == label_chan) {
                guess_data_chunk(label_chan, fpos, fsize, chunk_chan, previous);
                previous = &chunk_chan;
            }
            else if (label == label_data) {
                guess_data_chunk(label_data, fpos, fsize, chunk_data, previous);
                previous = &chunk_data;
            }
            else if (label == label_ep) {
                guess_data_chunk(label_ep, fpos, fsize, chunk_ep, previous);
                previous = &chunk_ep;
            }
            else if (label == label_eeph) {
                guess_data_chunk(label_eeph, fpos, fsize, chunk_eeph, previous);
                previous = &chunk_eeph;
            }
            else if (label == label_evt) {
                guess_data_chunk(label_evt, fpos, fsize, chunk_evt, previous);
                previous = &chunk_evt;
            }
        }
    }
    */


    static
    auto read_expected_chunks_reflib(const chunk& root, FILE* f, chunk& ep, chunk& chan, chunk& data, chunk& eeph, chunk& info, chunk& evt, std::vector<chunk>& user) -> void {
        const auto list_top_level{ sub_chunks(root, f) };
        for (const chunk& top_level_chunk : list_top_level) {
            if (is_eeph(top_level_chunk)) {
                eeph = top_level_chunk;
                continue;
            }
            else if (is_evt(top_level_chunk)) {
                evt = top_level_chunk;
                continue;
            }
            else if (is_info(top_level_chunk)) {
                info = top_level_chunk;
                continue;
            }
            // temp
            else if (is_average(top_level_chunk)) {
                const std::string e{ "[read_expected_chunks_reflib, cnt_epoch] average not implemented" };
                ctk_log_error(e);
                throw api::v1::CtkData{ e };
            }
            else if (is_stddev(top_level_chunk)) {
                const std::string e{ "[read_expected_chunks_reflib, cnt_epoch] stddev not implemented" };
                ctk_log_error(e);
                throw api::v1::CtkData{ e };
            }
            else if (is_wavelet(top_level_chunk)) {
                const std::string e{ "[read_expected_chunks_reflib, cnt_epoch] wavelet not implemented" };
                ctk_log_error(e);
                throw api::v1::CtkData{ e };
            }
            // TODO: maybe skip "refh" and "imp " as well
            // end temp

            if (!is_list(top_level_chunk)) {
                user.push_back(top_level_chunk);
                continue;
            }

            const auto list_subchunks{ sub_chunks(top_level_chunk, f) };
            for (const chunk& sub_chunk : list_subchunks) {
                if (is_raw3_chan(sub_chunk, top_level_chunk)) {
                    chan = sub_chunk;
                }
                else if (is_raw3_ep(sub_chunk, top_level_chunk)) {
                    ep = sub_chunk;
                }
                else if (is_raw3_data(sub_chunk, top_level_chunk)) {
                    data = sub_chunk;
                }
                else {
                    // whatever
                }
            }
        }
    }


    static
    auto validate_eeph_dimensions(measurement_count sample_count, size_t electrodes, size_t order, sint channel_count) -> void {
        if (electrodes < 1 || sample_count < 1) {
            std::ostringstream oss;
            oss << "[validate_eeph_dimensions, cnt_epoch] invalid " << electrodes << " x " << sample_count;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        if (order != electrodes) {
            std::ostringstream oss;
            oss << "[validate_eeph_dimensions, cnt_epoch] order != electrodes: " << order << " != " << electrodes;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        if (channel_count != cast(order, sint{}, ok{})) {
            std::ostringstream oss;
            oss << "[validate_eeph_dimensions, cnt_epoch] order != channels: " << order << " != " << channel_count;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
    }


    static
    auto read_reflib_cnt(const chunk& root, FILE* f) -> amorph {
        if (!is_root(root)) {
            const std::string e{ "[read_reflib_cnt, cnt_epoch] invalid file" };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        chunk ep{ empty_chunk(root) };
        chunk chan{ empty_chunk(root) };
        chunk data{ empty_chunk(root) };
        chunk eeph{ empty_chunk(root) };
        chunk inf{ empty_chunk(root) };
        chunk evt{ empty_chunk(root) };
        std::vector<chunk> user;

        read_expected_chunks_reflib(root, f, ep, chan, data, eeph, inf, evt, user);
        //scan_broken_reflib(f, ep, chan, data, eeph, inf, evt);

        if (ep.storage.size == 0 || chan.storage.size == 0 || data.storage.size == 0 || eeph.storage.size == 0) {
            const std::string ep_str{ ep.storage.size == 0 ? " raw3/ep" : "" };
            const std::string chan_str{ ep.storage.size == 0 ? " raw3/chan" : "" };
            const std::string data_str{ ep.storage.size == 0 ? " raw3/data" : "" };
            const std::string eeph_str{ ep.storage.size == 0 ? " eeph" : "" };

            std::ostringstream oss;
            oss << "[read_reflib_cnt, cnt_epoch, cnt_epoch] missing chunks:" << eeph_str << ep_str << data_str << chan_str;
            const auto e { oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        const auto eep_h{ read_eeph_riff(f, eeph) };
        const auto order{ read_chan_riff(f, chan) };
        const sint channel_count{ eep_h.channel_count };
        validate_eeph_dimensions(eep_h.sample_count, eep_h.electrodes.size(), order.size(), channel_count);

        const auto [length, offsets]{ read_ep_riff(f, ep)  };
        const auto[start_time, information]{ read_info_riff(f, inf, eep_h.version) };

        amorph x;
        x.header.StartTime = start_time;
        x.header.EpochLength = static_cast<measurement_count::value_type>(length);
        x.header.Electrodes = eep_h.electrodes;
        x.header.SamplingFrequency = eep_h.sampling_frequency;
        x.sample_count = eep_h.sample_count;
        x.version = eep_h.version;
        x.history = eep_h.history;
        x.epoch_ranges = offsets2ranges_riff(data, offsets);
        x.trigger_range = chunk_payload(evt);
        x.order = order;
        x.information = information;

        const auto chunk2content = [](const chunk& x) -> user_content { return user_chunk(x); };
        x.user.resize(user.size());
        std::transform(begin(user), end(user), begin(x.user), chunk2content);

        return x;
    }


    static
    auto read_compressed_epoch(FILE* f, const file_range& x) -> std::vector<uint8_t> {
        assert(f && 0 <= x.fpos && 0 < x.size);

        if (!seek(f, x.fpos, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[read_compressed_epoch, cnt_epoch] can not seek to file position " << x.fpos;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        std::vector<uint8_t> storage;
        storage.resize(static_cast<size_t>(x.size));
        read(f, begin(storage), end(storage));
        return storage;
    }


    static
    auto append_to_filename(std::filesystem::path x, std::string appendix) -> std::filesystem::path {
        auto name{ x.filename() };
        name += appendix;
        x.replace_filename(name);
        return x;
    }


    auto fname_data(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_raw3_data.bin");
    }

    auto fname_ep(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_raw3_ep.bin");
    }

    auto fname_chan(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_raw3_chan.bin");
    }

    auto fname_sample_count(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_raw3_sample_count.bin");
    }

    auto fname_electrodes(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_electrodes.bin");
    }

    auto fname_sampling_frequency(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_sampling_frequency.bin");
    }

    auto fname_triggers(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_triggers.bin");
    }

    auto fname_info(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_info.bin");
    }

    auto fname_cnt_type(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_type.bin");
    }

    auto fname_history(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_history.bin");
    }

    auto fname_time_series_header(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_time_series_header.bin");
    }

    auto fname_flat(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_flat");
    }

    auto fname_archive_bin(std::filesystem::path x) -> std::filesystem::path {
        return append_to_filename(x, "_archive.bin");
    }


    auto delete_files(const std::vector<std::filesystem::path>& xs) -> bool {
        const auto del = [](bool acc, const std::filesystem::path& x) -> bool { return acc && std::filesystem::remove(x); };
        return std::accumulate(begin(xs), end(xs), true, del);
    }



    static
    auto compressed_epoch_length(epoch_count n, measurement_count total, measurement_count epoch_length) -> measurement_count {
        const sint el{ epoch_length };
        const sint i{ n };
        const sint i_next{ plus(i, sint{ 1 }, ok{})  };
        const measurement_count next{ multiply(i_next, el, ok{}) };

        if (total < next) {
            const measurement_count previous{ static_cast<measurement_count::value_type>(i * el) };
            if (total <= previous) {
                return measurement_count{ 0 };
            }

            return total - previous;
        }

        return epoch_length;
    }


    static
    auto epoch_n(FILE* f, epoch_count i, const std::vector<file_range>& epoch_ranges, measurement_count sample_count, measurement_count epoch_length) -> compressed_epoch {
        const epoch_count total{ vsize(epoch_ranges) };
        if (i < 0 || total <= i) {
            std::ostringstream oss;
            oss << "[epoch_n, cnt_epoch] epoch " << (i + epoch_count{ 1 }) << "/" << total << " not accessible";
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
        assert(0 <= i);
        assert(f);

        const auto n{ as_sizet(i) };
        const auto data{ read_compressed_epoch(f, epoch_ranges[n]) };
        const auto length{ compressed_epoch_length(i, sample_count, epoch_length) };

        return { length, data };
    }




    static
    auto reserve_size_field(FILE* f, const chunk& c) -> void {
        assert(is_even(tell(f)));
        for (size_t i{ 0 }; i < field_size(c); ++i) {
            write(f, uint8_t{ 0 });
        }
    }

    static
    auto update_size_field(FILE* f, const chunk& c) -> void {
        const int64_t size_position{ c.storage.fpos + label_size() };
        assert(is_even(size_position));
        if (!seek(f, size_position, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[update_size_field, cnt_epoch] can not seek back to file position " << size_position;
            const auto e{ oss.str() };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        c.riff->write_entity(f, c.storage.size);
    }


    class riff_chunk_writer
    {
        FILE* f;
        chunk scratch;

    public:

        riff_chunk_writer(FILE* f, const chunk& c)
        : f{ f }
        , scratch{ c } {
            scratch.storage.fpos = tell(f);
            assert(is_even(scratch.storage.fpos));

            write(f, scratch.id);
            reserve_size_field(f, scratch);
            if (is_root_or_list(scratch)) {
                write(f, scratch.label); // list label
            }
        }

        ~riff_chunk_writer() {
            const auto fpos{ tell(f) };
            scratch.storage.size = fpos - scratch.storage.fpos - header_size(scratch);
            update_size_field(f, scratch);
            seek(f, fpos, SEEK_SET);

            if (!is_even(scratch.storage.size)) {
                write(f, uint8_t{ 0 }); // riff chunks have even size
            }
        }
    };


    auto content2chunk(FILE* f, const riff_text& x) -> void {
        assert(is_even(tell(f)));

        if (x.s.empty()) {
            return;
        }

        riff_chunk_writer raii{ f, x.c };

        write(f, begin(x.s), end(x.s));
    }


    auto content2chunk(FILE* f, const riff_file& x) -> void {
        assert(is_even(tell(f)));

        if (!x.fname.has_filename()) {
            const std::string e{ "[content2chunk(riff_file), cnt_epoch] empty file name" };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        file_ptr fin{ open_r(x.fname) };
        const int64_t fsize{ content_size(x.fname) };

        riff_chunk_writer raii{ f, x.c };
        copy_file_portion(fin.get(), { x.offset, fsize - x.offset }, f);
    }


    auto content2chunk(FILE* f, const riff_list& l) -> void {
        assert(is_even(tell(f)));

        if (l.subnodes.empty()) {
            const std::string e{ "[content2chunk(riff_list), cnt_epoch] empty list" };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        riff_chunk_writer raii{ f, l.c };

        for (const auto& subnode : l.subnodes) {
            subnode.write(f);
        }
    }


    riff_text::riff_text(const chunk& c, const std::string& s)
    : c{ c }
    , s{ s } {
    }


    riff_file::riff_file(const chunk& c, const std::filesystem::path& fname, int64_t offset)
    : c{ c }
    , fname{ fname }
    , offset{ offset } {
    }


    riff_list::riff_list(const chunk& list)
    : c{ list } {
        if (!is_root_or_list(c)) {
            std::ostringstream oss;
            oss << "[riff_list::riff_list, cnt_epoch] chunk " << as_string(c.id) << "/" << as_string(c.label) << " is not a list";
            const auto e{ oss.str() };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }
    }

    auto riff_list::push_back(const riff_node& x) -> void {
        subnodes.push_back(x);
    }

    auto riff_list::back() const -> const riff_node& {
        return subnodes.back();
    }



    auto epoch_writer_flat::add_file(std::filesystem::path name, file_tag id, std::string chunk_id) -> FILE* {
        tokens.push_back(own_file{ open_w(name), file_token{ tagged_file{ id, name }, as_label(chunk_id) } });
        write_part_header(tokens.back().f.get(), tokens.back().t.tag.id, tokens.back().t.chunk_id);
        return tokens.back().f.get();
    }

    auto epoch_writer_flat::close_last() -> void {
        tokens.back().f.reset(nullptr);
    }

    epoch_writer_flat::epoch_writer_flat(const std::filesystem::path& cnt, const api::v1::TimeSeries& x, api::v1::RiffType s)
    : samples{ 0 }
    , epoch_size{ x.EpochLength }
    , start_time{ x.StartTime }
    , f_ep{ nullptr }
    , f_data{ nullptr }
    , f_sample_count{ nullptr }
    , f_triggers{ nullptr }
    , f_info{ nullptr }
    , f_history{ nullptr }
    , riff{ make_cnt_field_size(s) }
    , fname{ cnt }
    , open_files{ 0 } {
        validate(x);

        // these files remain open during the life time of the object
        f_ep = add_file(fname_ep(fname), file_tag::ep, "raw3");
        f_data = add_file(fname_data(fname), file_tag::data, "raw3");
        f_sample_count = add_file(fname_sample_count(fname), file_tag::sample_count, "eeph");
        f_triggers = add_file(fname_triggers(fname), file_tag::triggers, "evt ");
        f_info = add_file(fname_info(fname), file_tag::info, "info");
        f_history = add_file(fname_history(fname), file_tag::history, "eeph");
        open_files = tokens.size();

        const measurement_count::value_type epoch_length{ epoch_size };
        riff->write_entity(f_ep, epoch_length);
        epoch_ranges.emplace_back(0, 0);

        const api::v1::DcDate start{ api::v1::timepoint2dcdate(start_time) };
        ascii_parseable(start.Date); // compatibility: stored in info
        ascii_parseable(start.Fraction); // compatibility: stored in info
        const auto i{ make_info_content(start, api::v1::Info{}) };
        write(f_info, begin(i), end(i));
        ::fflush(f_info);

        // written only once
        const sensor_count c{ vsize(x.Electrodes) };
        const auto o{ natural_row_order(c) };
        FILE* f_chan{ add_file(fname_chan(fname), file_tag::chan, "raw3") };
        write(f_chan, begin(o), end(o));
        close_last();

        ascii_parseable(x.SamplingFrequency); // compatibility: stored in eeph
        FILE* f_sampling_frequency{ add_file(fname_sampling_frequency(fname), file_tag::sampling_frequency, "eeph") };
        write(f_sampling_frequency, x.SamplingFrequency);
        close_last();

        FILE* f_electrodes{ add_file(fname_electrodes(fname), file_tag::electrodes, "eeph") };
        write_electrodes_bin(f_electrodes, x.Electrodes);
        close_last();

        FILE* f_type{ add_file(fname_cnt_type(fname), file_tag::cnt_type, "cntt") };
        const auto t{ riff->root_id() };
        write(f_type, begin(t), end(t));
        close_last();
    }

    auto epoch_writer_flat::append(const compressed_epoch& ce) -> void {
        assert(f_data && f_ep && f_sample_count);
        assert(!epoch_ranges.empty());

        if (ce.data.empty()) {
            const std::string e{ "[epoch_writer_flat::append, cnt_epoch] no compressed data" };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        // 1) compressed epoch data
        write(f_data, begin(ce.data), end(ce.data));

        // 2) offsets into the compressed epoch data
        riff->write_entity(f_ep, epoch_ranges.back().fpos);
        epoch_ranges.emplace_back(tell(f_data) - part_header_size, 0);

        // 3) sample count
        using Int = measurement_count::value_type;
        const Int length{ ce.length };
        const Int sample_count{ samples };
        const Int sum{ plus(sample_count, length, ok{}) };
        write(f_sample_count, sum);
        samples = measurement_count{ sum };
    }

    auto epoch_writer_flat::append(const api::v1::Trigger& x) -> void {
        assert(riff && f_triggers);
        riff->write_trigger(f_triggers, x);
    }

    auto epoch_writer_flat::append(const std::vector<api::v1::Trigger>& xs) -> void {
        assert(riff && f_triggers);
        riff->write_triggers(f_triggers, xs);
    }

    auto epoch_writer_flat::close() -> void {
        for (size_t i{ 0 }; i < open_files; ++i) {
            tokens[i].f.reset(nullptr);
        }
        open_files = 0;
    }

    auto epoch_writer_flat::flush() -> void {
        ::fflush(f_data);
        ::fflush(f_ep);
        ::fflush(f_sample_count);
        ::fflush(f_triggers);
    }

    auto epoch_writer_flat::info(const api::v1::Info& x) -> void {
        assert(f_info);
        const api::v1::DcDate start{ api::v1::timepoint2dcdate(start_time) };
        const auto i{ make_info_content(start, x) };
        seek(f_info, part_header_size, SEEK_SET);
        write(f_info, begin(i), end(i));
        ::fflush(f_info);
    }

    auto epoch_writer_flat::history(const std::string& x) -> void {
        assert(f_history);
        seek(f_history, part_header_size, SEEK_SET);
        write(f_history, begin(x), end(x));
        ::fflush(f_history);
    }

    auto epoch_writer_flat::file_tokens() const -> std::vector<tagged_file> {
        const auto get_tag = [](const own_file& x) -> tagged_file { return x.t.tag; };

        std::vector<tagged_file> result(tokens.size());
        std::transform(begin(tokens), end(tokens), begin(result), get_tag);
        return result;
    }

    auto epoch_writer_flat::file_name() const -> std::filesystem::path {
        return fname;
    }

    auto epoch_writer_flat::epoch_length() const -> measurement_count {
        return epoch_size;
    }

    auto epoch_writer_flat::sample_count() const -> measurement_count {
        return samples;
    }





    epoch_reader_common::epoch_reader_common(FILE* fd, FILE* ft, const amorph& d, const cnt_field_sizes* r, int64_t offset)
    : f_data{ fd }
    , f_triggers{ ft }
    , data{ &d }
    , riff{ r } {
        const status_amorph status{ is_valid(d) };
        if (status != status_amorph::ok) {
            switch (status) {
                case status_amorph::ok: assert(false);
                case status_amorph::samples: {
                    std::ostringstream oss;
                    oss << "[epoch_reader_common::epoch_reader_common, cnt_epoch] invalid sample count " << d.sample_count;
                    const auto e{ oss.str() };
                    ctk_log_error(e);
                    throw api::v1::CtkData{ e };
                }
                case status_amorph::ts: {
                    const std::string e{ "[epoch_reader_common::epoch_reader_common, cnt_epoch] invalid time signal" };
                    ctk_log_error(e);
                    validate(d.header);
                    const std::string msg{ "[epoch_reader_common::epoch_reader_common, cnt_epoch] expected exception was not thrown" };
                    ctk_log_critical(msg);
                    throw api::v1::CtkBug{ msg };
                }
                case status_amorph::order: {
                    const std::string e{ "[epoch_reader_common::epoch_reader_common, cnt_epoch] invalid row order" };
                    ctk_log_error(e);
                    throw api::v1::CtkData{ e };
                }
                case status_amorph::order_elc: {
                    std::ostringstream oss;
                    oss << "[epoch_reader_common::epoch_reader_common, cnt_epoch] electrodes != channels: "
                    << d.header.Electrodes.size() << " != " << d.order.size();
                    const auto e{ oss.str() };
                    ctk_log_error(e);
                    throw api::v1::CtkData{ e };
                }
                case status_amorph::ranges: {
                    const std::string e{ "[epoch_reader_common::epoch_reader_common, cnt_epoch] no epoch data" };
                    ctk_log_error(e);
                    throw api::v1::CtkData{ e };
                }
                case status_amorph::fpos: {
                    const std::string e{ "[epoch_reader_common::epoch_reader_common, cnt_epoch] invalid file position" };
                    ctk_log_error(e);
                    throw api::v1::CtkData{ e };
                }
                case status_amorph::increasing: {
                    const std::string e{ "[epoch_reader_common::epoch_reader_common, cnt_epoch] non increasing epochs" };
                    ctk_log_error(e);
                    throw api::v1::CtkData{ e };
                }
                case status_amorph::content: {
                    const std::string e{ "[epoch_reader_common::epoch_reader_common, cnt_epoch] epoch without content" };
                    ctk_log_error(e);
                    throw api::v1::CtkData{ e };
                }
            }
        }

        seek(f_data, offset, SEEK_SET);
        seek(f_triggers, offset, SEEK_SET);
    }

    auto epoch_reader_common::count() const -> epoch_count {
        return epoch_count{ vsize(data->epoch_ranges) };
    }

    auto epoch_reader_common::epoch(epoch_count i) const -> compressed_epoch {
        return epoch_n(f_data, i, data->epoch_ranges, data->sample_count, measurement_count{ data->header.EpochLength });
    }

    auto epoch_reader_common::has_triggers() const -> bool {
        return f_triggers && 0 < data->trigger_range.size;
    }

    auto epoch_reader_common::triggers() const -> std::vector<api::v1::Trigger> {
        if (!has_triggers()) {
            return {};
        }

        return riff->read_triggers(f_triggers, data->trigger_range);
    }

    auto epoch_reader_common::epoch_length() const -> measurement_count {
        return measurement_count{ data->header.EpochLength };
    }

    auto epoch_reader_common::sample_count() const -> measurement_count {
        return data->sample_count;
    }

    auto epoch_reader_common::sampling_frequency() const -> double {
        return data->header.SamplingFrequency;
    }

    auto epoch_reader_common::param_eeg() const -> api::v1::TimeSeries {
        return data->header;
    }

    auto epoch_reader_common::order() const -> std::vector<int16_t> {
        return data->order;
    }

    auto epoch_reader_common::channel_count() const -> sensor_count {
        return sensor_count{ vsize(data->order) };
    }

    auto epoch_reader_common::channels() const -> std::vector<api::v1::Electrode> {
        return data->header.Electrodes;
    }

    auto epoch_reader_common::info_content() const -> std::string {
        return make_info_content(*data);
    }

    auto epoch_reader_common::information() const -> api::v1::Info {
        return data->information;
    }

    auto epoch_reader_common::cnt_type() const -> api::v1::RiffType {
        return string2riff(riff->root_id());
    }

    auto epoch_reader_common::file_version() const -> api::v1::FileVersion {
        return data->version;
    }

    auto epoch_reader_common::segment_start_time() const -> api::v1::DcDate {
        return api::v1::timepoint2dcdate(data->header.StartTime);
    }

    auto epoch_reader_common::history() const -> std::string {
        return data->history;
    }


    static
    auto add_ctk_part(const std::filesystem::path& fname, file_tag tag, std::string label, std::string ui_name, std::vector<tagged_file>& xs) -> void {
        constexpr const bool compare_label{ true };

        file_ptr f{ open_r(fname) };
        read_part_header(f.get(), tag, as_label(label), compare_label);
        xs.emplace_back(tag, fname);

        std::ostringstream oss;
        oss << "[add_ctk_part, cnt_epoch] found " << ui_name << " file " << fname.string();
        ctk_log_debug(oss.str());
    }


    static
    auto ctk_parts(const std::filesystem::path& cnt) -> std::vector<tagged_file> {
        const auto flat_cnt{ fname_flat(cnt) };
        std::vector<tagged_file> xs;
        xs.reserve(10);

        add_ctk_part(fname_data(flat_cnt), file_tag::data, "raw3", "raw3/data", xs);
        add_ctk_part(fname_ep(flat_cnt), file_tag::ep, "raw3", "raw3/ep", xs);
        add_ctk_part(fname_chan(flat_cnt), file_tag::chan, "raw3", "raw3/chan", xs);
        add_ctk_part(fname_sample_count(flat_cnt), file_tag::sample_count, "eeph", "eeph/sample_count", xs);
        add_ctk_part(fname_electrodes(flat_cnt), file_tag::electrodes, "eeph", "eeph/electrodes", xs);
        add_ctk_part(fname_sampling_frequency(flat_cnt), file_tag::sampling_frequency, "eeph", "eeph/sampling_frequency", xs);
        add_ctk_part(fname_triggers(flat_cnt), file_tag::triggers, "evt ", "evt", xs);
        add_ctk_part(fname_cnt_type(flat_cnt), file_tag::cnt_type, "cntt", "cnt_type", xs);
        add_ctk_part(fname_info(flat_cnt), file_tag::info, "info", "info", xs);
        add_ctk_part(fname_history(flat_cnt), file_tag::history, "eeph", "eeph/history", xs);

        return xs;
    }


    // NB: the initialization order in this constructor is important
    epoch_reader_flat::epoch_reader_flat(const std::filesystem::path& cnt)
    : tokens{ ctk_parts(cnt) }
    , f_data{ open_r(data_file_name()) }
    , f_triggers{ open_r(trigger_file_name()) }
    , file_name{ cnt }
    , riff{ make_cnt_field_size(cnt_type()) }
    , a{ init() }
    , common{ f_data.get(), f_triggers.get(), a, riff.get(), part_header_size } {
    }

    // NB: the initialization order in this constructor is important
    epoch_reader_flat::epoch_reader_flat(const std::filesystem::path& cnt, const std::vector<tagged_file>& available)
    : tokens{ available }
    , f_data{ open_r(data_file_name()) }
    , f_triggers{ open_r(trigger_file_name()) }
    , file_name{ cnt }
    , riff{ make_cnt_field_size(cnt_type()) }
    , a{ init() }
    , common{ f_data.get(), f_triggers.get(), a, riff.get(), part_header_size } {
    }

    // NB: the initialization order in this constructor is important
    epoch_reader_flat::epoch_reader_flat(const epoch_reader_flat& x)
    : tokens{ x.tokens }
    , f_data{ open_r(data_file_name()) }
    , f_triggers{ open_r(trigger_file_name()) }
    , file_name{ x.file_name }
    , riff{ make_cnt_field_size(x.common.cnt_type()) }
    , a{ x.a }
    , common{ f_data.get(), f_triggers.get(), x.a, x.riff.get(), part_header_size } {
    }

    auto epoch_reader_flat::common_epoch_reader() const -> const epoch_reader_common& {
        return common;
    }

    // reflib
    auto epoch_reader_flat::writer_map() const -> riff_list {
        const auto t{ common.cnt_type() };
        const int64_t offset{ part_header_size };

        riff_list root{ root_chunk(t) };
        root.push_back(riff_node{ riff_text{ data_chunk(t, "eeph"), make_eeph_content(a) } });
        root.push_back(riff_node{ riff_text{ data_chunk(t, "info"), make_info_content(a) } });

        riff_list raw3{ list_chunk(t, "raw3") };
        raw3.push_back(riff_node{ riff_file{ data_chunk(t, "ep  "), ep_file_name(),   offset } });
        raw3.push_back(riff_node{ riff_file{ data_chunk(t, "chan"), chan_file_name(), offset } });
        raw3.push_back(riff_node{ riff_file{ data_chunk(t, "data"), data_file_name(), offset } });
        root.push_back(raw3);

        if (common.has_triggers()) {
            root.push_back(riff_node{ riff_file{ data_chunk(t, "evt "), trigger_file_name(), offset } });
        }

        return root;
    }
    
    auto epoch_reader_flat::data_file_name() const -> std::filesystem::path {
        return get_name(file_tag::data);
    }

    auto epoch_reader_flat::trigger_file_name() const -> std::filesystem::path {
        return get_name(file_tag::triggers);
    }

    auto epoch_reader_flat::ep_file_name() const -> std::filesystem::path {
        return get_name(file_tag::ep);
    }

    auto epoch_reader_flat::chan_file_name() const -> std::filesystem::path {
        return get_name(file_tag::chan);
    }

    auto epoch_reader_flat::get_name(file_tag id) const -> std::filesystem::path {
        const auto match_id = [id](const auto& x) -> bool { return x.id == id; };

        const auto first{ begin(tokens) };
        const auto last{ end(tokens) };
        const auto found{ std::find_if(first, last, match_id) };
        if (found == last) {
            std::ostringstream oss;
            oss << "[epoch_reader_flat::get_name, cnt_epoch] no file of type " << id;
            ctk_log_error(oss.str());
            throw api::v1::CtkData{ oss.str() };
        }

        return found->file_name;
    }

    auto epoch_reader_flat::cnt_type() const -> api::v1::RiffType {
        auto f{ open_r(get_name(file_tag::cnt_type)) };
        skip_part_header(f.get());

        std::string s("1234");
        read(f.get(), begin(s), end(s));
        return string2riff(s);
    }

    auto epoch_reader_flat::init() -> amorph {
        const auto [epoch, offsets]{ read_ep_flat(ep_file_name(), cnt_type()) };
        const auto [stamp, information]{ read_info_flat(get_name(file_tag::info), { 4, 4 }) };

        amorph x;
        x.header.StartTime = stamp;
        x.header.EpochLength = static_cast<measurement_count::value_type>(epoch);
        x.header.SamplingFrequency = read_sampling_frequency_flat(get_name(file_tag::sampling_frequency));
        x.header.Electrodes = read_electrodes_flat(get_name(file_tag::electrodes));
        x.order = read_chan_flat(chan_file_name());
        x.sample_count = read_sample_count_flat(get_name(file_tag::sample_count));
        x.epoch_ranges = offsets2ranges_flat(data_file_name(), offsets);
        x.trigger_range = flat_payload(trigger_file_name());
        x.history = read_history_flat(get_name(file_tag::history));
        x.information = information;

        validate_eeph_dimensions(x.sample_count, x.header.Electrodes.size(), x.order.size(), cast(x.order.size(), sint{}, ok{}));
        return x;
    }


    // NB: the initialization order in this constructor is important
    epoch_reader_riff::epoch_reader_riff(const std::filesystem::path& cnt)
    : f{ open_r(cnt) }
    , file_name{ cnt }
    , riff{ make_cnt_field_size(cnt_type()) }
    , a{ init() }
    , common{ f.get(), f.get(), a, riff.get(), 0 /* initial offset */ } {
    }

    epoch_reader_riff::epoch_reader_riff(const epoch_reader_riff& x)
    : f{ open_r(x.file_name) }
    , file_name{ x.file_name }
    , riff{ make_cnt_field_size(x.common.cnt_type()) }
    , a{ x.a }
    , common{ f.get(), f.get(), x.a, x.riff.get(), 0 /* initial offset */ } {
    }

    auto epoch_reader_riff::common_epoch_reader() const -> const epoch_reader_common& {
        return common;
    }
    
    auto epoch_reader_riff::embedded_files() const -> std::vector<std::string> {
        std::vector<std::string> result(a.user.size());

        std::transform(begin(a.user), end(a.user), begin(result), [](const auto& x) -> std::string { return x.label; });
        return result;
    }

    auto epoch_reader_riff::extract_embedded_file(const std::string& label, const std::filesystem::path& output) const -> void {
        if (file_name.string().empty()) {
            const std::string e{ "[epoch_reader_riff::extract_embedded_file, cnt_epoch] no output file name provided" };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }

        const auto first{ begin(a.user) };
        const auto last{ end(a.user) };
        const auto chunk{ std::find_if(first, last, [label](const auto& x) -> bool { return x.label == label; }) };
        if (chunk == last) {
            std::ostringstream oss;
            oss << "[epoch_reader_riff::extract_embedded_file, cnt_epoch] non existing chunk " << label;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkLimit{ e };
        }

        auto fout{ open_w(output) };
        copy_file_portion(f.get(), chunk->storage, fout.get());
    }

    auto epoch_reader_riff::cnt_type() const -> api::v1::RiffType {
        rewind(f.get());
        const auto chunk32{ read_root(f.get(), api::v1::RiffType::riff32) };
        if (is_root(chunk32)) {
            return api::v1::RiffType::riff32;
        }

        rewind(f.get());
        const auto chunk64{ read_root(f.get(), api::v1::RiffType::riff64) };
        if (is_root(chunk64)) {
            return api::v1::RiffType::riff64;
        }

        const std::string e{ "[epoch_reader_riff::cnt_type, cnt_epoch] neither RIFF nor RF64" };
        ctk_log_error(e);
        throw api::v1::CtkData{ e };
    }

    auto epoch_reader_riff::init() -> amorph {
        rewind(f.get());
        const auto x{ read_root(f.get(), string2riff(riff->root_id())) };
        if (!is_root(x)) {
            std::ostringstream oss;
            oss << "[epoch_reader_riff::init, cnt_epoch] not a root chunk " << as_string(x.id) << "/" << as_string(x.label);
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        return read_reflib_cnt(x, f.get());
    }


    auto operator<<(std::ostream& os, const amorph&) -> std::ostream& {
        return os;
    }

    auto sex2char(ctk::api::v1::Sex x) -> uint8_t {
        if (x == ctk::api::v1::Sex::Male) {
            return 'M';
        }
        else if (x == ctk::api::v1::Sex::Female) {
            return 'F';
        }

        return 0; // compatibility
    }

    auto char2sex(uint8_t x) -> ctk::api::v1::Sex {
        if (x == 'M' || x == 'm') {
            return ctk::api::v1::Sex::Male;
        }
        else if (x == 'F' || x == 'f') {
            return ctk::api::v1::Sex::Female;
        }

        return ctk::api::v1::Sex::Unknown;
    }


    auto hand2char(ctk::api::v1::Handedness x) -> uint8_t {
        if (x == ctk::api::v1::Handedness::Left) {
            return 'L';
        }
        else if (x == ctk::api::v1::Handedness::Right) {
            return 'R';
        }
        else if (x == ctk::api::v1::Handedness::Mixed) {
            return 'M';
        }

        return 0; // compatibility
    }

    auto char2hand(uint8_t x) -> ctk::api::v1::Handedness {
        if (x == 'L' || x == 'l') {
            return ctk::api::v1::Handedness::Left;
        }
        else if (x == 'R' || x == 'r') {
            return ctk::api::v1::Handedness::Right;
        }
        else if (x == 'M' || x == 'm') {
            return ctk::api::v1::Handedness::Mixed;
        }

        return ctk::api::v1::Handedness::Unknown;
    }

} /* namespace impl */ } /* namespace ctk */

