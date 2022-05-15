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
#include "file/cnt_reflib.h"

namespace ctk { namespace impl { namespace test {

using namespace ctk::api::v1;

auto binary_file(const std::vector<api::v1::Electrode>& xs) -> void {
    const std::filesystem::path temporary{ "container_objects.bin" };
    {
        file_ptr f{ open_w(temporary) };
        write_electrodes_bin(f.get(), xs);
    }
    {
        file_ptr f{ open_r(temporary) };
        const auto ys{ read_electrodes_bin(f.get()) };
        REQUIRE(xs == ys);
    }
    std::filesystem::remove(temporary);

}


TEST_CASE("electrodes round trip", "[consistency]") {
    std::vector<api::v1::Electrode> input;

    api::v1::Electrode e;
    e.ActiveLabel = "";
    e.Unit = "";
    e.IScale = 1;
    e.RScale = 1;

    REQUIRE_THROWS(parse_electrodes(make_electrodes_content({ e }), false));

    e.ActiveLabel = "label";
    REQUIRE_THROWS(parse_electrodes(make_electrodes_content({ e }), false));

    e.Unit = "unit";
    REQUIRE(parse_electrodes(make_electrodes_content({ e }), false) == std::vector<api::v1::Electrode>{ e });

    e.ActiveLabel = "";
    REQUIRE_THROWS(parse_electrodes(make_electrodes_content({ e }), false));
    e.ActiveLabel = "label";

    input.push_back(api::v1::Electrode{ "1", "ref" });

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

    e1.Status = "status";
    input.push_back(e1);

    e1.Type = "type";
    input.push_back(e1);

    e2.Type = "type";
    input.push_back(e2);

    e3.Reference = "reference";
    input.push_back(e3);

    e3.Status = "status";
    input.push_back(e3);

    e3.Type = "type";
    input.push_back(e3);

    const auto s{ make_electrodes_content(input) };
    const auto ascii_output{ parse_electrodes(s, false) };
    REQUIRE(input == ascii_output);
    binary_file(input);

    std::vector<api::v1::Electrode> compat;
    api::v1::Electrode e4{ e };
    api::v1::Electrode e5{ e };

    e4.Status = "status";
    compat.push_back(e4);

    e5.Type = "type";
    compat.push_back(e5);

    /* No (valid) label but exactly 5 columns enables    */
    /*   workaround for old files: it must be a reflabel */
    const auto s1{ make_electrodes_content(compat) };
    const auto sloppy{ parse_electrodes(s1, true) };
    REQUIRE(sloppy[0].Reference == "STAT:stat");
    REQUIRE(sloppy[1].Reference == "TYPE:type");
    REQUIRE(sloppy[0].ActiveLabel == e.ActiveLabel);
    REQUIRE(sloppy[0].IScale == e.IScale);
    REQUIRE(sloppy[0].RScale == e.RScale);
    REQUIRE(sloppy[0].Unit == e.Unit);
    REQUIRE(sloppy[1].ActiveLabel == e.ActiveLabel);
    REQUIRE(sloppy[1].IScale == e.IScale);
    REQUIRE(sloppy[1].RScale == e.RScale);
    REQUIRE(sloppy[1].Unit == e.Unit);
}


TEST_CASE("input validation", "[correct]") {
    // no active label
    api::v1::Electrode e;
    REQUIRE_THROWS(validate(e));

    // no unit
    e.ActiveLabel = "1";
    e.Unit = "";
    REQUIRE_THROWS(validate(e));

    // embedded zero in name
    e.ActiveLabel = "zero_s";
    e.ActiveLabel[4] = 0;
    e.Unit = "unit";
    REQUIRE_THROWS(validate(e));

    // white space in name
    e.ActiveLabel = "space l";
    e.Unit = "unit";
    REQUIRE_THROWS(validate(e));

    e.ActiveLabel = "1";
    e.Reference = "space r";
    REQUIRE_THROWS(validate(e));

    e.Reference = "ref";
    e.Unit = "space u";
    REQUIRE_THROWS(validate(e));

    e.Unit = "unit";
    e.Status = "space s";
    REQUIRE_THROWS(validate(e));

    e.Status = "status";
    e.Type = "space t";
    REQUIRE_THROWS(validate(e));

    // name too long
    e = api::v1::Electrode{};
    std::string s;
    s.resize(api::v1::sizes::eeph_electrode_active + 1);
    std::iota(begin(s), end(s), 'a');
    e.ActiveLabel = s;
    REQUIRE_THROWS(validate(e));
    e.ActiveLabel.pop_back();
    validate(e);

    s.resize(api::v1::sizes::eeph_electrode_reference + 1);
    std::iota(begin(s), end(s), 'a');
    e.Reference = s;
    REQUIRE_THROWS(validate(e));
    e.Reference.pop_back();
    validate(e);

    s.resize(api::v1::sizes::eeph_electrode_unit + 1);
    std::iota(begin(s), end(s), 'a');
    e.Unit = s;
    REQUIRE_THROWS(validate(e));
    e.Unit.pop_back();
    validate(e);

    s.resize(api::v1::sizes::eeph_electrode_status + 1);
    std::iota(begin(s), end(s), 'a');
    e.Status = s;
    REQUIRE_THROWS(validate(e));
    e.Status.pop_back();
    validate(e);

    s.resize(api::v1::sizes::eeph_electrode_type + 1);
    std::iota(begin(s), end(s), 'a');
    e.Type = s;
    REQUIRE_THROWS(validate(e));
    e.Type.pop_back();
    validate(e);

    // labels starts with [
    e.ActiveLabel = "[label";
    REQUIRE_THROWS(validate(e));

    // labels starts with ;
    e.ActiveLabel = ";label";
    REQUIRE_THROWS(validate(e));

    // infinite iscale
    e.ActiveLabel = "label";
    e.IScale = std::numeric_limits<double>::infinity();
    REQUIRE_THROWS(validate(e));

    // infinite rscale
    e.IScale = 1;
    e.RScale = std::numeric_limits<double>::infinity();
    REQUIRE_THROWS(validate(e));
}


TEST_CASE("binary file electrodes round trip", "[consistency]") {
    std::vector<api::v1::Electrode> xs{
        { "active label 1", "reference label 1", "a unit", 1.0, 1.0 },
        { "#active label 2", "reference label 2", "another unit", 321.0, 0.12 },
        { ";active label 3", "refe\re\nce label 3", "a unit", 1.0, 1.0 },
        { "", "", "", 0.0, 0.0 },
        { "active", "", "", 0.0, 0.0 },
        { "", "ref", "", 0.0, 0.0 },
        { "", "", "unit", 0.0, 0.0 },
        { "active", "ref", "", 0.0, 0.0 },
        { "active", "ref", "unit", 0.0, 0.0 },
        { "", "ref", "", 0.0, 0.0 },
        { "", "ref", "unit", 0.0, 0.0 }
    };

    api::v1::Electrode e;
    e.Status = "with a status";
    xs.push_back(e);

    e.Status = "";
    e.Type = "with a reference";
    xs.push_back(e);

    e.Status = "both";
    xs.push_back(e);

    e.Status = "embedded_zeroe_s";
    e.Status[8] = 0;
    e.Status[14] = 0;
    xs.push_back(e);

    binary_file(xs);
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


