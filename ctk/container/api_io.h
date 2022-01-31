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

#include "api_data.h"

namespace ctk { namespace impl {

    auto write_info(FILE*, const api::v1::Info&) -> void;
    auto write_time_series(FILE*, const api::v1::TimeSeries&) -> void;

    auto write_electrodes(FILE*, const std::vector<api::v1::Electrode>&) -> void;
    auto read_electrodes(FILE*) -> std::vector<api::v1::Electrode>;

} /* namespace impl */ } /* namespace ctk */

