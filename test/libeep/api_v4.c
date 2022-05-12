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

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "v4/eep.h"
#include "api_c.h"
#include "input_txt.h"
#include "comparison.h"


#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4477)
#endif // WIN32


/* PERFORMANCE */

// reads as fast as possible, v4
long accessible_chunks_v4(const char* fname, long chunk) {
    char msg[512];
    long accessible = 0;

    const cntfile_t v4 = libeep_read(fname);
    if(v4 == -1) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_v4] can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        return accessible;
    }

    const int electrodes = libeep_get_channel_count(v4);
    const long samples = libeep_get_sample_count(v4);
    if (electrodes < 1 || samples < 1) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_v4] invalid matrix dimensions %dx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (long sample = 0; sample < samples; sample += chunk) {
        const long remaining = samples - sample;
        const long due = chunk < remaining ? chunk : remaining;

        float* matrix = libeep_get_samples(v4, sample, sample + due);
        if (!matrix) {
            snprintf(msg, sizeof(msg), "[accessible_chunks_v4] can not access range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            continue;
        }
        libeep_free_samples(matrix);
        accessible += due;
    }

cleanup:

    libeep_close(v4);
    return accessible;
}


// reads as fast as possible, ctk
int64_t accessible_chunks_ctk(const char* fname, long chunk) {
    char msg[512];
    int64_t accessible = 0;

    ctk_reflib_reader* ctk = ctk_reflib_reader_make(fname);
    if (!ctk) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_ctk] can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        return accessible;
    }

    const size_t electrodes = ctk_reflib_reader_electrode_count(ctk);
    const int64_t samples = ctk_reflib_reader_sample_count(ctk);
    if (electrodes < 1 || samples < 1) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_ctk] invalid matrix dimensions %ldx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t area = electrodes * (size_t)chunk;
    float* matrix = (float*)malloc(area * sizeof(float));
    if (!matrix) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_ctk] can not allocate %ldx%ld matrix", electrodes, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }
    const int64_t matrix_size = (int64_t)area;

    for (int64_t sample = 0; sample < samples; sample += chunk) {
        const int64_t remaining = samples - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;

        const int64_t received = ctk_reflib_reader_v4(ctk, sample, due, matrix, matrix_size);
        if (received != due) {
            snprintf(msg, sizeof(msg), "[accessible_chunks_ctk] can not access range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            continue;
        }
        accessible += due;
    }

    free(matrix);

cleanup:
    ctk_reflib_reader_dispose(ctk);
    return accessible;
}


// writes as fast as possible, v4
long write_in_chunks_v4(const char* fname, long chunk) {
    float* matrix = NULL;
    ctk_reflib_reader* reader = NULL;
    cntfile_t writer = -1;
    char msg[512];
    char stamp_str[256];
    long written = 0;
    const char* delme_cnt = "write_in_chunks_v4.cnt";
    const int cnt64 = 1;
    chaninfo_t channels = -1;

    reader = ctk_reflib_reader_make(fname);
    if (!reader) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_v4] ctk can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t electrodes = ctk_reflib_reader_electrode_count(reader);
    const int64_t samples = ctk_reflib_reader_sample_count(reader);
    if (electrodes < 1 || samples < 1) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_v4] invalid matrix dimensions %ldx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t area = electrodes * (size_t)chunk;
    matrix = (float*)malloc(area * sizeof(float));
    if (!matrix) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_v4] can not allocate %ldx%ld matrix", electrodes, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    channels = libeep_create_channel_info();
    for (size_t i = 0; i < electrodes; ++i) {
        const char* label = ctk_reflib_reader_electrode_label(reader, i);
        const char* ref = ctk_reflib_reader_electrode_reference(reader, i);
        const char* unit = ctk_reflib_reader_electrode_unit(reader, i);
        if (libeep_add_channel(channels, label, ref, unit) != i + 1) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_v4] can not write electrode %ld: '%s'-'%s' '%s'", i, label, ref, unit);
            ctk_log_error(msg);
            goto cleanup;
        }
    }
    const double rate = ctk_reflib_reader_sampling_frequency(reader);

    writer = libeep_write_cnt(delme_cnt, (int)rate, channels, cnt64);
    if (writer == -1) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_v4] v4 can not open '%s' for writing", delme_cnt);
        ctk_log_error(msg);
        goto cleanup;
    }

    const struct timespec stamp = ctk_reflib_reader_start_time(reader);
    double day_seconds, subseconds;
    if (ctk_timespec2dcdate(&stamp, &day_seconds, &subseconds) != EXIT_SUCCESS) {
        print_timespec(&stamp, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[write_in_chunks_v4] start time conversion to dcdate %s", stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }
    recinfo_t recinfo = libeep_create_recinfo();
    libeep_set_start_date_and_fraction(recinfo, day_seconds, subseconds);

    for (int64_t sample = 0; sample < samples; sample += chunk) {
        const int64_t remaining = samples - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;
        const size_t due_size = electrodes * (size_t)due;

        const int64_t received = ctk_reflib_reader_v4(reader, sample, due, matrix, due_size);
        if (received != due) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_v4] can not read range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        libeep_add_samples(writer, matrix, (int)due);
        written += (long)due;
    }

cleanup:

    if (matrix) { free(matrix); }
    if (writer != -1) libeep_close(writer);
    ctk_reflib_reader_dispose(reader);
    remove(delme_cnt);
    return written;
}

// writes as fast as possible, ctk
long write_in_chunks_ctk(const char* fname, long chunk) {
    char msg[512];
    int64_t written = 0;
    ctk_reflib_reader* reader = NULL;
    ctk_reflib_writer* writer = NULL;
    float* matrix = NULL;
    const char* delme_cnt = "write_in_chunks_ctk.cnt";
    const int cnt64 = 1;

    reader = ctk_reflib_reader_make(fname);
    if (!reader) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t electrodes = ctk_reflib_reader_electrode_count(reader);
    const int64_t samples = ctk_reflib_reader_sample_count(reader);
    if (electrodes < 1 || samples < 1) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] invalid matrix dimensions %ldx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t area = electrodes * (size_t)chunk;
    matrix = (float*)malloc(area * sizeof(float));
    if (!matrix) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not allocate %ldx%ld matrix", electrodes, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    writer = ctk_reflib_writer_make(delme_cnt, cnt64);
    if (!writer) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not open '%s' for writing", delme_cnt);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (size_t i = 0; i < electrodes; ++i) {
        const char* label = ctk_reflib_reader_electrode_label(reader, i);
        const char* ref = ctk_reflib_reader_electrode_reference(reader, i);
        const char* unit = ctk_reflib_reader_electrode_unit(reader, i);
        const double iscale = ctk_reflib_reader_electrode_iscale(reader, i);
        const double rscale = ctk_reflib_reader_electrode_rscale(reader, i);
        if (ctk_reflib_writer_electrode(writer, label, ref, unit, iscale, rscale) != EXIT_SUCCESS) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not write electrode %ld: '%s'-'%s' '%s' %f %f", i, label, ref, unit, iscale, rscale);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

    const double rate = ctk_reflib_reader_sampling_frequency(reader);
    if (ctk_reflib_writer_sampling_frequency(writer, rate) != EXIT_SUCCESS) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not write sampling frequency %f", rate);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int64_t epoch = ctk_reflib_reader_epoch_length(reader);
    if (ctk_reflib_writer_epoch_length(writer, epoch) != EXIT_SUCCESS) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not write epoch length %ld", epoch);
        ctk_log_error(msg);
        goto cleanup;
    }

    const struct timespec stamp = ctk_reflib_reader_start_time(reader);
    if (ctk_reflib_writer_start_time(writer, &stamp) != EXIT_SUCCESS) {
        char stamp_str[256];
        print_timespec(&stamp, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not write eeg start time %s", stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (int64_t sample = 0; sample < samples; sample += chunk) {
        const int64_t remaining = samples - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;
        const size_t due_size = electrodes * (size_t)due;

        const int64_t received = ctk_reflib_reader_v4(reader, sample, due, matrix, due_size);
        if (received != due) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not read range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        if (ctk_reflib_writer_v4(writer, matrix, due_size) != EXIT_SUCCESS) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not write range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        written += due;
    }

cleanup:

    if (matrix) { free(matrix); }
    ctk_reflib_reader_dispose(reader);
    ctk_reflib_writer_close(writer);
    ctk_reflib_writer_dispose(writer);
    remove(delme_cnt);
    return (long)written;
}


void compare_reader_performance(const char* fname, long chunk) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[compare_reader_performance] processing '%s'", last_n(fname, 40));
    ctk_log_info(msg);

    stderr_read_speed_begin(fname, chunk);

    const clock_t b_ctk = clock();
    const int64_t accessible_ctk = accessible_chunks_ctk(fname, chunk);
    const clock_t e_ctk = clock();
    const double t_ctk = ((double)(e_ctk - b_ctk)) / CLOCKS_PER_SEC;

    const clock_t b_v4 = clock();
    const int64_t accessible_v4 = accessible_chunks_v4(fname, chunk);
    const clock_t e_v4 = clock();
    const double t_v4 = ((double)(e_v4 - b_v4)) / CLOCKS_PER_SEC;

    if (accessible_ctk != accessible_v4) {
        stderr_speed_end_incomparable();
        return;
    }

    stderr_speed_end("v4", t_v4, "ctk", t_ctk);
}

void compare_writer_performance(const char* fname, long chunk) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[compare_writer_performance] processing '%s'", last_n(fname, 40));
    ctk_log_info(msg);

    stderr_write_speed_begin(fname, chunk);

    const clock_t b_ctk = clock();
    const int64_t written_ctk = write_in_chunks_ctk(fname, chunk);
    const clock_t e_ctk = clock();
    const double t_ctk = ((double)(e_ctk - b_ctk)) / CLOCKS_PER_SEC;

    const clock_t b_v4 = clock();
    const int64_t written_v4 = write_in_chunks_v4(fname, chunk);
    const clock_t e_v4 = clock();
    const double t_v4 = ((double)(e_v4 - b_v4)) / CLOCKS_PER_SEC;

    if (written_ctk != written_v4) {
        stderr_speed_end_incomparable();
        return;
    }

    stderr_speed_end("v4", t_v4, "ctk", t_ctk);
}


/* COMPATIBILITY */

enum bool compare_electrode_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk, size_t i, enum bool* v4_truncated_scale) {
    const char* label_v4 = libeep_get_channel_label(v4, (int)i);
    const char* ref_v4 = libeep_get_channel_reference(v4, (int)i);
    const char* unit_v4 = libeep_get_channel_unit(v4, (int)i);
    const float scale = libeep_get_channel_scale(v4, (int)i);

    const char* label_ctk = ctk_reflib_reader_electrode_label(ctk, (int)i);
    const char* ref_ctk = ctk_reflib_reader_electrode_reference(ctk, (int)i);
    const char* unit_ctk = ctk_reflib_reader_electrode_unit(ctk, (int)i);
    const double iscale = ctk_reflib_reader_electrode_iscale(ctk, (int)i);
    const double rscale = ctk_reflib_reader_electrode_rscale(ctk, (int)i);

    return equal_electrode_v4(label_v4, ref_v4, unit_v4, scale
                            , label_ctk, ref_ctk, unit_ctk, iscale, rscale
                            , "compare_electrode_v4_ctk", v4_truncated_scale);
}

enum bool compare_electrode_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y, size_t i) {
    const char* label_x = ctk_reflib_reader_electrode_label(x, i);
    const char* ref_x = ctk_reflib_reader_electrode_reference(x, i);
    const char* unit_x = ctk_reflib_reader_electrode_unit(x, i);
    const double iscale_x = ctk_reflib_reader_electrode_iscale(x, i);
    const double rscale_x = ctk_reflib_reader_electrode_rscale(x, i);

    const char* label_y = ctk_reflib_reader_electrode_label(y, i);
    const char* ref_y = ctk_reflib_reader_electrode_reference(y, i);
    const char* unit_y = ctk_reflib_reader_electrode_unit(y, i);
    const double iscale_y = ctk_reflib_reader_electrode_iscale(y, i);
    const double rscale_y = ctk_reflib_reader_electrode_rscale(y, i);

    return equal_electrode( label_x, ref_x, unit_x, iscale_x, rscale_x
                          , label_y, ref_y, unit_y, iscale_y, rscale_y
                          , "compare_electrode_ctk");
}

enum bool compare_electrode_v4(cntfile_t x, cntfile_t y, size_t i) {
    const char* label_x = libeep_get_channel_label(x, (int)i);
    const char* ref_x = libeep_get_channel_reference(x, (int)i);
    const char* unit_x = libeep_get_channel_unit(x, (int)i);
    const float scale_x = libeep_get_channel_scale(x, (int)i);

    const char* label_y = libeep_get_channel_label(y, (int)i);
    const char* ref_y = libeep_get_channel_reference(y, (int)i);
    const char* unit_y = libeep_get_channel_unit(y, (int)i);
    const float scale_y = libeep_get_channel_scale(y, (int)i);

    return equal_electrode( label_x, ref_x, unit_x, 1, scale_x
                          , label_y, ref_y, unit_y, 1, scale_y
                          , "compare_electrode_v4");
}

enum summary compare_electrodes_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk, enum bool* v4_truncated_scale) {
    char msg[512];
    enum summary result = ok;

    *v4_truncated_scale = nope;

    const int electrodes_v4 = libeep_get_channel_count(v4);
    if (electrodes_v4 < 0) {
        snprintf(msg, sizeof(msg), "[compare_electrodes_v4_ctk] v4 negative count %d", electrodes_v4);
        ctk_log_error(msg);
        return header_elc;
    }

    const size_t electrodes_ctk = ctk_reflib_reader_electrode_count(ctk);
    if (electrodes_v4 != electrodes_ctk) {
        result = header_elc;
        snprintf(msg, sizeof(msg), "[compare_electrodes_v4_ctk] count %d != %ld", electrodes_v4, electrodes_ctk);
        ctk_log_error(msg);
    }

    for (size_t i = 0; i < electrodes_ctk; ++i) {
        if (!compare_electrode_v4_ctk(v4, ctk, i, v4_truncated_scale)) {
            result = header_elc;
        }
    }

    return result;
}

enum summary compare_electrodes_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    char msg[512];
    enum summary result = ok;

    const size_t electrodes_x = ctk_reflib_reader_electrode_count(x);
    const size_t electrodes_y = ctk_reflib_reader_electrode_count(y);
    if (electrodes_x != electrodes_y) {
        result = header_elc;
        snprintf(msg, sizeof(msg), "[compare_electrodes_ctk] count %ld != %ld", electrodes_x, electrodes_y);
        ctk_log_error(msg);
    }

    for (size_t i = 0; i < electrodes_x; ++i) {
        if (!compare_electrode_ctk(x, y, i)) {
            result = header_elc;
        }
    }

    return result;
}

enum summary compare_electrodes_v4(cntfile_t x, cntfile_t y) {
    char msg[512];
    enum summary result = ok;

    const int electrodes_x = libeep_get_channel_count(x);
    if (electrodes_x < 0) {
        snprintf(msg, sizeof(msg), "[compare_electrodes_v4] x negative count %d", electrodes_x);
        ctk_log_error(msg);
        return header_elc;
    }

    const int electrodes_y = libeep_get_channel_count(y);
    if (electrodes_y < 0) {
        snprintf(msg, sizeof(msg), "[compare_electrodes_v4] y negative count %d", electrodes_y);
        ctk_log_error(msg);
        return header_elc;
    }

    if (electrodes_x != electrodes_y) {
        result = header_elc;
        snprintf(msg, sizeof(msg), "[compare_electrodes_v4] count %d != %d", electrodes_x, electrodes_y);
        ctk_log_error(msg);
    }

    for (int i = 0; i < electrodes_x; ++i) {
        if (!compare_electrode_v4(x, y, i)) {
            result = header_elc;
        }
    }

    return result;
}


enum summary compare_start_time_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    double date_v4, frac_v4;
    libeep_get_start_date_and_fraction(v4, &date_v4, &frac_v4);

    const struct timespec time_ctk = ctk_reflib_reader_start_time(ctk);

    return equal_date_timespec(date_v4, frac_v4, &time_ctk, "compare_start_time_v4_ctk") ? ok : header_stamp; // bool -> summary
}


enum summary compare_start_time_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    const struct timespec stamp_x = ctk_reflib_reader_start_time(x);
    const struct timespec stamp_y = ctk_reflib_reader_start_time(y);

    return equal_timespec(&stamp_x, &stamp_y, "compare_start_time_ctk") ?  ok : header_stamp; // bool -> summary
}


enum summary compare_start_time_v4(cntfile_t x, cntfile_t y) {
    double date_x, frac_x;
    libeep_get_start_date_and_fraction(x, &date_x, &frac_x);

    double date_y, frac_y;
    libeep_get_start_date_and_fraction(y, &date_y, &frac_y);

    return equal_date(date_x, frac_x, date_y, frac_y, "compare_start_time_v4") ? ok : header_stamp; // bool -> summary
}



enum summary compare_sample_rate_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    char msg[512];

    const int rate_v4 = libeep_get_sample_frequency(v4);
    const double rate_ctk = ctk_reflib_reader_sampling_frequency(ctk);

    if (rate_v4 != (int)round(rate_ctk)) {
        snprintf(msg, sizeof(msg), "[compare_sample_rate_v4_ctk] %d != %f", rate_v4, rate_ctk);
        ctk_log_error(msg);
        return header_srate;
    }

    return ok;
}


enum summary compare_sample_rate_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    char msg[512];

    const double rate_x = ctk_reflib_reader_sampling_frequency(x);
    const double rate_y = ctk_reflib_reader_sampling_frequency(y);

    if (rate_x != rate_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_rate_ctk] %f != %f", rate_x, rate_y);
        ctk_log_error(msg);
        return header_srate;
    }

    return ok;
}


enum summary compare_sample_rate_v4(cntfile_t x, cntfile_t y) {
    char msg[512];

    const int rate_x = libeep_get_sample_frequency(x);
    const int rate_y = libeep_get_sample_frequency(y);

    if (rate_x != rate_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_rate_v4] %d != %d", rate_x, rate_y);
        ctk_log_error(msg);
        return header_srate;
    }

    return ok;
}



enum summary compare_sample_count_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    char msg[512];

    const long samples_v4 = libeep_get_sample_count(v4);
    const int64_t samples_ctk = ctk_reflib_reader_sample_count(ctk);
    if (samples_v4 != samples_ctk) {
        snprintf(msg, sizeof(msg), "[compare_sample_count_v4_ctk] %ld != %ld", samples_v4, samples_ctk);
        ctk_log_error(msg);
        return header_smpl;
    }

    return ok;
}


enum summary compare_sample_count_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    char msg[512];

    const int64_t samples_x = ctk_reflib_reader_sample_count(x);
    const int64_t samples_y = ctk_reflib_reader_sample_count(y);
    if (samples_x != samples_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_count_ctk] %ld != %ld", samples_x, samples_y);
        ctk_log_error(msg);
        return header_smpl;
    }

    return ok;
}


enum summary compare_sample_count_v4(cntfile_t x, cntfile_t y) {
    char msg[512];

    const long samples_x = libeep_get_sample_count(x);
    const long samples_y = libeep_get_sample_count(y);
    if (samples_x != samples_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_count_v4] %ld != %ld", samples_x, samples_y);
        ctk_log_error(msg);
        return header_smpl;
    }

    return ok;
}


enum summary compare_subject_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    const char* id_v4 = libeep_get_patient_id(v4);
    const char* name_v4 = libeep_get_patient_name(v4);
    const char* address_v4 = libeep_get_patient_address(v4);
    const char* phone_v4 = libeep_get_patient_phone(v4);
    const char sex_v4 = libeep_get_patient_sex(v4);
    const char hand_v4 = libeep_get_patient_handedness(v4);
    int year_v4, month_v4, day_v4;
    libeep_get_date_of_birth(v4, &year_v4, &month_v4, &day_v4);

    const char* id_ctk = ctk_reflib_reader_subject_id(ctk);
    const char* name_ctk = ctk_reflib_reader_subject_name(ctk);
    const char* address_ctk = ctk_reflib_reader_subject_address(ctk);
    const char* phone_ctk = ctk_reflib_reader_subject_phone(ctk);
    const char sex_ctk = ctk_reflib_reader_subject_sex(ctk);
    const char hand_ctk = ctk_reflib_reader_subject_handedness(ctk);
    const struct timespec dob = ctk_reflib_reader_subject_dob(ctk);

    return equal_subject_v4( id_v4, name_v4, address_v4, phone_v4, sex_v4, hand_v4, year_v4, month_v4, day_v4
                         , id_ctk, name_ctk, address_ctk, phone_ctk, sex_ctk, hand_ctk, &dob
                         , "compare_subject_v4_ctk") ? ok : info; // bool -> summary
}


enum summary compare_subject_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    const char* id_x = ctk_reflib_reader_subject_id(x);
    const char* name_x = ctk_reflib_reader_subject_name(x);
    const char* address_x = ctk_reflib_reader_subject_address(x);
    const char* phone_x = ctk_reflib_reader_subject_phone(x);
    const char sex_x = ctk_reflib_reader_subject_sex(x);
    const char hand_x = ctk_reflib_reader_subject_handedness(x);
    const struct timespec dob_x = ctk_reflib_reader_subject_dob(x);

    const char* id_y = ctk_reflib_reader_subject_id(y);
    const char* name_y = ctk_reflib_reader_subject_name(y);
    const char* address_y = ctk_reflib_reader_subject_address(y);
    const char* phone_y = ctk_reflib_reader_subject_phone(y);
    const char sex_y = ctk_reflib_reader_subject_sex(y);
    const char hand_y = ctk_reflib_reader_subject_handedness(y);
    const struct timespec dob_y = ctk_reflib_reader_subject_dob(y);

    return equal_subject( id_x, name_x, address_x, phone_x, sex_x, hand_x, &dob_x
                        , id_y, name_y, address_y, phone_y, sex_y, hand_y, &dob_y
                        , "compare_subject_ctk") ? ok : info; // bool -> summary
}

enum summary compare_subject_v4(cntfile_t x, cntfile_t y) {
    const char* id_x = libeep_get_patient_id(x);
    const char* name_x = libeep_get_patient_name(x);
    const char* address_x = libeep_get_patient_address(x);
    const char* phone_x = libeep_get_patient_phone(x);
    const char sex_x = libeep_get_patient_sex(x);
    const char hand_x = libeep_get_patient_handedness(x);

    const char* id_y = libeep_get_patient_id(y);
    const char* name_y = libeep_get_patient_name(y);
    const char* address_y = libeep_get_patient_address(y);
    const char* phone_y = libeep_get_patient_phone(y);
    const char sex_y = libeep_get_patient_sex(y);
    const char hand_y = libeep_get_patient_handedness(y);

    int year_x, month_x, day_x;
    int year_y, month_y, day_y;
    libeep_get_date_of_birth(x, &year_x, &month_x, &day_x);
    libeep_get_date_of_birth(y, &year_y, &month_y, &day_y);

    return equal_subject_v4_v4( id_x, name_x, address_x, phone_x, sex_x, hand_x, year_x, month_x, day_x
                              , id_y, name_y, address_y, phone_y, sex_y, hand_y, year_y, month_y, day_y
                              , "compare_subject_v4") ? ok : info; // bool -> summary
}


enum summary compare_institution_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    const char* hospital_v4 = libeep_get_hospital(v4);
    const char* physician_v4 = libeep_get_physician(v4);
    const char* technician_v4 = libeep_get_technician(v4);

    const char* hospital_ctk = ctk_reflib_reader_hospital(ctk);
    const char* physician_ctk = ctk_reflib_reader_physician(ctk);
    const char* technician_ctk = ctk_reflib_reader_technician(ctk);

    return equal_institution(hospital_v4, physician_v4, technician_v4
                            , hospital_ctk, physician_ctk, technician_ctk
                            , "compare_institution_v4_ctk") ? ok : info; // bool -> summary
}

enum summary compare_institution_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    const char* hospital_x = ctk_reflib_reader_hospital(x);
    const char* physician_x = ctk_reflib_reader_physician(x);
    const char* technician_x = ctk_reflib_reader_technician(x);

    const char* hospital_y = ctk_reflib_reader_hospital(y);
    const char* physician_y = ctk_reflib_reader_physician(y);
    const char* technician_y = ctk_reflib_reader_technician(y);

    return equal_institution(hospital_x, physician_x, technician_x
                            , hospital_y, physician_y, technician_y
                            , "compare_institution_ctk") ? ok : info; // bool -> summary
}

enum summary compare_institution_v4(cntfile_t x, cntfile_t y) {
    const char* hospital_x = libeep_get_hospital(x);
    const char* physician_x = libeep_get_physician(x);
    const char* technician_x = libeep_get_technician(x);

    const char* hospital_y = libeep_get_hospital(y);
    const char* physician_y = libeep_get_physician(y);
    const char* technician_y = libeep_get_technician(y);

    return equal_institution(hospital_x, physician_x, technician_x
                            , hospital_y, physician_y, technician_y
                            , "compare_institution_v4") ? ok : info; // bool -> summary
}


enum summary compare_equipment_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    const char* make_ctk = ctk_reflib_reader_machine_make(ctk);
    const char* model_ctk = ctk_reflib_reader_machine_model(ctk);
    const char* sn_ctk = ctk_reflib_reader_machine_sn(ctk);

    const char* make_v4 = libeep_get_machine_make(v4);
    const char* model_v4 = libeep_get_machine_model(v4);
    const char* sn_v4 = libeep_get_machine_serial_number(v4);

    return equal_equipment( make_v4, model_v4, sn_v4
                          , make_ctk, model_ctk, sn_ctk
                          , "compare_equipment_v4_ctk") ? ok : info; // bool -> summary
}

enum summary compare_equipment_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    const char* make_x = ctk_reflib_reader_machine_make(x);
    const char* model_x = ctk_reflib_reader_machine_model(x);
    const char* sn_x = ctk_reflib_reader_machine_sn(x);

    const char* make_y = ctk_reflib_reader_machine_make(y);
    const char* model_y = ctk_reflib_reader_machine_model(y);
    const char* sn_y = ctk_reflib_reader_machine_sn(y);

    return equal_equipment( make_x, model_x, sn_x
                          , make_y, model_y, sn_y
                          , "compare_equipment_ctk") ? ok : info; // bool -> summary
}

enum summary compare_equipment_v4(cntfile_t x, cntfile_t y) {
    const char* make_x = libeep_get_machine_make(x);
    const char* model_x = libeep_get_machine_model(x);
    const char* sn_x = libeep_get_machine_serial_number(x);

    const char* make_y = libeep_get_machine_make(y);
    const char* model_y = libeep_get_machine_model(y);
    const char* sn_y = libeep_get_machine_serial_number(y);

    return equal_equipment( make_x, model_x, sn_x
                          , make_y, model_y, sn_y
                          , "compare_equipment_v4") ? ok : info; // bool -> summary
}

enum summary compare_experiment_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    const char* name_v4 = libeep_get_test_name(v4);
    const char* serial_v4 = libeep_get_test_serial(v4);
    const char* comment_v4 = libeep_get_comment(v4);

    const char* name_ctk = ctk_reflib_reader_test_name(ctk);
    const char* serial_ctk = ctk_reflib_reader_test_serial(ctk);
    const char* comment_ctk = ctk_reflib_reader_comment(ctk);

    return equal_experiment( name_v4, serial_v4, comment_v4
                           , name_ctk, serial_ctk, comment_ctk
                           , "compare_experiment_v4_ctk") ? ok : info; // bool -> summary
}


enum summary compare_experiment_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    const char* name_x = ctk_reflib_reader_test_name(x);
    const char* serial_x = ctk_reflib_reader_test_serial(x);
    const char* comment_x = ctk_reflib_reader_comment(x);

    const char* name_y = ctk_reflib_reader_test_name(y);
    const char* serial_y = ctk_reflib_reader_test_serial(y);
    const char* comment_y = ctk_reflib_reader_comment(y);

    return equal_experiment( name_x, serial_x, comment_x
                           , name_y, serial_y, comment_y
                           , "compare_experiment_ctk") ? ok : info; // bool -> summary
}

enum summary compare_experiment_v4(cntfile_t x, cntfile_t y) {
    const char* name_x = libeep_get_test_name(x);
    const char* serial_x = libeep_get_test_serial(x);
    const char* comment_x = libeep_get_comment(x);

    const char* name_y = libeep_get_test_name(y);
    const char* serial_y = libeep_get_test_serial(y);
    const char* comment_y = libeep_get_comment(y);

    return equal_experiment( name_x, serial_x, comment_x
                           , name_y, serial_y, comment_y
                           , "compare_experiment_v4") ? ok : info; // bool -> summary
}


enum bool compare_trigger_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk, size_t i) {
    char msg[512];

    char code_ctk[9];
    int64_t sample_ctk;
    if (ctk_reflib_reader_trigger(ctk, i, &sample_ctk, code_ctk, sizeof(code_ctk)) != EXIT_SUCCESS) {
        snprintf(msg, sizeof(msg), "[compare_trigger_v4_ctk] ctk can not obtain trigger %ld", i);
        ctk_log_error(msg);
        return nope;
    }

    uint64_t sample_v4;
    const char* code_v4 = libeep_get_trigger(v4, (int)i, &sample_v4);

    return equal_trigger_u64_s64(code_v4, sample_v4, code_ctk, sample_ctk, "compare_trigger_v4_ctk");
}

enum bool compare_trigger_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y, size_t i) {
    char msg[512], code_x[9], code_y[9];
    int64_t sample_x, sample_y;

    if (ctk_reflib_reader_trigger(x, i, &sample_x, code_x, sizeof(code_x)) != EXIT_SUCCESS) {
        snprintf(msg, sizeof(msg), "[compare_trigger_ctk] x can not obtain trigger %ld", i);
        ctk_log_error(msg);
        return nope;
    }

    if (ctk_reflib_reader_trigger(y, i, &sample_y, code_y, sizeof(code_y)) != EXIT_SUCCESS) {
        snprintf(msg, sizeof(msg), "[compare_trigger_ctk] y can not obtain trigger %ld", i);
        ctk_log_error(msg);
        return nope;
    }

    return equal_trigger(code_x, sample_x, code_y, sample_y, "compare_trigger_ctk");
}

enum bool compare_trigger_v4(cntfile_t x, cntfile_t y, int i) {
    uint64_t sample_x, sample_y;
    const char* code_x = libeep_get_trigger(x, i, &sample_x);
    const char* code_y = libeep_get_trigger(y, i, &sample_y);

    return equal_trigger_u64_u64(code_x, sample_x, code_y, sample_y, "compare_trigger_v4");
}

enum summary compare_triggers_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk) {
    char msg[512];
    enum summary result = ok;

    const int count_v4 = libeep_get_trigger_count(v4);
    if (count_v4 < 0) {
        snprintf(msg, sizeof(msg), "[compare_triggers_v4_ctk] v4 invalid count %d", count_v4);
        ctk_log_error(msg);
        return trg;
    }

    const size_t count_ctk = ctk_reflib_reader_trigger_count(ctk);
    if (count_v4 != count_ctk) {
        result = trg;
        snprintf(msg, sizeof(msg), "[compare_triggers_v4_ctk] count %d != %ld", count_v4, count_ctk);
        ctk_log_error(msg);
    }

    for (size_t i = 0; i < count_ctk; ++i) {
        if (!compare_trigger_v4_ctk(v4, ctk, i)) {
            result = trg;
        }
    }

    return result;
}

enum summary compare_triggers_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    char msg[512];
    enum summary result = ok;

    const size_t triggers_x = ctk_reflib_reader_trigger_count(x);
    const size_t triggers_y = ctk_reflib_reader_trigger_count(y);
    if (triggers_x != triggers_y) {
        result = trg;
        snprintf(msg, sizeof(msg), "[compare_triggers_ctk] count %ld != %ld", triggers_x, triggers_y);
        ctk_log_error(msg);
    }

    for (size_t i = 0; i < triggers_x; ++i) {
        if (!compare_trigger_ctk(x, y, i)) {
            result = trg;
        }
    }

    return result;
}

enum summary compare_triggers_v4(cntfile_t x, cntfile_t y) {
    char msg[512];
    enum summary result = ok;

    const int triggers_x = libeep_get_trigger_count(x);
    const int triggers_y = libeep_get_trigger_count(y);

    if (triggers_x < 0 || triggers_y < 0) {
        snprintf(msg, sizeof(msg), "[compare_triggers_v4] invalid count %d, %d", triggers_x, triggers_y);
        ctk_log_error(msg);
        return trg;
    }

    if (triggers_x != triggers_y) {
        result = trg;
        snprintf(msg, sizeof(msg), "[compare_triggers_v4] count %d != %d", triggers_x, triggers_y);
        ctk_log_error(msg);
    }

    for (int i = 0; i < triggers_x; ++i) {
        if (!compare_trigger_v4(x, y, i)) {
            result = trg;
        }
    }

    return result;
}


enum summary compare_metadata_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk, enum bool* v4_truncated_scale) {
    enum summary status = ok;
    status |= compare_electrodes_v4_ctk(v4, ctk, v4_truncated_scale);
    status |= compare_start_time_v4_ctk(v4, ctk);
    status |= compare_sample_rate_v4_ctk(v4, ctk);
    status |= compare_sample_count_v4_ctk(v4, ctk);
    status |= compare_subject_v4_ctk(v4, ctk);
    status |= compare_institution_v4_ctk(v4, ctk);
    status |= compare_equipment_v4_ctk(v4, ctk);
    status |= compare_experiment_v4_ctk(v4, ctk);
    status |= compare_triggers_v4_ctk(v4, ctk);
    return status;
}


enum summary compare_meta_data_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    enum summary status = ok;
    status |= compare_electrodes_ctk(x, y);
    status |= compare_start_time_ctk(x, y);
    status |= compare_sample_rate_ctk(x, y);
    status |= compare_sample_count_ctk(x, y);
    status |= compare_subject_ctk(x, y);
    status |= compare_institution_ctk(x, y);
    status |= compare_equipment_ctk(x, y);
    status |= compare_experiment_ctk(x, y);
    status |= compare_triggers_ctk(x, y);
    return status;
}

enum summary compare_meta_data_v4(cntfile_t x, cntfile_t y) {
    enum summary status = ok;
    status |= compare_electrodes_v4(x, y);
    status |= compare_start_time_v4(x, y);
    status |= compare_sample_rate_v4(x, y);
    status |= compare_sample_count_v4(x, y);
    status |= compare_subject_v4(x, y);
    status |= compare_institution_v4(x, y);
    status |= compare_equipment_v4(x, y);
    status |= compare_experiment_v4(x, y);
    status |= compare_triggers_v4(x, y);
    return status;
}


enum summary compare_sample_data_v4_ctk(cntfile_t v4, ctk_reflib_reader* ctk, enum bool v4_truncated_scale) {
    char msg[512];
    enum summary result = ok;

    const int64_t samples_ctk = ctk_reflib_reader_sample_count(ctk);
    const size_t electrodes_ctk = ctk_reflib_reader_electrode_count(ctk);
    if (samples_ctk < 1 || electrodes_ctk < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_v4_ctk] ctk invalid matrix dimensions %ldx%ld", electrodes_ctk, samples_ctk);
        ctk_log_error(msg);
        return header_elc | header_smpl;
    }

    float* sample_ctk = (float*)malloc(electrodes_ctk * sizeof(float));
    if (!sample_ctk) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_v4_ctk] can not allocate 1x%ld matrix", electrodes_ctk);
        ctk_log_error(msg);
        return aux;
    }

    float max_diff = 0;
    int max_diff_channel = -1;
    for (int64_t sample = 0; sample < samples_ctk; ++sample) {
        int64_t received = ctk_reflib_reader_v4(ctk, sample, 1, sample_ctk, electrodes_ctk);
        if (received != 1) {
            snprintf(msg, sizeof(msg), "[compare_sample_data_v4_ctk] ctk can not access sample %ld", sample);
            ctk_log_warning(msg);
            continue;
        }

        float* sample_v4 = libeep_get_samples(v4, (long)sample, (long)(sample + 1));
        if (!sample_v4) {
            snprintf(msg, sizeof(msg), "[compare_sample_data_v4_ctk] v4 can not access sample %ld", sample);
            ctk_log_warning(msg);
            continue;
        }

        for (size_t electrode = 0; electrode < electrodes_ctk; ++electrode) {
            if (sample_v4[electrode] != sample_ctk[electrode]) {
                const float diff = (float)fabs((float)(sample_v4[electrode] - sample_ctk[electrode]));
                if (max_diff < diff) { // disregards the unit
                    max_diff = diff;
                    max_diff_channel = (int)electrode;
                }
            }
        }
        libeep_free_samples(sample_v4);
    }

    free(sample_ctk);

    if (max_diff_channel != -1) {
        const char* unit = ctk_reflib_reader_electrode_unit(ctk, max_diff_channel);
        snprintf(msg, sizeof(msg), "[compare_sample_data_v4_ctk] maximum value difference %.23f%s", max_diff, unit);

        if (v4_truncated_scale) {
            ctk_log_warning(msg);
        }
        else {
            result = eeg_data;
            ctk_log_error(msg);
        }
    }

    return result;
}


enum summary compare_sample_data_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    char msg[512];
    enum summary result = ok;
    double* matrix_x = NULL;
    double* matrix_y = NULL;

    const int64_t samples_x = ctk_reflib_reader_sample_count(x);
    const int64_t samples_y = ctk_reflib_reader_sample_count(y);
    const size_t electrodes_x = ctk_reflib_reader_electrode_count(x);
    const size_t electrodes_y = ctk_reflib_reader_electrode_count(y);

    if (samples_x < 1 || electrodes_x < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] x invalid matrix dimensions %ldx%ld", electrodes_x, samples_x);
        ctk_log_error(msg);
        return header_elc | header_smpl;
    }

    if (samples_y < 1 || electrodes_y < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] y invalid matrix dimensions %ldx%ld", electrodes_y, samples_y);
        ctk_log_error(msg);
        return header_elc | header_smpl;
    }

    if (samples_x != samples_y) {
        result = header_smpl;
        snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] sample count x = %ld y = %ld", samples_x, samples_y);
        ctk_log_error(msg);
        goto cleanup;
    }

    if (electrodes_x != electrodes_y) {
        result = header_elc;
        snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] electrode count x = %ld y = %ld", electrodes_x, electrodes_y);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int64_t chunk = 1024 * 4;
    const size_t area = electrodes_x * (size_t)chunk;
    matrix_x = (double*)malloc(area * sizeof(double));
    if (!matrix_x) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] x can not allocate %ldx%ld matrix", electrodes_x, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    matrix_y = (double*)malloc(area * sizeof(double));
    if (!matrix_y) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] y can not allocate %ldx%ld matrix", electrodes_x, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (int64_t sample = 0; sample < samples_x; sample += chunk) {
        const int64_t remaining = samples_x - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;
        const size_t due_size = (size_t)due * electrodes_x;

        const int64_t received_x = ctk_reflib_reader_row_major(x, sample, due, matrix_x, due_size);
        if (received_x != due) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] x can not access range [%ld, %ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        const int64_t received_y = ctk_reflib_reader_row_major(y, sample, due, matrix_y, due_size);
        if (received_y != due) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] y can not access range [%ld, %ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        for (size_t i = 0; i < due_size; ++i) {
            if (matrix_x[i] != matrix_y[i]) {
                result = eeg_data;
                snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] data mismatch at %ld/%ld: %f != %f", i, due_size, matrix_x[i], matrix_y[i]);
                ctk_log_error(msg);
                goto cleanup;
            }
        }
    }

cleanup:

    if (matrix_x) free(matrix_x);
    if (matrix_y) free(matrix_y);

    return result;
}


enum summary compare_sample_data_v4(cntfile_t x, cntfile_t y) {
    char msg[512];

    const int electrodes_x = libeep_get_channel_count(x);
    const int electrodes_y = libeep_get_channel_count(y);
    const long samples_x = libeep_get_sample_count(x);
    const long samples_y = libeep_get_sample_count(y);
    if (electrodes_x < 1 || samples_x < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_v4] x invalid matrix dimensions %dx%ld", electrodes_x, samples_x);
        ctk_log_error(msg);
        return eeg_data;
    }

    if (electrodes_y < 1 || samples_y < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_v4] y invalid matrix dimensions %dx%ld", electrodes_y, samples_y);
        ctk_log_error(msg);
        return eeg_data;
    }

    if (samples_x != samples_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_v4] sample count x = %ld y = %ld", samples_x, samples_y);
        ctk_log_error(msg);
        return eeg_data;
    }

    if (electrodes_x != electrodes_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_v4] electrode count x = %d y = %d", electrodes_x, electrodes_y);
        ctk_log_error(msg);
        return eeg_data;
    }

    enum summary result = ok;
    const int64_t chunk = 1024 * 4;
    for (int64_t sample = 0; sample < samples_x; sample += chunk) {
        const int64_t remaining = samples_x - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;
        const size_t due_size = (size_t)due * (size_t)electrodes_x;

        float* matrix_x = libeep_get_samples(x, (long)sample, (long)(sample + due));
        if (!matrix_x) {
            snprintf(msg, sizeof(msg), "[compare_sample_data_v4] x can not access range [%ld, %ld)", sample, sample + due);
            continue;
        }
        float* matrix_y = libeep_get_samples(x, (long)sample, (long)(sample + due));
        if (!matrix_y) {
            free(matrix_x);
            snprintf(msg, sizeof(msg), "[compare_sample_data_v4] y can not access range [%ld, %ld)", sample, sample + due);
            ctk_log_error(msg);
            continue;
        }

        for (size_t i = 0; i < due_size; ++i) {
            if (matrix_x[i] != matrix_y[i]) {
                result = eeg_data;
                snprintf(msg, sizeof(msg), "[compare_sample_data_v4] data mismatch at %ld/%ld: %f != %f", i, due_size, matrix_x[i], matrix_y[i]);
                ctk_log_error(msg);
                break;
            }
        }

        libeep_free_samples(matrix_x);
        libeep_free_samples(matrix_y);
    }

    return result;
}




enum summary compare_files_ctk(const char* fname_x, const char* fname_y) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[compare_files_ctk] processing '%s' and '%s'", last_n(fname_x, 40), last_n(fname_y, 40));
    ctk_log_info(msg);

    ctk_reflib_reader* reader_x = NULL;
    ctk_reflib_reader* reader_y = NULL;
    enum summary result = ok;

    stderr_compare_begin(fname_x, fname_y, "ctk");

    reader_x = ctk_reflib_reader_make(fname_x);
    if (!reader_x) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_ctk] x can not open '%s' for reading", last_n(fname_x, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname_x);
        goto cleanup;
    }

    reader_y = ctk_reflib_reader_make(fname_y);
    if (!reader_y) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_ctk] y can not open '%s' for reading", last_n(fname_y, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname_y);
        goto cleanup;
    }

    result |= compare_meta_data_ctk(reader_x, reader_y);
    result |= compare_sample_data_ctk(reader_x, reader_y);
    stderr_print_success(result);

cleanup:

    ctk_reflib_reader_dispose(reader_x);
    ctk_reflib_reader_dispose(reader_y);
    return result;
}


enum summary compare_files_v4(const char* fname_x, const char* fname_y) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[compare_files_v4] processing '%s' and '%s'", last_n(fname_x, 40), last_n(fname_y, 40));
    ctk_log_info(msg);

    cntfile_t reader_x = -1;
    cntfile_t reader_y = -1;
    enum summary result = ok;

    stderr_compare_begin(fname_x, fname_y, " v4");

    reader_x = libeep_read(fname_x);
    if (reader_x == -1) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_v4] x can not open '%s' for reading", last_n(fname_x, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname_x);
        goto cleanup;
    }

    reader_y = libeep_read(fname_y);
    if (reader_y == -1) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_v4] y can not open '%s' for reading", last_n(fname_y, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname_y);
        goto cleanup;
    }

    result |= compare_meta_data_v4(reader_x, reader_y);
    result |= compare_sample_data_v4(reader_x, reader_y);
    stderr_print_success(result);

cleanup:

    if (reader_x != -1) libeep_close(reader_x);
    if (reader_y != -1) libeep_close(reader_y);

    return result;
}


enum summary compare_reader_v4_ctk(const char* fname) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[compare_reader_v4_ctk] processing (v4, ctk) '%s'", last_n(fname, 40));
    ctk_log_info(msg);

    ctk_reflib_reader* ctk = NULL;
    cntfile_t v4 = -1;
    enum summary result = ok;

    stderr_compare_1file_2readers_begin(fname, " v4", "ctk");

    ctk = ctk_reflib_reader_make(fname);
    if (!ctk) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_reader_v4_ctk] ctk can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        stderr_compare_1file_2readers_failed("ctk");
        goto cleanup;
    }

    v4 = libeep_read(fname);
    if(v4 == -1) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_reader_v4_ctk] v4 can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        stderr_compare_1file_2readers_failed(" v4");
        goto cleanup;
    }

    enum bool v4_truncated_scale = nope;
    result |= compare_metadata_v4_ctk(v4, ctk, &v4_truncated_scale);
    result |= compare_sample_data_v4_ctk(v4, ctk, v4_truncated_scale);
    stderr_print_success(result);

cleanup:

    ctk_reflib_reader_dispose(ctk);
    if (v4 != -1) libeep_close(v4);
    return result;
}


enum summary copy_ctk2ctk(ctk_reflib_reader* reader, ctk_reflib_writer* writer) {
    enum summary result = ok;
    double* matrix = NULL;
    char msg[512];
    char stamp_str[256];

    const size_t electrodes = ctk_reflib_reader_electrode_count(reader);
    const int64_t samples = ctk_reflib_reader_sample_count(reader);
    if (samples < 1 || electrodes < 1) {
        result = header_elc | header_smpl;
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] invalid matrix dimensions %ldx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (size_t i = 0; i < electrodes; ++i) {
        const char* label = ctk_reflib_reader_electrode_label(reader, i);
        const char* ref = ctk_reflib_reader_electrode_reference(reader, i);
        const char* unit = ctk_reflib_reader_electrode_unit(reader, i);
        const double iscale = ctk_reflib_reader_electrode_iscale(reader, i);
        const double rscale = ctk_reflib_reader_electrode_rscale(reader, i);

        if (ctk_reflib_writer_electrode(writer, label, ref, unit, iscale, rscale) != EXIT_SUCCESS) {
            result = header_elc;
            snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write electrode %ld: '%s'-'%s' '%s' %f %f", i, label, ref, unit, iscale, rscale);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

    const double rate = ctk_reflib_reader_sampling_frequency(reader);
    if (ctk_reflib_writer_sampling_frequency(writer, rate) != EXIT_SUCCESS) {
        result = header_srate;
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write sampling frequency %f", rate);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int64_t epoch = ctk_reflib_reader_epoch_length(reader);
    if (ctk_reflib_writer_epoch_length(writer, epoch) != EXIT_SUCCESS) {
        result = header_epoch;
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write epoch length %ld", epoch);
        ctk_log_error(msg);
        goto cleanup;
    }

    const struct timespec stamp = ctk_reflib_reader_start_time(reader);
    if (ctk_reflib_writer_start_time(writer, &stamp) != EXIT_SUCCESS) {
        result = header_stamp;
        print_timespec(&stamp, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write eeg start time %s", stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t triggers = ctk_reflib_reader_trigger_count(reader);
    int64_t trigger_sample;
    char trigger_code[9];
    for (size_t i = 0; i < triggers; ++i) {
        if (ctk_reflib_reader_trigger(reader, i, &trigger_sample, trigger_code, sizeof(trigger_code)) != EXIT_SUCCESS) {
            result = trg;
            snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not read trigger %ld", i);
            ctk_log_error(msg);
            goto cleanup;
        }

        if (ctk_reflib_writer_trigger(writer, trigger_sample, trigger_code) != EXIT_SUCCESS) {
            result = trg;
            snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write trigger %ld: %ld '%s'", i, trigger_sample, trigger_code);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

    const char* id = ctk_reflib_reader_subject_id(reader);
    const char* name = ctk_reflib_reader_subject_name(reader);
    const char* address = ctk_reflib_reader_subject_address(reader);
    const char* phone = ctk_reflib_reader_subject_phone(reader);
    const char sex = ctk_reflib_reader_subject_sex(reader);
    const char hand = ctk_reflib_reader_subject_handedness(reader);
    const struct timespec dob = ctk_reflib_reader_subject_dob(reader);
    if (ctk_reflib_writer_subject(writer, id, name, address, phone, sex, hand, &dob) != EXIT_SUCCESS) {
        result = info;
        print_timespec(&dob, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write: id '%s', name '%s', address '%s', phone '%s', sex %c, hand %c, dob %s", id, name, address, phone, sex, hand, stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* hospital = ctk_reflib_reader_hospital(reader);
    const char* physician = ctk_reflib_reader_physician(reader);
    const char* technician = ctk_reflib_reader_technician(reader);
    if (ctk_reflib_writer_institution(writer, hospital, physician, technician) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write: hospital '%s', physician '%s', technician '%s'", hospital, physician, technician);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* make = ctk_reflib_reader_machine_make(reader);
    const char* model = ctk_reflib_reader_machine_model(reader);
    const char* sn = ctk_reflib_reader_machine_sn(reader);
    if (ctk_reflib_writer_equipment(writer, make, model, sn) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write: make '%s', model '%s', sn '%s'", make, model, sn);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* test_name = ctk_reflib_reader_test_name(reader);
    const char* test_serial = ctk_reflib_reader_test_serial(reader);
    const char* comment = ctk_reflib_reader_comment(reader);
    if (ctk_reflib_writer_experiment(writer, test_name, test_serial, comment) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write: test name '%s', test serial '%s', comment '%s'", test_name, test_serial, comment);
        ctk_log_error(msg);
        goto cleanup;
    }

    const long chunk = 1024 * 4;
    const size_t area = electrodes * (size_t)chunk;
    matrix = (double*)malloc(area * sizeof(double));
    if (!matrix) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not allocate %ldx%ld matrix", electrodes, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (int64_t sample = 0; sample < samples; sample += chunk) {
        const int64_t remaining = samples - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;
        const size_t due_size = (size_t)due * electrodes;

        const int64_t received = ctk_reflib_reader_row_major(reader, sample, due, matrix, due_size);
        if (received != due) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not read range [%ld-%ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        if (ctk_reflib_writer_row_major(writer, matrix, due_size) != EXIT_SUCCESS) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not write range [%ld-%ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

cleanup:

    if (matrix) free(matrix);
    ctk_reflib_writer_close(writer);
    return result;
}


enum summary copy_v42ctk(cntfile_t reader, ctk_reflib_writer* writer) {
    enum summary result = ok;
    char msg[512];
    char stamp_str[256];

    const int electrodes = libeep_get_channel_count(reader);
    const long samples = libeep_get_sample_count(reader);
    if (samples < 1 || electrodes < 1) {
        result = header_elc | header_smpl;
        snprintf(msg, sizeof(msg), "[copy_v42ctk] invalid matrix dimensions %dx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (int i = 0; i < electrodes; ++i) {
        const char* label = libeep_get_channel_label(reader, i);
        const char* ref = libeep_get_channel_reference(reader, i);
        const char* unit = libeep_get_channel_unit(reader, i);
        const float scale = libeep_get_channel_scale(reader, i);

        if (ctk_reflib_writer_electrode(writer, label, ref, unit, 1, scale) != EXIT_SUCCESS) {
            result = header_elc;
            snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write electrode %d: '%s'-'%s' '%s' %f", i, label, ref, unit, scale);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

    const int rate = libeep_get_sample_frequency(reader);
    if (ctk_reflib_writer_sampling_frequency(writer, (double)rate) != EXIT_SUCCESS) {
        result = header_srate;
        snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write sampling frequency %d", rate);
        ctk_log_error(msg);
        goto cleanup;
    }

    double day_seconds, subseconds;
    libeep_get_start_date_and_fraction(reader, &day_seconds, &subseconds);
    struct timespec stamp;
    if (ctk_dcdate2timespec(day_seconds, subseconds, &stamp) != EXIT_SUCCESS) {
        result = header_stamp;
        snprintf(msg, sizeof(msg), "[copy_v42ctk] conversion of (%f, %f) to time failed", day_seconds, subseconds);
        ctk_log_error(msg);
        goto cleanup;
    }
    if (ctk_reflib_writer_start_time(writer, &stamp) != EXIT_SUCCESS) {
        result = header_stamp;
        print_timespec(&stamp, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write eeg start time (%f %f) [%s]", day_seconds, subseconds, stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int triggers = libeep_get_trigger_count(reader);
    if (triggers < 0) {
        snprintf(msg, sizeof(msg), "[copy_v42ctk] invalid trigger count %d", triggers);
        ctk_log_error(msg);
        return trg;
    }
    uint64_t trigger_sample;
    for (size_t i = 0; i < triggers; ++i) {
        const char* trigger_code = libeep_get_trigger(reader, (int)i, &trigger_sample);

        if (LLONG_MAX < trigger_sample) {
            result = trg;
            snprintf(msg, sizeof(msg), "[copy_v42ctk] trigger sample %d > %lld (max)", triggers, LLONG_MAX);
            ctk_log_error(msg);
            continue;
        }

        if (ctk_reflib_writer_trigger(writer, (int64_t)trigger_sample, trigger_code) != EXIT_SUCCESS) {
            result = trg;
            snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write trigger %ld: %ld '%s'", i, trigger_sample, trigger_code);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

    const char* id = libeep_get_patient_id(reader);
    const char* name = libeep_get_patient_name(reader);
    const char* address = libeep_get_patient_address(reader);
    const char* phone = libeep_get_patient_phone(reader);
    const char sex = libeep_get_patient_sex(reader);
    const char hand = libeep_get_patient_handedness(reader);
    int year, month, day;
    libeep_get_date_of_birth(reader, &year, &month, &day);
    struct tm ymd;
    memset(&ymd, 0, sizeof(ymd));
    ymd.tm_year = year - 1900;
    ymd.tm_mon = month - 1;
    ymd.tm_mday = day;
    struct timespec dob;
    if (ctk_tm2timespec(&ymd, &dob) != EXIT_SUCCESS) {
        result = info;
        ctk_log_error("[copy_eeg2ctk] dob conversion from tm");
        goto cleanup;
    }
    if (ctk_reflib_writer_subject(writer, id, name, address, phone, sex, hand, &dob) != EXIT_SUCCESS) {
        result = info;
        print_timespec(&dob, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write: id '%s', name '%s', address '%s', phone '%s', sex %c, hand %c, dob %s", id, name, address, phone, sex, hand, stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* hospital = libeep_get_hospital(reader);
    const char* physician = libeep_get_physician(reader);
    const char* technician = libeep_get_technician(reader);
    if (ctk_reflib_writer_institution(writer, hospital, physician, technician) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write: hospital '%s', physician '%s', technician '%s'", hospital, physician, technician);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* make = libeep_get_machine_make(reader);
    const char* model = libeep_get_machine_model(reader);
    const char* sn = libeep_get_machine_serial_number(reader);
    if (ctk_reflib_writer_equipment(writer, make, model, sn) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write: make '%s', model '%s', sn '%s'", make, model, sn);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* test_name = libeep_get_test_name(reader);
    const char* test_serial = libeep_get_test_serial(reader);
    const char* comment = libeep_get_comment(reader);
    if (ctk_reflib_writer_experiment(writer, test_name, test_serial, comment) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write: test name '%s', test serial '%s', comment '%s'", test_name, test_serial, comment);
        ctk_log_error(msg);
        goto cleanup;
    }

    const long chunk = 1024 * 4;
    for (long sample = 0; sample < samples; sample += chunk) {
        const long remaining = samples - sample;
        const long due = chunk < remaining ? chunk : remaining;
        const size_t due_size = (size_t)due * electrodes;

        float* matrix = libeep_get_samples(reader, sample, sample + due);
        if (!matrix) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[copy_v42ctk] can not read range [%ld-%ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        if (ctk_reflib_writer_v4(writer, matrix, due_size) != EXIT_SUCCESS) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[copy_v42ctk] can not write range [%ld-%ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        libeep_free_samples(matrix);
    }

cleanup:

    ctk_reflib_writer_close(writer);
    return result;
}

enum bool ctkread_ctkwrite_compareall(const char* fname) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[ctkread_ctkwrite_compareall] processing '%s'", last_n(fname, 40));
    ctk_log_error(msg);

    const char* delme_cnt = "ctkread_ctkwrite_compareall.cnt";
    stderr_copy_begin(fname, delme_cnt, "ctk", "ctk");

    ctk_reflib_reader* reader_ctk = NULL;
    ctk_reflib_writer* writer_ctk = NULL;
    enum summary result = ok;

    reader_ctk = ctk_reflib_reader_make(fname);
    if (!reader_ctk) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[ctkread_ctkwrite_compareall] can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname);
        goto cleanup;
    }

    const int cnt64 = 1;
    writer_ctk = ctk_reflib_writer_make(delme_cnt, cnt64);
    if (!writer_ctk) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[ctkread_ctkwrite_compareall] can not open '%s' for writing", delme_cnt);
        ctk_log_error(msg);
        stderr_failed_writer(delme_cnt);
        goto cleanup;
    }

    result = copy_ctk2ctk(reader_ctk, writer_ctk); // closes writer_ctk
    stderr_print_success(result);

    result |= compare_reader_v4_ctk(delme_cnt);
    result |= compare_files_v4(fname, delme_cnt);
    result |= compare_files_ctk(fname, delme_cnt);

cleanup:

    ctk_reflib_writer_dispose(writer_ctk);
    ctk_reflib_reader_dispose(reader_ctk);
    remove(delme_cnt);

    return result == ok;
}


enum bool v4read_ctkwrite_compareall(const char* fname) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[v4read_ctkwrite_compareall] processing '%s'", last_n(fname, 40));
    ctk_log_info(msg);

    ctk_reflib_writer* writer_ctk = NULL;
    cntfile_t reader_v4 = -1;
    enum summary result = ok;

    const char* delme_cnt = "v4read_ctkwrite_compareall.cnt";
    stderr_copy_begin(fname, delme_cnt, " v4", "ctk");

    reader_v4 = libeep_read(fname);
    if(reader_v4 == -1) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[v4read_ctkwrite_compareall] v4 can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname);
        goto cleanup;
    }

    const int cnt64 = 1;
    writer_ctk = ctk_reflib_writer_make(delme_cnt, cnt64);
    if (!writer_ctk) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[v4read_ctkwrite_compareall] ctk can not open '%s' for writing", delme_cnt);
        ctk_log_error(msg);
        stderr_failed_writer(fname);
        goto cleanup;
    }

    result = copy_v42ctk(reader_v4, writer_ctk); // closes writer_ctk
    stderr_print_success(result);

    result |= compare_reader_v4_ctk(delme_cnt);
    result |= compare_files_v4(fname, delme_cnt);
    result |= compare_files_ctk(fname, delme_cnt);

cleanup:

    ctk_reflib_writer_dispose(writer_ctk);
    if (reader_v4 != -1) libeep_close(reader_v4);
    remove(delme_cnt);

    return result == ok;
}


int main(int argc, char* argv[]) {
    (void)(argc); // unused
    (void)(argv); // unused

    libeep_init();

    if (ctk_set_logger("file", "warning") != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    struct input_txt cnt_files = make_input_txt();
    const char* fname = NULL;

    // compatibility: reader
    // reads a file with both eep and ctk and compares the data
    stderr_intermezzo("compatibility, reader");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        compare_reader_v4_ctk(fname);
        fname = next(&cnt_files);
    }

    // compatibility: writer
    // reads source with ctk, writes temporary with ctk
    // compares temporary with ctk and eep
    // compares source and temporary with ctk
    // compares source and temporary with eep
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    stderr_intermezzo("compatibility, writer: source ctk");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        ctkread_ctkwrite_compareall(fname);
        fname = next(&cnt_files);
    }

    // nope
    // compatibility: writer
    // reads source with eep, writes temporary with ctk
    // compares temporary with ctk and eep
    // compares source and temporary with ctk
    // compares source and temporary with eep
    /*
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    stderr_intermezzo("compatibility, writer: source v4");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        v4read_ctkwrite_compareall(fname);
        fname = next(&cnt_files);
    }
    */

    // performance: reader
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    stderr_intermezzo("performance, reader");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        //compare_reader_performance(fname, 1);
        compare_reader_performance(fname, 1024 * 4);
        fname = next(&cnt_files);
    }

    // performance: writer
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    stderr_intermezzo("performance, writer");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        //compare_writer_performance(fname, 1);
        compare_writer_performance(fname, 1024 * 4);
        fname = next(&cnt_files);
    }

    dispose(&cnt_files);
    libeep_exit();
    return EXIT_SUCCESS;
}

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32
