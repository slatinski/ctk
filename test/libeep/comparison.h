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

#include <inttypes.h>
#include <time.h>

enum bool { nope, yep };

enum bool equal_strings(const char* x, const char* y, const char* func);
enum bool equal_sex(char x, char y, const char* func);
enum bool equal_handedness(char x, char y, const char* func);

enum bool equal_date(double date_x, double frac_x, double date_y, double frac_y, const char* func);
enum bool equal_timespec(const struct timespec* x, const struct timespec* y, const char* func);
enum bool equal_date_timespec(double date, double frac, const struct timespec* ts, const char* func);

enum bool equal_electrode( const char* label_x, const char* ref_x, const char* unit_x, double is_x, double rs_x
                         , const char* label_y, const char* ref_y, const char* unit_y, double is_y, double rs_y
                         , const char* func);
enum bool equal_electrode_v4( const char* label_v4, const char* ref_v4, const char* unit_v4, double scale
                            , const char* label_ctk, const char* ref_ctk, const char* unit_ctk, double iscale, double rscale
                            , const char* func, enum bool* v4_truncated_scale);


enum bool equal_subject( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, const struct timespec* dob_x
                       , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct timespec* dob_y
                       , const char* func);
enum bool equal_subject_v4( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, int year_x, int month_x, int day_x
                          , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct timespec* dob_y
                          , const char* func);
enum bool equal_subject_v4_v4( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, int year_x, int month_x, int day_x
                             , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, int year_y, int month_y, int day_y
                             , const char* func);
enum bool equal_subject_eeg_ctk( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, const struct tm* dob_x
                               , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct timespec* dob_y
                               , const char* func);
enum bool equal_subject_eeg_eeg( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, const struct tm* dob_x
                               , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct tm* dob_y
                               , const char* func);


enum bool equal_institution( const char* hospital_x, const char* physician_x, const char* technician_x
                           , const char* hospital_y, const char* physician_y, const char* technician_y
                           , const char* func);

enum bool equal_equipment( const char* make_x, const char* model_x, const char* sn_x
                         , const char* make_y, const char* model_y, const char* sn_y
                         , const char* func);

enum bool equal_experiment( const char* name_x, const char* serial_x, const char* comment_x
                          , const char* name_y, const char* serial_y, const char* comment_y
                          , const char* func);

enum bool equal_trigger(const char* code_x, int64_t sample_x, const char* code_y, int64_t sample_y, const char* func);
enum bool equal_trigger_u64_s64(const char* code_x, uint64_t sample_x, const char* code_y, int64_t sample_y, const char* func);
enum bool equal_trigger_u64_u64(const char* code_x, uint64_t sample_x, const char* code_y, uint64_t sample_y, const char* func);


void print_timespec(const struct timespec*, char* output, size_t size);

