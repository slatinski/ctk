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

#include <cstdio>
#include <memory>
#include <optional>
#include <sstream>
#include <filesystem>

#include "exception.h"

namespace ctk { namespace impl {

    auto seek(FILE*, int64_t offset, int whence) -> bool;
    auto tell(FILE*) -> int64_t;
    auto maybe_tell(FILE*) -> int64_t;


    template<typename T>
    auto read(FILE* f, T result) -> T {
        if (std::fread(&result, sizeof(result), 1, f) != 1) {
            throw api::v1::CtkData{ "cannot read value" };
        }

        return result;
    }

    template<typename T>
    auto maybe_read(FILE* f, T result) -> std::optional<T> {
        if (std::fread(&result, sizeof(result), 1, f) != 1) {
            return std::nullopt;
        }

        return std::optional<T>{ result };
    }


    template<typename I>
    // [first, last) is expected to be a continguous range of memory
    auto read(FILE* f, I first, I last) -> void {
        if (first == last) {
            return;
        }
        const auto length{ static_cast<size_t>(last - first) };

        using T = typename std::iterator_traits<I>::value_type;
        T* start{ &*first };
        if (std::fread(start, sizeof(T), length, f) != length) {
            throw api::v1::CtkData{ "cannot read range" };
        }
    }

    template<typename T>
    auto write(FILE* f, T x) -> void {
        if (std::fwrite(&x, sizeof(T), 1, f) != 1) {
            throw api::v1::CtkData{ "cannot write value" };
        }
    }

    template<typename I>
    // [first, last) is expected to be a continguous range of memory
    auto write(FILE* f, I first, I last) -> void {
        if (first == last) {
            return;
        }
        const auto length{ static_cast<size_t>(last - first) };

        using T = typename std::iterator_traits<I>::value_type;
        const T* start{ &*first };
        if (std::fwrite(start, sizeof(T), length, f) != length) {
            throw api::v1::CtkData{ "cannot write range" };
        }
    }

    struct close_file
    {
        auto operator()(FILE* f) const -> void {
            if (!f) return;
            std::fclose(f);
        }
    };
    using file_ptr = std::unique_ptr<FILE, close_file>; // use different types for reading/writing?

    auto open_r(const std::filesystem::path&) -> file_ptr;
    auto open_w(const std::filesystem::path&) -> file_ptr;


    struct file_range
    {
        int64_t fpos;
        int64_t size;

        file_range(int64_t fpos, int64_t size);
        file_range();
        file_range(const file_range&) = default;
        file_range(file_range&&) = default;
        ~file_range() = default;

        auto operator=(const file_range&) -> file_range& = default;
        auto operator=(file_range&&) -> file_range& = default;

        friend auto operator==(const file_range&, const file_range&) -> bool = default;
        friend auto operator!=(const file_range&, const file_range&) -> bool = default;
    };
    auto operator<<(std::ostream&, const file_range&) -> std::ostream&;

    auto copy_file_portion(FILE* fin, file_range x, FILE* fout) -> void;
    auto content_size(const std::filesystem::path&) -> int64_t;

} /* namespace impl */ } /* namespace ctk */

