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
#include <iostream>
#include "api_data.h"
#include "file/io.h"

namespace ctk { namespace impl {

    using label_type = uint32_t; // TODO: does not belong here

    auto as_string(label_type l) -> std::string;
    auto as_label(const std::string& s) -> label_type;


    enum class file_tag : uint8_t
    {
      // .cnt
        data
      , ep
      , chan
      , sample_count
      , electrodes
      , sampling_frequency
      , triggers
      , info
      , cnt_type
      , history
      // .evt
      , satellite_evt
      // cannary
      , length
    };

    auto operator<<(std::ostream&, const file_tag&) -> std::ostream&;

    auto is_part_header(FILE*, file_tag expected_tag, label_type expected_label, bool compare_label) -> bool;
    auto read_part_header(FILE*, file_tag expected_tag, label_type expected_label, bool compare_label) -> label_type;
    auto write_part_header(FILE*, file_tag, label_type) -> void;

    constexpr const int64_t part_header_size{
        sizeof(uint32_t) /* fourcc */ + sizeof(uint8_t) /* version */ + sizeof(file_tag) + sizeof(label_type)
    };

} /* namespace impl */ } /* namespace ctk */


