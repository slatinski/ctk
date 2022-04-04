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

#include "file/ctk_part.h"

#include <array>
#include <cassert>
#include <cstring>

#include "exception.h"
#include "file/io.h"

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
            const std::string e{ "[write_part_header, ctk_part] invalid size or not the first record in a file" };
            throw api::v1::CtkBug{ e };
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
            std::ostringstream oss;
            oss << "[read_part_header_impl, ctk_part] invalid part file tag " << tag_id << ", expected " << expected_tag;
            const auto e{ oss.str() };
            throw api::v1::CtkBug{ e };
        }

        const label_type chunk_id{ read(f, label_type{}) };
        if (compare_label && chunk_id != expected_label) {
            std::ostringstream oss;
            oss << "[read_part_header_impl, ctk_part] invalid part file cnt label " << chunk_id << ", expected " << expected_label;
            const auto e{ oss.str() };
            throw api::v1::CtkBug{ e };
        }

        return { chunk_id, part_error::ok };
    }

    auto skip_part_header(FILE* f) -> void {
        if (seek(f, part_header_size, SEEK_SET)) {
            return;
        }

        const std::string msg{ "[skip_part_header, ctk_part] can not seek" };
        throw api::v1::CtkData{ msg };
    }

    auto read_part_header(FILE* f, file_tag expected_tag, label_type expected_label, bool compare_label) -> label_type {
        const auto[x, e]{ read_part_header_impl(f, expected_tag, expected_label, compare_label) };
        if (e == part_error::ok) {
            return x;
        }
        else if (e == part_error::not_ctk_part) {
            const std::string msg{ "[read_part_header, ctk_part] not a ctk part file" };
            throw api::v1::CtkData{ msg };
        }
        else if (e == part_error::unknown_version) {
            const std::string msg{ "[read_part_header, ctk_part] unknown version" };
            throw api::v1::CtkData{ msg };
        }
        else if (e == part_error::invalid_tag) {
            const std::string msg{ "[read_part_header, ctk_part] invalid file_tag enumeration" };
            throw api::v1::CtkData{ msg };
        }

        const std::string msg{ "[read_part_header, ctk_part] unknown error code" };
        throw api::v1::CtkBug{ msg };
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
            default: {
                const std::string e{ "[operator<<(file_tag), ctk_part] invalid" };
                throw api::v1::CtkBug{ e };
            }
        }
        return os;
    }


} /* namespace impl */ } /* namespace ctk */


