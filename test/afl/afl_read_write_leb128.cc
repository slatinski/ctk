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

#include <iostream>
#include <cassert>
#include "ctk/file/leb128.h"
#include "ctk/exception.h"

auto generate_input_file(const std::filesystem::path& fname) -> void {
    using namespace ctk::impl;

    std::cerr << "writing " << fname << "\n";
    
    const std::vector<int64_t> xs{ std::numeric_limits<int64_t>::min(), -2, -1, 0, 1, 2, std::numeric_limits<int64_t>::max() };
    file_ptr f{ open_w(fname) };
    for(int64_t x : xs) {
        write_leb128(f.get(), x);
    }
}


auto read(const std::filesystem::path& fname) -> bool {
    using namespace ctk::impl;

    std::array<uint8_t, 10> scratch;
    const auto first{ begin(scratch) };
    const auto last{ end(scratch) };

    file_ptr f{ open_r(fname) };
    if (!seek(f.get(), 0, SEEK_END)) {
        std::cerr << "can not seek to end\n";
        return false;
    }
    const int64_t size{ tell(f.get()) };
    if (!seek(f.get(), 0, SEEK_SET)) {
        std::cerr << "can not seek to begin\n";
        return false;
    }

    int64_t x;

    while (tell(f.get()) < size) {
        try {
            x = read_leb128(f.get(), int64_t{});
        }
        // garbage input
        catch (const ctk::api::v1::CtkData& e) {
            std::cerr << " " << e.what() << "\n";
            return false;
        }

        const auto next{ encode_leb128(x, first, last) };
        const auto[y, next_decoded]{ decode_leb128(first, next, int64_t{}) };
        assert(x == y);
        assert(next == next_decoded);

        // is deterministic
        const auto ne{ encode_leb128(x, first, last) };
        const auto[z, nd]{ decode_leb128(first, ne, int64_t{}) };
        assert(x == z);
        assert(next == ne);
        assert(next == nd);
    }

    return true;
}

auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << "missing argument: file name\n";
        return 1;
    }
    // generates a seed file for the fuzzer
    //generate_input_file(argv[1]);
    //return 0;

    read(argv[1]);
    return 0;
}

