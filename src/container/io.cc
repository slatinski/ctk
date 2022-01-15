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

#include <sstream>
#include <cassert>

#include "container/io.h"

namespace ctk { namespace impl {

    auto seek(FILE* f, int64_t i, int whence) -> bool {
#ifdef WIN32
        return ::_fseeki64(f, i, whence) == 0;
#else
        return ::fseeko(f, i, whence) == 0;
#endif
    }


    auto tell(FILE* f) -> int64_t {
#ifdef WIN32
        const auto result{ ::_ftelli64(f) };
#else
        const auto result{ ::ftello(f) };
#endif
        if (result < 0) {
            throw api::v1::ctk_bug{ "tell: can not tell position" };
        }

        return result;
    }


    auto open_r(const std::filesystem::path& fname) -> file_ptr {
        file_ptr p{ fopen(fname.string().c_str(), "rb") };
        if (!p) {
            std::ostringstream oss;
            oss << "open_r: cannot open " << fname << " for reading";
            throw api::v1::ctk_data{ oss.str() };
        }
        assert(p);
        return p;
    }


    auto open_w(const std::filesystem::path& fname) -> file_ptr {
        file_ptr p{ fopen(fname.string().c_str(), "wb") };
        if (!p) {
            std::ostringstream oss;
            oss << "open_w: can not open " << fname << " for writing";
            throw api::v1::ctk_data{ oss.str() };
        }
        assert(p);
        return p;
    }

} /* namespace impl */ } /* namespace ctk */

