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

    // base class
    struct CtkException : public std::runtime_error
    {
        explicit CtkException(const std::string& msg)
        : std::runtime_error{ msg } {
        }

        explicit CtkException(const char* msg)
        : std::runtime_error{ msg } {
        }

        virtual ~CtkException() = default;
    };

    // garbage data
    struct CtkData : public CtkException
    {
        explicit CtkData(const std::string& msg)
        : CtkException{ msg } {
        }

        explicit CtkData(const char* msg)
        : CtkException{ msg } {
        }

        virtual ~CtkData() = default;
    };

    // not possible to fullfill the request
    struct CtkLimit : public CtkException
    {
        explicit CtkLimit(const std::string& msg)
        : CtkException{ msg } {
        }

        explicit CtkLimit(const char* msg)
        : CtkException{ msg } {
        }

        virtual ~CtkLimit() = default;
    };

    // inconsistent state, detected a bug in this library
    struct CtkBug : public CtkException
    {
        explicit CtkBug(const std::string& msg)
        : CtkException{ msg } {
        }

        explicit CtkBug(const char* msg)
        : CtkException{ msg } {
        }

        virtual ~CtkBug() = default;
    };

} /* namespace v1 */ } /* namespace api */ } /* namespace ctk */

