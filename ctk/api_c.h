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

#ifdef __cplusplus
#include <cstdint>
#include <ctime>
extern "C" {
#else
#include <stdint.h>
#include <time.h>
#endif


int ctk_dcdate2timespec(double day_seconds, double subseconds, struct timespec*);
int ctk_timespec2dcdate(const struct timespec*, double* day_seconds, double* subseconds);
int ctk_tm2timespec(const struct tm*, struct timespec*);
int ctk_timespec2tm(const struct timespec*, struct tm*);


// reflib writer handle
struct ctk_reflib_writer;
typedef struct ctk_reflib_writer ctk_reflib_writer;

// construction/destruction
ctk_reflib_writer* ctk_reflib_writer_make(const char* file_name, int riff64);
void ctk_reflib_writer_dispose(ctk_reflib_writer*);

// NB: the cnt/evt files are created by this function
int ctk_reflib_writer_close(ctk_reflib_writer*);

// setup phase
int ctk_reflib_writer_electrode_uv(ctk_reflib_writer*, const char* active, const char* reference);
int ctk_reflib_writer_electrode(ctk_reflib_writer*, const char* active, const char* reference, const char* unit, double iscale, double rscale);
int ctk_reflib_writer_sampling_frequency(ctk_reflib_writer*, double);
int ctk_reflib_writer_epoch_length(ctk_reflib_writer*, int64_t);
int ctk_reflib_writer_start_time(ctk_reflib_writer*, const struct timespec*);

// data input phase
int ctk_reflib_writer_column_major(ctk_reflib_writer*, const double* matrix, size_t element_count);
int ctk_reflib_writer_column_major_int32(ctk_reflib_writer*, const int32_t* matrix, size_t element_count);
int ctk_reflib_writer_row_major(ctk_reflib_writer*, const double* matrix, size_t element_count);
int ctk_reflib_writer_row_major_int32(ctk_reflib_writer*, const int32_t* matrix, size_t element_count);
int ctk_reflib_writer_v4(ctk_reflib_writer*, const float* matrix, size_t element_count);

int ctk_reflib_writer_trigger(ctk_reflib_writer*, int64_t sample, const char* code);

int ctk_reflib_writer_impedance(ctk_reflib_writer*, const struct timespec*, const float* impedances, size_t element_count);
int ctk_reflib_writer_video(ctk_reflib_writer*, const struct timespec*, double duration, int32_t trigger_code);
int ctk_reflib_writer_epoch(ctk_reflib_writer*, const struct timespec*, double duration, double offset, int32_t trigger_code);

// recording information
// can be supplied both during the setup or data input phases before the file is closed.
int ctk_reflib_writer_hospital(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_physician(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_technician(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_subject_id(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_subject_name(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_subject_address(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_subject_phone(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_subject_sex(ctk_reflib_writer*, char);
int ctk_reflib_writer_subject_handedness(ctk_reflib_writer*, char);
int ctk_reflib_writer_subject_dob(ctk_reflib_writer*, const struct timespec*);
int ctk_reflib_writer_machine_make(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_machine_model(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_machine_sn(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_test_name(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_test_serial(ctk_reflib_writer*, const char*);
int ctk_reflib_writer_comment(ctk_reflib_writer*, const char*);

// "not specified" can be signaled either by passing NULL or the empty string
int ctk_reflib_writer_subject(ctk_reflib_writer*, const char* id, const char* name, const char* address, const char* phone, char sex, char handedness, const struct timespec* date_of_birth);
int ctk_reflib_writer_institution(ctk_reflib_writer*, const char* hospital, const char* physician, const char* technician);
int ctk_reflib_writer_equipment(ctk_reflib_writer*, const char* machine_make, const char* machine_model, const char* machine_sn);
int ctk_reflib_writer_experiment(ctk_reflib_writer*, const char* test_name, const char* test_serial, const char* comment);



// reflib reader handle
struct ctk_reflib_reader;
typedef struct ctk_reflib_reader ctk_reflib_reader;

// construction/destruction
ctk_reflib_reader* ctk_reflib_reader_make(const char* file_name);
void ctk_reflib_reader_dispose(ctk_reflib_reader*);

size_t ctk_reflib_reader_electrode_count(ctk_reflib_reader*);
const char* ctk_reflib_reader_electrode_label(ctk_reflib_reader*, size_t);
const char* ctk_reflib_reader_electrode_reference(ctk_reflib_reader*, size_t);
const char* ctk_reflib_reader_electrode_unit(ctk_reflib_reader*, size_t);
double ctk_reflib_reader_electrode_iscale(ctk_reflib_reader*, size_t);
double ctk_reflib_reader_electrode_rscale(ctk_reflib_reader*, size_t);

struct timespec ctk_reflib_reader_start_time(ctk_reflib_reader*);
double ctk_reflib_reader_sampling_frequency(ctk_reflib_reader*);
int64_t ctk_reflib_reader_epoch_length(ctk_reflib_reader*);

int64_t ctk_reflib_reader_sample_count(ctk_reflib_reader*);
int64_t ctk_reflib_reader_column_major(ctk_reflib_reader*, int64_t i, int64_t samples, double* matrix, size_t element_count);
int64_t ctk_reflib_reader_column_major_int32(ctk_reflib_reader*, int64_t i, int64_t samples, int32_t* matrix, size_t element_count);
int64_t ctk_reflib_reader_row_major(ctk_reflib_reader*, int64_t i, int64_t samples, double* matrix, size_t element_count);
int64_t ctk_reflib_reader_row_major_int32(ctk_reflib_reader*, int64_t i, int64_t samples, int32_t* matrix, size_t element_count);
int64_t ctk_reflib_reader_v4(ctk_reflib_reader*, int64_t i, int64_t samples, float* matrix, size_t element_count);

size_t ctk_reflib_reader_trigger_count(ctk_reflib_reader*);
int ctk_reflib_reader_trigger(ctk_reflib_reader*, size_t i, int64_t* sample, char* code, size_t csize);

size_t ctk_reflib_reader_impedance_count(ctk_reflib_reader*);
size_t ctk_reflib_reader_impedance_size(ctk_reflib_reader*, size_t i); // usually equals the electrode count: one impedance value per channel
int ctk_reflib_reader_impedance(ctk_reflib_reader*, size_t i, struct timespec* stamp, float* impedances, size_t isize);

size_t ctk_reflib_reader_video_count(ctk_reflib_reader*);
int ctk_reflib_reader_video(ctk_reflib_reader*, size_t i, struct timespec* stamp, double* duration, int32_t* trigger_code);

size_t ctk_reflib_reader_epoch_count(ctk_reflib_reader*);
int ctk_reflib_reader_epoch(ctk_reflib_reader*, size_t i, struct timespec* stamp, double* duration, double* offset, int32_t* trigger_code);

const char* ctk_reflib_reader_hospital(ctk_reflib_reader*);
const char* ctk_reflib_reader_physician(ctk_reflib_reader*);
const char* ctk_reflib_reader_technician(ctk_reflib_reader*);
const char* ctk_reflib_reader_subject_id(ctk_reflib_reader*);
const char* ctk_reflib_reader_subject_name(ctk_reflib_reader*);
const char* ctk_reflib_reader_subject_address(ctk_reflib_reader*);
const char* ctk_reflib_reader_subject_phone(ctk_reflib_reader*);
char ctk_reflib_reader_subject_sex(ctk_reflib_reader*);
char ctk_reflib_reader_subject_handedness(ctk_reflib_reader*);
struct timespec ctk_reflib_reader_subject_dob(ctk_reflib_reader*);
const char* ctk_reflib_reader_machine_make(ctk_reflib_reader*);
const char* ctk_reflib_reader_machine_model(ctk_reflib_reader*);
const char* ctk_reflib_reader_machine_sn(ctk_reflib_reader*);
const char* ctk_reflib_reader_test_name(ctk_reflib_reader*);
const char* ctk_reflib_reader_test_serial(ctk_reflib_reader*);
const char* ctk_reflib_reader_comment(ctk_reflib_reader*);


/* type: "console", "file", "visual studio" */
/*       if set to "file" the output file is located under ./logs/ctk_YYYY-MM-DD.txt */
/* level: "trace", "debug", "info", "warning", "error", "critical", "off" */
int ctk_set_logger(const char* type, const char* level);
int ctk_log_trace(const char* msg);
int ctk_log_debug(const char* msg);
int ctk_log_info(const char* msg);
int ctk_log_warning(const char* msg);
int ctk_log_error(const char* msg);
int ctk_log_critical(const char* msg);

#ifdef __cplusplus
} // extern "C"
#endif

