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

#include "ctk.h"
#include "test/util.h"
#include "test/wrap_libeep.h"
#include "test/wrap_thislib.h"
#include "ctk/container/file_reflib.h"

#include <vector>
#include <numeric>
#include <memory>
#include <iostream>
#include <chrono>

namespace ctk { namespace impl { namespace test {

struct execution_time
{
    std::chrono::microseconds eep;
    std::chrono::microseconds reflib;
    std::chrono::microseconds extended;

    execution_time()
    : eep{0}
    , reflib{0}
    , extended{0} {
    }

    execution_time(std::chrono::microseconds eep, std::chrono::microseconds reflib, std::chrono::microseconds extended)
    : eep{ eep }
    , reflib{ reflib }
    , extended{ extended } {
    }

    auto operator+=(const execution_time& x) -> execution_time& {
        eep += x.eep;
        reflib += x.reflib;
        extended += x.extended;
        return *this;
    }
};


struct sizes
{
    size_t eep;
    size_t reflib;
    size_t extended;
    size_t uncompressed;

    sizes()
    : eep{0}
    , reflib{0}
    , extended{0}
    , uncompressed{0} {
    }

    sizes(size_t eep, size_t reflib, size_t extended, size_t uncompressed)
    : eep{ eep }
    , reflib{ reflib }
    , extended{ extended }
    , uncompressed{ uncompressed } {
    }

    auto operator+=(const sizes& x) -> sizes& {
        eep += x.eep;
        reflib += x.reflib;
        extended += x.extended;
        uncompressed += x.uncompressed;
        return *this;
    }
};


using US = std::chrono::microseconds;

auto time_decoding(const compressed_epoch& e, const std::vector<int16_t>& order, int repetitions) -> execution_time {
    libeep eep{ e.length, order };
    libthis thislib{ e.length, order, int32_t{}, reflib{} };
    libthis thisextended{ e.length, order, int32_t{}, extended{} };

    const auto[v_eep, t_eep]{ eep.decode(e.data, repetitions) };
    const auto[v_ref, t_ref]{ thislib.decode(e.data, repetitions) };
    REQUIRE(v_eep == v_ref);

    const auto[b_ext, u0]{ thisextended.encode(v_eep) };
    const auto[v_ext, t_ext]{ thisextended.decode(b_ext, repetitions) };
    REQUIRE(v_eep == v_ext);

    const auto us_eep{ std::chrono::duration_cast<US>(t_eep) };
    const auto us_ref{ std::chrono::duration_cast<US>(t_ref) };
    const auto us_ext{ std::chrono::duration_cast<US>(t_ext) };

    return execution_time{ us_eep, us_ref, us_ext };
}


auto time_encoding(const compressed_epoch& e, const std::vector<int16_t>& order, int repetitions) -> std::pair<execution_time, sizes> {
    libeep eep{ e.length, order };
    libthis thislib{ e.length, order, int32_t{}, reflib{} };
    libthis thisextended{ e.length, order, int32_t{}, extended{} };
    const auto[input, u0]{ thislib.decode(e.data) };

    const auto[b_eep, t_eep]{ eep.encode(input, repetitions) };
    const auto[b_ref, t_ref]{ thislib.encode(input, repetitions) };
    const auto[b_ext, t_ext]{ thisextended.encode(input, repetitions) };

    const auto us_eep{ std::chrono::duration_cast<US>(t_eep) };
    const auto us_ref{ std::chrono::duration_cast<US>(t_ref) };
    const auto us_ext{ std::chrono::duration_cast<US>(t_ext) };

    const size_t s_eep{ b_eep.size() };
    const size_t s_ref{ b_ref.size() };
    const size_t s_ext{ b_ext.size() };
    const size_t s_unc{ input.size() * sizeof(int32_t) };

    return { { us_eep, us_ref, us_ext }, { s_eep, s_ref, s_ext, s_unc } };
}

auto time_decoding_reencoded(const compressed_epoch& e, const std::vector<int16_t>& order, int repetitions) -> std::pair<execution_time, double> {
    libeep eep{ e.length, order };
    libthis thislib{ e.length, order, int32_t{}, reflib{} };

    const auto[input, u0]{ thislib.decode(e.data) };
    const auto[bytes, u1]{ thislib.encode(input) };

    const auto[v_eep, t_eep]{ eep.decode(bytes, repetitions) };
    const auto[v_ref, t_ref]{ thislib.decode(bytes, repetitions) };
    REQUIRE(v_eep == v_ref);
    REQUIRE(v_eep == input);

    const US us_eep{ std::chrono::duration_cast<US>(t_eep) };
    const US us_ref{ std::chrono::duration_cast<US>(t_ref) };
    const US us_ext{0};

    double ratio{ 1.0 };
    if (!input.empty()) {
        ratio = double(bytes.size()) / double(input.size() * sizeof(int32_t));
    }

    return { execution_time{ us_eep, us_ref, us_ext }, ratio };
}


auto print_unit_eep(const execution_time& x) -> std::string {
    const double ref_eep{ 100.0 * x.reflib.count() / x.eep.count() };
    const double ext_eep{ 100.0 * x.extended.count() / x.eep.count() };

    std::ostringstream oss;
    oss << "l[r" << d2s(ref_eep) << "%, e" << d2s(ext_eep) << "%] |";

    return oss.str();
}

auto print_unit_eep(const sizes& x) -> std::string {
    const double ref_eep{ 100.0 * x.reflib / x.eep };
    const double ext_eep{ 100.0 * x.extended / x.eep };

    std::ostringstream oss;
    oss << "l[r" << d2s(ref_eep) << "%, e" << d2s(ext_eep) << "%] |";

    return oss.str();
}


auto print_unit_eep_redecoded(const execution_time& initial, const execution_time& re) -> std::string {
    const double eep_eep{ 100.0 * re.eep.count() / initial.eep.count() };
    const double ref_eep{ 100.0 * re.reflib.count() / initial.eep.count() };

    std::ostringstream oss;
    oss << "[l" << d2s(eep_eep) << "%, r" << d2s(ref_eep) << "%] |";

    return oss.str();
}



auto print(const execution_time& dec, const execution_time& enc, const execution_time& re, const sizes& storage, std::string msg = std::string{}) -> void {
    std::cerr << msg
              << " dec: " << print_unit_eep(dec)
              << " enc: " << print_unit_eep(enc)
              << " redec: " << print_unit_eep_redecoded(dec, re)
              << " sz: " << print_unit_eep(storage)
              << "\n";
}

auto print(const execution_time& dec, const execution_time& enc, const execution_time& re, const sizes& storage, double size) -> void {
    std::cerr << " dec: " << print_unit_eep(dec)
              << " enc: " << print_unit_eep(enc)
              << " redec: " << print_unit_eep_redecoded(dec, re)
              << " sz: " << print_unit_eep(storage)
              << " c/u: " << d2s(size * 100.0) << "%"
              << "\n";
}


auto run(epoch_reader_riff& reader, int repetitions) -> std::tuple<execution_time, execution_time, execution_time, sizes> {
    execution_time t_decoder;
    execution_time t_encoder;
    execution_time t_decoder_re;
    double sz_comp_uncomp{ 0 };
    sizes storage;
    const auto& order{ reader.data().order() };

    const epoch_count epochs{ reader.data().count() };
    for (epoch_count i{ 0 }; i < epochs; ++i) {
        const compressed_epoch ce{ reader.data().epoch(i) };
        if (ce.data.empty()) {
            std::cerr << "cnt: cannot read epoch " << (i + epoch_count{ 1 }) << "/" << epochs << "\n";
            continue;
        }

        t_decoder += time_decoding(ce, order, repetitions);
        const auto[t_e, s_e]{ time_encoding(ce, order, repetitions) };
        t_encoder += t_e;
        storage += s_e;
        const auto[tdr, sz]{ time_decoding_reencoded(ce, order, repetitions) };
        t_decoder_re += tdr;
        sz_comp_uncomp += sz;
    }

    print(t_decoder, t_encoder, t_decoder_re, storage, sz_comp_uncomp / sint(epochs));

    return { t_decoder, t_encoder, t_decoder_re, storage };
}


TEST_CASE("compepoch", "[compatibility] [consistency] [performance]") {
    const size_t fname_width{ 7 };
	const int repetitions{ 1 };
    constexpr const bool is_broken{ false };
    std::cerr << repetitions << " repetitions per epoch\n";

    execution_time t_decoder;
    execution_time t_encoder;
    execution_time t_decoder_re;
    sizes sz;
    size_t i{0};

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
		try {
			std::cerr << s2s(fname, fname_width);
            epoch_reader_riff reader{ fname, is_broken };

            const auto[d, e, r, s]{ run(reader, repetitions) };
            t_decoder += d;
            t_encoder += e;
            t_decoder_re += r;
            sz += s;

            if (i % 10 == 0) {
                print(t_decoder, t_encoder, t_decoder_re, sz, s2s("AVG", fname_width));
                std::cerr << "\n";
            }
            ++i;
		}
        catch (const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
	}

    print(t_decoder, t_encoder, t_decoder_re, sz, s2s("TOTAL", fname_width));
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
