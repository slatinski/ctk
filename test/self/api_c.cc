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

#include <filesystem>
#include "api_c.h"

namespace ctk { namespace impl { namespace test {

TEST_CASE("writer basics", "[consistency]") {
    const char* delme_cnt{ "delme.cnt" };
    ctk_reflib_writer* x{ nullptr };
    double matrix_one[]{ 0 };
    double matrix_two[]{ 0, 0 };
    timespec stamp;
    timespec_get(&stamp, TIME_UTC);

    std::filesystem::remove(delme_cnt);

    // no file name
    x = ctk_reflib_writer_make(nullptr, 0);
    REQUIRE(!x);
    ctk_reflib_writer_dispose(x);

    // no file name
    x = ctk_reflib_writer_make("", 1);
    REQUIRE(!x);
    ctk_reflib_writer_dispose(x);

    // no metadata, no data
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    ctk_reflib_writer_close(x);
    ctk_reflib_writer_dispose(x);
    REQUIRE(!std::filesystem::exists(delme_cnt));

    // no metadata, data
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_one, sizeof(matrix_one)) == EXIT_FAILURE); // invalid dimensions
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_two, sizeof(matrix_two)) == EXIT_FAILURE); // invalid dimensions
    ctk_reflib_writer_close(x);
    ctk_reflib_writer_dispose(x);
    REQUIRE(!std::filesystem::exists(delme_cnt));

    // metadata, no corresponding data
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "1", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_sampling_frequency(x, 256) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_start_time(x, &stamp) == EXIT_SUCCESS);
    ctk_reflib_writer_close(x);
    ctk_reflib_writer_dispose(x);
    REQUIRE(!std::filesystem::exists(delme_cnt));

    // invalid metadata, ref is optional
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, nullptr, "ref") == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "1", nullptr) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_electrode(x, nullptr, "ref", "uV", 1, 1) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_electrode(x, "1", nullptr, "uV", 1, 1) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_electrode(x, "1", "ref", nullptr, 1, 1) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_start_time(x, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_sampling_frequency(x, -1) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_epoch_length(x, -1) == EXIT_FAILURE);
    ctk_reflib_writer_close(x);
    ctk_reflib_writer_dispose(x);
    REQUIRE(!std::filesystem::exists(delme_cnt));

    // invalid matrix dimensions
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "1", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "2", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "3", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_sampling_frequency(x, 256) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_start_time(x, &stamp) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_one, sizeof(matrix_one)) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_two, sizeof(matrix_two)) == EXIT_FAILURE);
    ctk_reflib_writer_close(x);
    ctk_reflib_writer_dispose(x);
    REQUIRE(!std::filesystem::exists(delme_cnt));

    // minimum metadata, minimum data, success
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "1", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_sampling_frequency(x, 256) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_start_time(x, &stamp) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_one, sizeof(matrix_one)) == EXIT_SUCCESS);
    ctk_reflib_writer_close(x);
    ctk_reflib_writer_dispose(x);
    REQUIRE(std::filesystem::exists(delme_cnt));
    std::filesystem::remove(delme_cnt);

    // minimum metadata, data samples of different size, success
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "1", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_sampling_frequency(x, 256) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_start_time(x, &stamp) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_one, sizeof(matrix_one)) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_two, sizeof(matrix_two)) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_row_major(x, matrix_one, sizeof(matrix_one)) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_row_major(x, matrix_two, sizeof(matrix_two)) == EXIT_SUCCESS);
    ctk_reflib_writer_close(x);
    ctk_reflib_writer_dispose(x);
    REQUIRE(std::filesystem::exists(delme_cnt));
    std::filesystem::remove(delme_cnt);

    // no success after close
    x = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(x);
    ctk_reflib_writer_close(x);
    REQUIRE(ctk_reflib_writer_electrode_uv(x, "1", "ref") == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_sampling_frequency(x, 256) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_start_time(x, &stamp) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_column_major(x, matrix_one, 1) == EXIT_FAILURE);
    ctk_reflib_writer_dispose(x);
    REQUIRE(!std::filesystem::exists(delme_cnt));

    // invalid pointer
    REQUIRE(ctk_reflib_writer_electrode_uv(nullptr, "1", "ref") == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_electrode(nullptr, "1", "ref", "uV", 1, 1) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_sampling_frequency(nullptr, 256) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_epoch_length(nullptr, 256) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_start_time(nullptr, &stamp) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_column_major(nullptr, nullptr, 0) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_column_major_int32(nullptr, nullptr, 0) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_row_major(nullptr, nullptr, 0) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_row_major_int32(nullptr, nullptr, 0) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_v4(nullptr, nullptr, 0) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_trigger(nullptr, 0, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_hospital(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_physician(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_technician(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_subject_id(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_subject_name(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_subject_address(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_subject_phone(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_subject_sex(nullptr, 0) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_subject_handedness(nullptr, 0) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_subject_dob(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_machine_make(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_machine_model(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_machine_sn(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_test_name(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_test_serial(nullptr, nullptr) == EXIT_FAILURE);
    REQUIRE(ctk_reflib_writer_comment(nullptr, nullptr) == EXIT_FAILURE);
}

auto equal_dbl(const double* x, const double* y, size_t size) -> bool {
    const auto last_x{ x + size };
    const auto last_y{ y + size };
    const auto [mx, my]{ std::mismatch(x, last_x, y, last_y) };
    return mx == last_x && my == last_y;
}

TEST_CASE("write/read", "[consistency]") {
    const char* delme_cnt{ "delme.cnt" };
    ctk_reflib_writer* writer{ nullptr };
    ctk_reflib_reader* reader{ nullptr };
    const double matrix_cm[] = {
        11, 21, 31, 41,
        12, 22, 32, 42
    };
    constexpr const size_t size_m = sizeof(matrix_cm) / sizeof(matrix_cm[0]);
    const double matrix_rm[] = {
        13, 14,
        23, 24,
        33, 34,
        43, 44
    };
    const double rm_0[] = { 13, 23, 33, 43 };
    const double rm_1[] = { 14, 24, 34, 44 };
    double matrix[size_m] = { 0 };
    timespec now;
    timespec_get(&now, TIME_UTC);

    std::filesystem::remove(delme_cnt);

    writer = ctk_reflib_writer_make(delme_cnt, 0);
    REQUIRE(writer);
    REQUIRE(ctk_reflib_writer_electrode_uv(writer, "1", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_electrode_uv(writer, "2", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_electrode_uv(writer, "3", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_electrode_uv(writer, "4", "ref") == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_sampling_frequency(writer, 256) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_start_time(writer, &now) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_column_major(writer, matrix_cm, size_m) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_column_major(writer, matrix_cm, 3) == EXIT_FAILURE); // invalid dimensions
    REQUIRE(ctk_reflib_writer_row_major(writer, matrix_rm, size_m) == EXIT_SUCCESS);
    REQUIRE(ctk_reflib_writer_row_major(writer, matrix_rm, 1) == EXIT_FAILURE); // invalid dimensions
    REQUIRE(ctk_reflib_writer_column_major(writer, matrix_cm, size_m) == EXIT_SUCCESS);
    ctk_reflib_writer_close(writer);
    ctk_reflib_writer_dispose(writer);
    REQUIRE(std::filesystem::exists(delme_cnt));

    reader = ctk_reflib_reader_make(delme_cnt);
    REQUIRE(reader);
    REQUIRE(ctk_reflib_reader_electrode_count(reader) == 4);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_label(reader, 0), "1") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_label(reader, 1), "2") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_label(reader, 2), "3") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_label(reader, 3), "4") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_reference(reader, 0), "ref") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_reference(reader, 1), "ref") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_reference(reader, 2), "ref") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_reference(reader, 3), "ref") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_unit(reader, 0), "uV") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_unit(reader, 1), "uV") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_unit(reader, 2), "uV") == 0);
    REQUIRE(strcmp(ctk_reflib_reader_electrode_unit(reader, 3), "uV") == 0);
    REQUIRE(ctk_reflib_reader_sampling_frequency(reader) == 256);
    timespec stamp = ctk_reflib_reader_start_time(reader);
    REQUIRE(now.tv_sec == stamp.tv_sec);
    REQUIRE(now.tv_nsec == stamp.tv_nsec);
    REQUIRE(ctk_reflib_reader_sample_count(reader) == 6);

    // column major of size 1
    REQUIRE(ctk_reflib_reader_column_major(reader, 0, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm, 4));

    REQUIRE(ctk_reflib_reader_column_major(reader, 1, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm + 4, 4));

    REQUIRE(ctk_reflib_reader_column_major(reader, 2, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, rm_0, 4));

    REQUIRE(ctk_reflib_reader_column_major(reader, 3, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, rm_1, 4));

    REQUIRE(ctk_reflib_reader_column_major(reader, 4, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm, 4));

    REQUIRE(ctk_reflib_reader_column_major(reader, 5, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm + 4, 4));

    // is the same as row major of size 1
    REQUIRE(ctk_reflib_reader_row_major(reader, 0, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm, 4));

    REQUIRE(ctk_reflib_reader_row_major(reader, 1, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm + 4, 4));

    REQUIRE(ctk_reflib_reader_row_major(reader, 2, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, rm_0, 4));

    REQUIRE(ctk_reflib_reader_row_major(reader, 3, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, rm_1, 4));

    REQUIRE(ctk_reflib_reader_row_major(reader, 4, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm, 4));

    REQUIRE(ctk_reflib_reader_row_major(reader, 5, 1, matrix, size_m) == 1);
    REQUIRE(equal_dbl(matrix, matrix_cm + 4, 4));

    ctk_reflib_reader_dispose(reader);
    std::filesystem::remove(delme_cnt);
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


