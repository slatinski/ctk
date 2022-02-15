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

#include <chrono>

#include "test/util.h"
#include "ctk.h"
#include "file/cnt_reflib.h"

namespace ctk { namespace impl { namespace test {


auto compare_readers(const std::string& fname) -> void {
    ctk::impl::cnt_reader_reflib_riff reader_direct{ fname };
    ctk::CntReaderReflib reader_api{ fname };

    const auto samples{ reader_direct.sample_count() };
    const auto samples_api{ reader_api.sampleCount() };
    REQUIRE(samples == ctk::impl::measurement_count{ samples_api });

    const auto triggers{ reader_direct.triggers() };
    const auto triggers_api{ reader_api.triggers() };
    REQUIRE(triggers == triggers_api);

    const auto desc{ reader_direct.description() };
    const auto desc_api{ reader_api.description() };
    REQUIRE(desc.epoch_length == desc_api.epoch_length);
    REQUIRE(desc.sampling_frequency == desc_api.sampling_frequency);
    REQUIRE(desc.start_time == desc_api.start_time);
    REQUIRE(desc.electrodes == desc_api.electrodes);

    REQUIRE(reader_direct.history() == reader_api.history());
    REQUIRE(reader_direct.file_version().major == reader_api.fileVersion().major);
    REQUIRE(reader_direct.file_version().minor == reader_api.fileVersion().minor);

    // missing dob
    REQUIRE(reader_direct.information() == reader_api.information());

    const sint chunk_api{ 1 };
    const measurement_count chunk{ chunk_api };

    for (measurement_count i{ 0 }; i < samples; ++i) {
        const auto v_direct{ reader_direct.range_column_major(i, chunk) };

        const sint i_api{ i };
        const auto v_api{ reader_api.rangeColumnMajorInt32(i_api, chunk_api) };
        REQUIRE(v_direct == v_api);
    }

    std::cerr << " ok\n";
}


TEST_CASE("compare readers", "[consistency] [read]") {
    const size_t fname_width{ 20 };
    input_txt input;
    std::string fname{ input.next() };

    while (!fname.empty()) {
        try {
            std::cerr << s2s(fname, fname_width);
            compare_readers(fname);
        }
        catch(const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
    }
}


auto read_direct(const std::string& fname) -> int64_t {
    ctk::impl::cnt_reader_reflib_riff reader{ fname };
    const auto samples{ reader.sample_count() };
    const auto electrodes{ reader.channels() };
    const measurement_count chunk{ 1 };

    int64_t accessible{ 0 };
    for (measurement_count i{ 0 }; i < samples; ++i) {
        const auto v{ reader.range_column_major(i, chunk) };
        if (v.size() == electrodes.size()) {
            ++accessible;
        }
    }

    const auto triggers{ reader.triggers() };
    const auto count{ ctk::impl::vsize(triggers) };

    return accessible + count;
}

auto read_api(const std::string& fname) -> int64_t {
    ctk::CntReaderReflib reader{ fname };
    const auto samples{ reader.sampleCount() };
    const auto electrodes{ reader.description().electrodes };
    const int64_t chunk{ 1 };

    int64_t accessible{ 0 };
    for (int64_t i{ 0 }; i < samples; ++i) {
        const auto v{ reader.rangeColumnMajorInt32(i, chunk) };
        if (v.size() == electrodes.size()) {
            ++accessible;
        }
    }

    const auto triggers{ reader.triggers() };
    const auto count{ ctk::impl::vsize(triggers) };

    return accessible + count;
}

auto test_reader_speed(const std::string& fname) -> void {
    read_api(fname); // warm up the hdd if applicable

    const auto direct_s{ std::chrono::steady_clock::now() };
    const auto consumed_direct{ read_direct(fname) };
    const auto direct_e{ std::chrono::steady_clock::now() };

    const auto api_s{ std::chrono::steady_clock::now() };
    const auto consumed_api{ read_api(fname) };
    const auto api_e{ std::chrono::steady_clock::now() };

    REQUIRE(consumed_direct == consumed_api);

    std::chrono::microseconds direct_t{ std::chrono::duration_cast<std::chrono::microseconds>(direct_e - direct_s) };
    std::chrono::microseconds api_t{ std::chrono::duration_cast<std::chrono::microseconds>(api_e - api_s) };

    const double api_direct{ 100.0 * api_t.count() / direct_t.count() };
    std::cerr << "api/direct " << d2s(api_direct) << "%\n";
}


TEST_CASE("test reader speed", "[performance] [read]") {
    const size_t fname_width{ 20 };
    input_txt input;
    std::string fname{ input.next() };

    while (!fname.empty()) {
        try {
            std::cerr << s2s(fname, fname_width);
            test_reader_speed(fname);
        }
        catch(const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
    }
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


