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

#include "test/util.h"
#include "api_data.h"
#include "container/file_reflib.h"

namespace ctk { namespace impl { namespace test {

using namespace ctk::api::v1;

auto binary_file(const std::vector<api::v1::Electrode>& xs) -> void {
    const std::filesystem::path temporary{ "container_objects.bin" };
    {
        file_ptr f{ open_w(temporary) };
        write_electrodes(f.get(), xs);
    }
    {
        file_ptr f{ open_r(temporary) };
        const auto ys{ read_electrodes(f.get()) };
        REQUIRE(xs == ys);
    }
    std::filesystem::remove(temporary);

}


TEST_CASE("electrodes round trip", "[consistency]") {
    std::vector<api::v1::Electrode> input;

    api::v1::Electrode e;
    REQUIRE_THROWS(parse_electrodes(make_electrodes_content({ e }), false));

    e.label = "label";
    REQUIRE_THROWS(parse_electrodes(make_electrodes_content({ e }), false));

    e.iscale = 0;
    REQUIRE_THROWS(parse_electrodes(make_electrodes_content({ e }), false));

    e.rscale = 0;
    REQUIRE_THROWS(parse_electrodes(make_electrodes_content({ e }), false));

    e.unit = "unit";
    REQUIRE(parse_electrodes(make_electrodes_content({ e }), false) == std::vector<api::v1::Electrode>{ e });

    input.push_back(e);

    // optional fields:
    // reference type status
    // 0         0    1         e1
    // 0         1    0         e2
    // 0         1    1         e1
    // 1         0    0         e3
    // 1         0    1         e3
    // 1         1    1         e3

    api::v1::Electrode e1{ e };
    api::v1::Electrode e2{ e };
    api::v1::Electrode e3{ e };

    e1.status = "status";
    input.push_back(e1);

    e1.type = "type";
    input.push_back(e1);

    e2.type = "type";
    input.push_back(e2);

    e3.reference = "reference";
    input.push_back(e3);

    e3.status = "status";
    input.push_back(e3);

    e3.type = "type";
    input.push_back(e3);

    const auto s{ make_electrodes_content(input) };
    const auto ascii_output{ parse_electrodes(s, false) };
    REQUIRE(input == ascii_output);
    binary_file(input);

    std::vector<api::v1::Electrode> compat;
    api::v1::Electrode e4{ e };
    api::v1::Electrode e5{ e };

    e4.status = "status";
    compat.push_back(e4);

    e5.type = "type";
    compat.push_back(e5);

    /* No (valid) label but exactly 5 columns enables    */
    /*   workaround for old files: it must be a reflabel */
    const auto s1{ make_electrodes_content(compat) };
    const auto sloppy{ parse_electrodes(s1, true) };
    REQUIRE(sloppy[0].reference == "STAT:stat");
    REQUIRE(sloppy[1].reference == "TYPE:type");
    REQUIRE(sloppy[0].label == e.label);
    REQUIRE(sloppy[0].iscale == e.iscale);
    REQUIRE(sloppy[0].rscale == e.rscale);
    REQUIRE(sloppy[0].unit == e.unit);
    REQUIRE(sloppy[1].label == e.label);
    REQUIRE(sloppy[1].iscale == e.iscale);
    REQUIRE(sloppy[1].rscale == e.rscale);
    REQUIRE(sloppy[1].unit == e.unit);

}

TEST_CASE("binary file electrodes round trip", "[consistency]") {
    const std::vector<api::v1::Electrode> xs{
        { "active label 1", "reference label 1", "a unit", 1.0, 1.0 },
        { "active label 2", "reference label 2", "another unit", 321.0, 0.12 },
        { "active label 3", "reference label 3", "a unit", 1.0, 1.0 }
    };
    binary_file(xs);
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


