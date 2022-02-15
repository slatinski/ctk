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
#include "ctk/file/cnt_epoch.h"


using namespace ctk::api::v1;
using namespace ctk::impl;


auto generate_input_file(const std::string& fname) -> void {
    std::cerr << "writing " << fname << "\n";

    Electrode e{ "1", "ref", "uV" };
    e.type = "none";
    e.status = "unknown";

    amorph x;
    x.sample_count = measurement_count{ 12 }; 
    x.header.sampling_frequency = 2048.12; 
    x.header.epoch_length = 1024; 
    x.header.electrodes = { e, e, e, e };
    x.history = "no history";

    const std::string xs{ make_eeph_content(x) };

    file_ptr f{ open_w(fname) };
    write(f.get(), begin(xs), end(xs));
}


auto read(const std::string& fname) -> void {
    file_ptr f{ open_r(fname) };
    const int64_t fsize{ file_size(f.get()) };

    std::string xs;
    xs.resize(fsize);
    read(f.get(), begin(xs), end(xs));
    call_parse_eeph(xs);
}


auto ignore_expected() -> void {
    try {
        throw;
    }
    // thrown by stl
    catch (const std::bad_alloc& e) {
        std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
    }
    catch (const std::length_error& e) {
        std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
    }
    /*
    catch (const std::ios_base::failure& e) {
        std::cerr << " " << e.what() << "\n"; // string io
    }
    */
    catch (const std::invalid_argument& e) {
        std::cerr << " " << e.what() << "\n"; // garbage data. stol, stod
    }
    catch (const std::out_of_range& e) {
        std::cerr << " " << e.what() << "\n"; // garbage data. stol, stod
    }
    // thrown by ctk
    catch (const ctk_limit& e) {
        std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
    }
    catch (const ctk_data& e) {
        std::cerr << " " << e.what() << "\n"; // garbage data
    }

    // not expected
    catch (const ctk_bug& e) {
        std::cerr << " " << e.what() << "\n"; // bug in this library
        throw;
    }
    catch (const std::exception& e) {
        std::cerr << " " << e.what() << "\n"; // bug in this library
        throw;
    }
}


auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << "missing argument: file name\n";
        return 1;
    }
    // generates a seed file for the fuzzer
    //generate_input_file(argv[1]);
    //return 0;


    try {
        read(argv[1]);
    }
    catch (...) {
        ignore_expected();
        return 1;
    }

    return 0;
}


