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

#pragma once

#include <stdio.h>

struct input_txt
{
    FILE* f;
    char line[4096];
    int more;
};

struct input_txt make_input_txt();
void dispose(struct input_txt*);

const char* next(struct input_txt*);
int reset(struct input_txt*);


// completely unrelated stderr i/o

const char* last_n(const char*, size_t);

void stderr_compare_1file_2readers_begin(const char* fname, const char* lib_x, const char* lib_y);
void stderr_compare_1file_2readers_failed(const char* lib);

void stderr_copy_begin(const char* fname, const char* delme_cnt, const char* lib_reader, const char* lib_writer);
void stderr_compare_begin(const char* fname_x, const char* fname_y, const char* lib_reader);

void stderr_failed_reader(const char* fname);
void stderr_failed_writer(const char* fname);

void stderr_read_speed_begin(const char* fname, long chunk);
void stderr_write_speed_begin(const char* fname, long chunk);
void stderr_speed_end_incomparable();
void stderr_speed_end(const char* lib_unit, double unit, const char* lib_compared, double compared);


enum summary { ok = 0
             , header_epoch = 1
             , header_srate = 1 << 1
             , header_stamp = 1 << 2
             , header_smpl  = 1 << 3
             , header_elc   = 1 << 4
             , info         = 1 << 5
             , trg          = 1 << 6
             , eeg_data     = 1 << 7
             , aux          = 1 << 8
             };
void stderr_print_success(enum summary result);

void stderr_intermezzo(const char*);

