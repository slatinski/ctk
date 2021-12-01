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

#include <stdexcept>
#include <string>


namespace ctk { namespace api { namespace v1 {

    struct exception : public std::runtime_error
    {
        explicit exception(const std::string& msg)
        : std::runtime_error{ msg } {
        }

        explicit exception(const char* msg)
        : std::runtime_error{ msg } {
        }

        virtual ~exception() = default;
    };

    struct ctk_data : public exception
    {
        explicit ctk_data(const std::string& msg)
        : exception{ msg } {
        }

        explicit ctk_data(const char* msg)
        : exception{ msg } {
        }

        virtual ~ctk_data() = default;
    };

    struct ctk_limit : public exception
    {
        explicit ctk_limit(const std::string& msg)
        : exception{ msg } {
        }

        explicit ctk_limit(const char* msg)
        : exception{ msg } {
        }

        virtual ~ctk_limit() = default;
    };

    struct ctk_bug : public exception
    {
        explicit ctk_bug(const std::string& msg)
        : exception{ msg } {
        }

        explicit ctk_bug(const char* msg)
        : exception{ msg } {
        }

        virtual ~ctk_bug() = default;
    };

} /* namespace v1 */ } /* namespace api */ } /* namespace ctk */

