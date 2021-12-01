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

#include "ctk.h"
#include <iostream>

auto column_major_first_matrix_layout() -> void {
    const int64_t rows{ 4 };
    const int64_t columns{ 3 };

    // column major first input matrix layout
    const std::vector<int16_t> input {
        11, 21, 31, // measurement at time point 1: sample data for sensors 1, 2 and 3
        12, 22, 32, // measurement at time point 2: sample data for sensors 1, 2 and 3
        13, 23, 33, // measurement at time point 3: sample data for sensors 1, 2 and 3
        14, 24, 34  // measurement at time point 4: sample data for sensors 1, 2 and 3
    };
    std::vector<int16_t> output; // not needed if the bytes are decoded back into the input vector
    std::vector<uint8_t> bytes;

    ctk::CompressInt16 encoder;
    if (!encoder.sensors(columns)) {
        std::cerr << "cannot set the sensor count for the encoder\n";
        return;
    }

    ctk::DecompressInt16 decoder;
    if (!decoder.sensors(columns)) {
        std::cerr << "cannot set the sensor count for the decoder\n";
        return;
    }

    // data processing
    for (int i = 0; i < 3; ++i) {
        // producer
        bytes = encoder.column_major(input, rows);
        if (bytes.empty()) {
            std::cerr << "encoding failed\n";
            return;
        }

        // ...

        // consumer
        output = decoder.column_major(bytes, rows);
        if (output.empty()) {
            std::cerr << "decoding failed\n";
            return;
        }

        if (input != output) {
            std::cerr << "encoding/decoding roundtrip failed\n";
            return;
        }
    }

    std::cerr << "column major ok\n";
}


auto row_major_first_matrix_layout() -> void {
    const int64_t columns{ 4 };
    const int64_t rows{ 3 };

    // row major first input matrix layout
    const std::vector<int64_t> input {
        11, 12, 13, 14, // sample data at time points 1, 2, 3 and 4 for sensor 1
        21, 22, 23, 24, // sample data at time points 1, 2, 3 and 4 for sensor 2
        31, 32, 33, 34  // sample data at time points 1, 2, 3 and 4 for sensor 3
    };
    std::vector<int64_t> output; // not needed if the bytes are decoded back into the input vector
    std::vector<uint8_t> bytes;

    ctk::CompressInt64 encoder;
    encoder.sensors(rows);

    ctk::DecompressInt64 decoder;
    decoder.sensors(rows);

    // data processing
    for (int i = 0; i < 3; ++i) {
        // producer
        bytes = encoder.row_major(input, columns);
        if (bytes.empty()) {
            std::cerr << "encoding failed\n";
            return;
        }

        // ...

        // consumer
        output = decoder.row_major(bytes, columns);
        if (output.empty()) {
            std::cerr << "decoding failed\n";
            return;
        }

        if (input != output) {
            std::cerr << "encoding/decoding roundtrip failed\n";
            return;
        }
    }

    std::cerr << "row major ok\n";
}


int main(int, char* []) {
    try {
        column_major_first_matrix_layout();
        row_major_first_matrix_layout();
    }
    catch(const ctk::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}

