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

#include "file/io.h"

#include <array>
#include <string>
#include <cassert>

#include "arithmetic.h"
#include "logger.h"


namespace ctk { namespace impl {

    auto seek(FILE* f, int64_t i, int whence) -> bool {
#ifdef WIN32
        return ::_fseeki64(f, i, whence) == 0;
#else
        return ::fseeko(f, cast(i, off_t{}, ok{}), whence) == 0;
#endif
    }


    auto tell(FILE* f) -> int64_t {
#ifdef WIN32
        const auto result{ ::_ftelli64(f) };
#else
        const auto result{ ::ftello(f) };
#endif
        if (result < 0) {
            const std::string e{ "[tell, io] can not obtain the current file position" };
            ctk_log_critical(e);
            throw api::v1::CtkBug{ e };
        }

        return result;
    }

    auto maybe_tell(FILE* f) -> int64_t {
#ifdef WIN32
        return ::_ftelli64(f);
#else
        return ::ftello(f);
#endif
    }



    auto open_r(const std::filesystem::path& fname) -> file_ptr {
        file_ptr p{ fopen(fname.string().c_str(), "rb") };
        if (!p) {
            std::ostringstream oss;
            oss << "[open_r, io] can not open " << fname.string() << " for reading";
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
        assert(p);
        return p;
    }


    auto open_w(const std::filesystem::path& fname) -> file_ptr {
        file_ptr p{ fopen(fname.string().c_str(), "wb") };
        if (!p) {
            std::ostringstream oss;
            oss << "[open_w, io] can not open " << fname.string() << " for writing";
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }
        assert(p);
        return p;
    }


    auto copy_file_portion(FILE* fin, file_range x, FILE* fout) -> void {
        if (!seek(fin, x.fpos, SEEK_SET)) {
            std::ostringstream oss;
            oss << "[copy_file_portion, io] can not seek to " << x.fpos;
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw api::v1::CtkData{ e };
        }

        constexpr const int64_t stride{ 1024 * 4 }; // arbitrary. TODO
        std::array<uint8_t, stride> buffer;

        ptrdiff_t chunk{ cast(std::min(x.size, stride), ptrdiff_t{}, ok{}) };
        while (0 < chunk) {
            read(fin, begin(buffer), begin(buffer) + chunk);
            write(fout, begin(buffer), begin(buffer) + chunk);

            x.size -= chunk;
            chunk = cast(std::min(x.size, stride), ptrdiff_t{}, ok{});
        }
    }

    auto content_size(const std::filesystem::path& x) -> int64_t {
        return cast(std::filesystem::file_size(x), int64_t{}, ok{});
    }

} /* namespace impl */ } /* namespace ctk */

