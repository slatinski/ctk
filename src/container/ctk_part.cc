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

#include <cassert>
#include <cstring>

#include "exception.h"
#include "container/ctk_part.h"
#include "container/io.h"
#include "container/api_io.h"

namespace ctk { namespace impl {

    static
    auto as_label_unchecked(const char* p) -> label_type {
        label_type l;
        memcpy(&l, p, sizeof(l));
        return l;
    }

    auto as_label(const std::string& s) -> label_type {
        constexpr const size_t size{ sizeof(label_type) };
        std::array<char, size> a;
        std::fill(begin(a), end(a), ' ');

        const auto amount{ std::min(s.size(), size) };
        const auto first{ begin(s) };
        const auto last{ first + static_cast<ptrdiff_t>(amount) };
        std::copy(first, last, begin(a));
        return as_label_unchecked(a.data());
    }

    auto as_string(label_type l) -> std::string {
        constexpr const size_t size{ sizeof(l) };
        std::array<char, size> a;

        memcpy(&a, &l, size);
        return std::string{ begin(a), end(a) };
    }


    auto write_part_header(FILE* f, file_tag tag, label_type label) -> void {
        constexpr const uint8_t fourcc[]{ 'c', 't', 'k', 'p' };
        constexpr const uint8_t version{ 1 };

        write(f, std::begin(fourcc), std::end(fourcc));
        write(f, version);
        write(f, tag);
        write(f, label);

        if (tell(f) != part_header_size) {
            throw api::v1::ctk_bug{ "write_part_header: invalid size / not the first record in a file" };
        }
    }

    enum class part_error { ok, not_ctk_part, unknown_version, invalid_tag };

    static
    auto read_part_header_impl(FILE* f, file_tag expected_tag, label_type expected_label, bool compare_label) -> std::pair<label_type, part_error> {
        uint8_t fourcc[]{ ' ', ' ', ' ', ' ' };
        read(f, std::begin(fourcc), std::end(fourcc));
        if (fourcc[0] != 'c' || fourcc[1] != 't' || fourcc[2] != 'k' || fourcc[3] != 'p') {
            return { 0, part_error::not_ctk_part };
        }

        const uint8_t version{ read(f, uint8_t{}) };
        if (version != 1) {
            return { 0, part_error::unknown_version };
        }

        const uint8_t id{ read(f, uint8_t{}) };
        constexpr const uint8_t max_id{ static_cast<uint8_t>(file_tag::length) };
        if (max_id <= id) {
            return { 0, part_error::invalid_tag };
        }

        const file_tag tag_id{ static_cast<file_tag>(id) };
        if (tag_id != expected_tag) {
            throw api::v1::ctk_bug{ "read_part_header_impl: invalid part file tag" };
        }

        const label_type chunk_id{ read(f, label_type{}) };
        if (compare_label && chunk_id != expected_label) {
            throw api::v1::ctk_bug{ "read_part_header_impl: invalid part file cnt label" };
        }

        return { chunk_id, part_error::ok };
    }

    auto is_part_header(FILE* f, file_tag expected_tag, label_type expected_label, bool compare_label) -> bool {
        const auto[x, e]{ read_part_header_impl(f, expected_tag, expected_label, compare_label) };
        return e == part_error::ok;
    }

    auto read_part_header(FILE* f, file_tag expected_tag, label_type expected_label, bool compare_label) -> label_type {
        const auto[x, e]{ read_part_header_impl(f, expected_tag, expected_label, compare_label) };
        if (e == part_error::ok) {
            return x;
        }
        else if (e == part_error::not_ctk_part) {
            throw api::v1::ctk_data{ "read_part_header: not a ctk part file" };
        }
        else if (e == part_error::unknown_version) {
            throw api::v1::ctk_data{ "read_part_header: unknown version" };
        }
        else if (e == part_error::invalid_tag) {
            throw api::v1::ctk_data{ "read_part_header: invalid file_tag enumeration" };
        }

        throw api::v1::ctk_bug{ "read_part_header: unknown error code" };
    }


    auto operator<<(std::ostream& os, const file_tag& x) -> std::ostream& {
        switch(x) {
            case file_tag::data: os << "data"; break;
            case file_tag::ep: os << "ep"; break;
            case file_tag::chan: os << "chan"; break;
            case file_tag::sample_count: os << "sample count"; break;
            case file_tag::electrodes: os << "electrodes"; break;
            case file_tag::sampling_frequency: os << "sampling frequency"; break;
            case file_tag::triggers: os << "triggers"; break;
            case file_tag::info: os << "info"; break;
            case file_tag::cnt_type: os << "cnt type"; break;
            case file_tag::history: os << "history"; break;
            case file_tag::satellite_evt: os << "evt data"; break;
            default: throw api::v1::ctk_bug{ "operator<<(file_tag): invalid" };
        }
        return os;
    }


} /* namespace impl */ } /* namespace ctk */


