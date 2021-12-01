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

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../catch.hpp"

#include "container/api_io.h"
#include "container/io.h"

namespace ctk { namespace impl { namespace test {

TEST_CASE("electrodes", "[consistency]") {
    api::v1::Electrode e{ "fpx", "", "uv", 0.1, 1.0 };
    std::vector<api::v1::Electrode> input(10);
    std::fill(begin(input), end(input), e);

    {
        auto f{ open_w("electrodes.bin") };
        write_electrodes(f.get(), input);
    }
    {
        auto f{ open_r("electrodes.bin") };
        auto output{ read_electrodes(f.get()) };
        REQUIRE(output == input);
    }

    if (std::remove("electrodes.bin") != 0) {
        // cannot delete
    }
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */

