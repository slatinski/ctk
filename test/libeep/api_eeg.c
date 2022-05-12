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
#include <string.h>
#include <time.h>

#include "eep/eepio.h"
#include "cnt/cnt.h"
#include "cnt/trg.h"
#include "api_c.h"
#include "input_txt.h"
#include "comparison.h"

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4477)
#endif // WIN32


/* PERFORMANCE */

// reads as fast as possible, eeg
int64_t accessible_chunks_eeg(const char* fname, long chunk) {
    FILE* f = NULL;
    eeg_t* eeg = NULL;
    sraw_t* matrix = NULL;
    int status = CNTERR_NONE;
    int64_t accessible = 0;
    char msg[512];

    f = eepio_fopen(fname, "rb");
    if (!f) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_eeg] can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        goto cleanup;
    }

    eeg = eep_init_from_file(fname, f, &status);
    if (status != CNTERR_NONE) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_eeg] can not initialize from file '%s'", last_n(fname, 40));
        ctk_log_error(msg);
        goto cleanup;
    }

    matrix = (sraw_t*)(malloc(CNTBUF_SIZE(eeg, chunk)));
    if (!matrix) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_eeg] can not allocate matrix\n");
        ctk_log_error(msg);
        goto cleanup;
    }

    const uint64_t samples = eep_get_samplec(eeg);
    const int relative = 0;
    for (uint64_t sample = 0; sample < samples; sample += chunk) {
        const uint64_t remaining = samples - sample;
        const uint64_t due = chunk < remaining ? chunk : remaining;

        status = eep_seek(eeg, DATATYPE_EEG, sample, relative);
        if(status != CNTERR_NONE) {
            snprintf(msg, sizeof(msg), "[accessible_chunks_eeg] can not seek to sample %ld\n", sample);
            ctk_log_warning(msg);
            continue;
        }

        status = eep_read_sraw(eeg, DATATYPE_EEG, matrix, due);
        if(status != CNTERR_NONE) {
            snprintf(msg, sizeof(msg), "[accessible_chunks_eeg] can not access range [%ld-%ld)\n", sample, sample + due);
            ctk_log_warning(msg);
            continue;
        }

        accessible += due;
    }

cleanup:

    if (matrix) free(matrix);
    if (eeg) eep_free(eeg);
    if (f) eepio_fclose(f);

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
        snprintf(msg, sizeof(msg), "[accessible_chunks_ctk] invalid dimensions: channels %ld, samples %ld\n", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t area = (size_t)chunk * electrodes;
    int32_t* matrix = (int32_t*)malloc(area * sizeof(int32_t));
    if (!matrix) {
        snprintf(msg, sizeof(msg), "[accessible_chunks_ctk] can not allocate matrix\n");
        ctk_log_error(msg);
        goto cleanup;
    }
    const int64_t matrix_size = (int64_t)area;

    for (int64_t sample = 0; sample < samples; sample += chunk) {
        const int64_t remaining = samples - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;

        const int64_t received = ctk_reflib_reader_column_major_int32(ctk, sample, due, matrix, matrix_size);
        if (received != due) {
            snprintf(msg, sizeof(msg), "[accessible_chunks_ctk] can not access range [%ld-%ld)\n", sample, sample + due);
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


// writes as fast as possible, eeg
int64_t write_in_chunks_eeg(const char* fname, long chunk) {
    FILE* f = NULL;
    ctk_reflib_reader* reader = NULL;
    eeg_t* writer = NULL;
    sraw_t* matrix = NULL;
    char msg[512];
    char stamp_str[256];
    int64_t written = 0;
    const char* delme_cnt = "write_in_chunks_eeg.cnt";
    eegchan_t* chan = NULL;

    reader = ctk_reflib_reader_make(fname);
    if (!reader) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] ctk can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t electrodes = ctk_reflib_reader_electrode_count(reader);
    if (SHRT_MAX < electrodes) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] channel count %zu > %d (max)", electrodes, SHRT_MAX);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int64_t samples = ctk_reflib_reader_sample_count(reader);
    if (electrodes < 1 || samples < 1) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] invalid matrix dimensions %ldx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    const size_t area = electrodes * (size_t)chunk;
    matrix = (sraw_t*)malloc(area * sizeof(sraw_t));
    if (!matrix) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] can not allocate %ldx%ld matrix", electrodes, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int64_t epoch = ctk_reflib_reader_epoch_length(reader);
    if (epoch < 0) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] negative epoch length %ld", epoch);
        ctk_log_error(msg);
        goto cleanup;
    }

    chan = eep_chan_init((short)electrodes);
    if (!chan) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] eeg eep_chan_init '%s'", delme_cnt);
        ctk_log_error(msg);
        goto cleanup;
    }
    for (short i = 0; i < (short)electrodes; ++i) {
        const char* label = ctk_reflib_reader_electrode_label(reader, i);
        const char* ref = ctk_reflib_reader_electrode_reference(reader, i);
        const char* unit = ctk_reflib_reader_electrode_unit(reader, i);
        const double iscale = ctk_reflib_reader_electrode_iscale(reader, i);
        const double rscale = ctk_reflib_reader_electrode_rscale(reader, i);

        eep_chan_set(chan, i, label, iscale, rscale, unit);
        eep_chan_set_reflab(chan, i, ref);
    }

    const double rate = ctk_reflib_reader_sampling_frequency(reader);
    writer = eep_init_from_values(1.0 / rate, (short)electrodes, chan);
    if (!writer) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] eeg eep_init_from_values '%s'", delme_cnt);
        ctk_log_error(msg);
        goto cleanup;
    }

    f = eepio_fopen(delme_cnt, "wb");
    if (!f) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] eeg can not open '%s' for writing", delme_cnt);
        ctk_log_error(msg);
        goto cleanup;
    }

    if (eep_create_file(writer, delme_cnt, f, NULL, 0, "") != CNTERR_NONE) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] eeg eep_create_file '%s'", delme_cnt);
        ctk_log_error(msg);
        goto cleanup;
    }

    if(eep_prepare_to_write(writer, DATATYPE_EEG, (uint64_t)epoch, NULL) != CNTERR_NONE) {
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] eeg eep_prepare_to_write '%s'", delme_cnt);
        ctk_log_error(msg);
        goto cleanup;
    }

    const struct timespec stamp = ctk_reflib_reader_start_time(reader);
    double day_seconds, subseconds;
    if (ctk_timespec2dcdate(&stamp, &day_seconds, &subseconds) != EXIT_SUCCESS) {
        print_timespec(&stamp, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] start time conversion to dcdate %s", stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }
    record_info_t recinfo;
    memset(&recinfo, 0, sizeof(recinfo));
    recinfo.m_startDate = day_seconds;
    recinfo.m_startFraction = subseconds;
    eep_set_recording_info(writer, &recinfo);

    for (int64_t sample = 0; sample < samples; sample += chunk) {
        const int64_t remaining = samples - sample;
        const int64_t due = chunk < remaining ? chunk : remaining;
        const size_t due_size = electrodes * (size_t)due;

        const int64_t received = ctk_reflib_reader_column_major_int32(reader, sample, due, matrix, due_size);
        if (received != due) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] can not read range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        if (eep_write_sraw(writer, matrix, (uint64_t)due) != CNTERR_NONE) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_eeg] can not write range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        written += due;
    }

cleanup:

    if (writer) { eep_finish_file(writer); }
    if (f) { eepio_fclose(f); }
    if (matrix) { free(matrix); }
    ctk_reflib_reader_dispose(reader);
    remove(delme_cnt);

    return written;
}


// writes as fast as possible, ctk
int64_t write_in_chunks_ctk(const char* fname, long chunk) {
    char msg[512];
    int64_t written = 0;
    ctk_reflib_reader* reader = NULL;
    ctk_reflib_writer* writer = NULL;
    int32_t* matrix = NULL;
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
    matrix = (int32_t*)malloc(area * sizeof(int32_t));
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

        const int64_t received = ctk_reflib_reader_column_major_int32(reader, sample, due, matrix, due_size);
        if (received != due) {
            snprintf(msg, sizeof(msg), "[write_in_chunks_ctk] can not read range [%ld-%ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        if (ctk_reflib_writer_column_major_int32(writer, matrix, due_size) != EXIT_SUCCESS) {
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
    return written;
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

    const clock_t b_eeg = clock();
    const int64_t accessible_eeg = accessible_chunks_eeg(fname, chunk);
    const clock_t e_eeg = clock();
    const double t_eeg = ((double)(e_eeg - b_eeg)) / CLOCKS_PER_SEC;

    if (accessible_ctk != accessible_eeg) {
        stderr_speed_end_incomparable();
        return;
    }

    stderr_speed_end("eeg", t_eeg, "ctk", t_ctk);
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

    const clock_t b_eeg = clock();
    const int64_t written_eeg = write_in_chunks_eeg(fname, chunk);
    const clock_t e_eeg = clock();
    const double t_eeg = ((double)(e_eeg - b_eeg)) / CLOCKS_PER_SEC;

    if (written_ctk != written_eeg) {
        stderr_speed_end_incomparable();
        return;
    }

    stderr_speed_end("eeg", t_eeg, "ctk", t_ctk);
}



/* COMPATIBILITY */


enum bool compare_electrode_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk, size_t i) {
    const char* label_eeg = eep_get_chan_label(eeg, (short)i);
    const char* ref_eeg = eep_get_chan_reflab(eeg, (short)i);
    const char* unit_eeg = eep_get_chan_unit(eeg, (short)i);
    const double is_eeg = eep_get_chan_iscale(eeg, (short)i);
    const double rs_eeg = eep_get_chan_rscale(eeg, (short)i);

    const char* label_ctk = ctk_reflib_reader_electrode_label(ctk, i);
    const char* ref_ctk = ctk_reflib_reader_electrode_reference(ctk, i);
    const char* unit_ctk = ctk_reflib_reader_electrode_unit(ctk, i);
    const double is_ctk = ctk_reflib_reader_electrode_iscale(ctk, i);
    const double rs_ctk = ctk_reflib_reader_electrode_rscale(ctk, i);

    return equal_electrode(label_eeg, ref_eeg, unit_eeg, is_eeg, rs_eeg
                          , label_ctk, ref_ctk, unit_ctk, is_ctk, rs_ctk
                          , "compare_electrode_eeg_ctk");
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

enum bool compare_electrode_eeg(eeg_t* x, eeg_t* y, short i) {
    const char* label_x = eep_get_chan_label(x, i);
    const char* ref_x = eep_get_chan_reflab(x, i);
    const char* unit_x = eep_get_chan_unit(x, i);
    const double is_x = eep_get_chan_iscale(x, i);
    const double rs_x = eep_get_chan_rscale(x, i);

    const char* label_y = eep_get_chan_label(y, i);
    const char* ref_y = eep_get_chan_reflab(y, i);
    const char* unit_y = eep_get_chan_unit(y, i);
    const double is_y = eep_get_chan_iscale(y, i);
    const double rs_y = eep_get_chan_rscale(y, i);

    return equal_electrode(label_x, ref_x, unit_x, is_x, rs_x
                          , label_y, ref_y, unit_y, is_y, rs_y
                          , "compare_electrode_eeg_ctk");
}

enum summary compare_electrodes_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    char msg[512];
    enum summary result = ok;

    const short electrodes_eeg = eep_get_chanc(eeg);
    if (electrodes_eeg < 0) {
        snprintf(msg, sizeof(msg), "[compare_electrodes_eeg_ctk] eeg negative count %d", electrodes_eeg);
        ctk_log_error(msg);
        return header_elc;
    }

    const size_t electrodes_ctk = ctk_reflib_reader_electrode_count(ctk);
    if (electrodes_eeg != electrodes_ctk) {
        result = header_elc;
        snprintf(msg, sizeof(msg), "[compare_electrodes_eeg_ctk] count %d != %ld", electrodes_eeg, electrodes_ctk);
        ctk_log_error(msg);
    }

    for (size_t i = 0; i < electrodes_ctk; ++i) {
        if (!compare_electrode_eeg_ctk(eeg, ctk, i)) {
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

enum summary compare_electrodes_eeg(eeg_t* x, eeg_t* y) {
    char msg[512];
    enum summary result = ok;

    const short electrodes_x = eep_get_chanc(x);
    if (electrodes_x < 0) {
        snprintf(msg, sizeof(msg), "[compare_electrodes_eeg] x negative count %d", electrodes_x);
        ctk_log_error(msg);
        return header_elc;
    }

    const short electrodes_y = eep_get_chanc(y);
    if (electrodes_y < 0) {
        snprintf(msg, sizeof(msg), "[compare_electrodes_eeg] y negative count %d", electrodes_y);
        ctk_log_error(msg);
        return header_elc;
    }

    if (electrodes_x != electrodes_y) {
        result = header_elc;
        snprintf(msg, sizeof(msg), "[compare_electrodes_eeg] count %d != %d", electrodes_x, electrodes_y);
        ctk_log_error(msg);
    }

    for (short i = 0; i < electrodes_x; ++i) {
        if (!compare_electrode_eeg(x, y, i)) {
            result = header_elc;
        }
    }

    return result;
}

enum summary compare_start_time_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    record_info_t info;
    eep_get_recording_info(eeg, &info);

    const struct timespec time_ctk = ctk_reflib_reader_start_time(ctk);

    return equal_date_timespec( info.m_startDate, info.m_startFraction, &time_ctk
                              , "compare_start_time_eeg_ctk") ? ok : header_stamp; // bool -> summary
}

enum summary compare_start_time_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    const struct timespec stamp_x = ctk_reflib_reader_start_time(x);
    const struct timespec stamp_y = ctk_reflib_reader_start_time(y);

    return equal_timespec(&stamp_x, &stamp_y, "compare_start_time_ctk") ?  ok : header_stamp; // bool -> summary
}

enum summary compare_start_time_eeg(eeg_t* x, eeg_t* y) {
    record_info_t info_x, info_y;
    eep_get_recording_info(x, &info_x);
    eep_get_recording_info(y, &info_y);
    return equal_date( info_x.m_startDate, info_x.m_startFraction
                     , info_y.m_startDate, info_y.m_startFraction
                     , "compare_start_time_eeg") ? ok : header_stamp; // bool -> summary
}

enum summary compare_sample_rate_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    char msg[512];

    const double period_eeg = eep_get_period(eeg);
    const double rate_eeg = 1. / period_eeg;
    const double rate_ctk = ctk_reflib_reader_sampling_frequency(ctk);

    if (rate_eeg != rate_ctk) {
        if ((1.0 / (1.0 / rate_ctk)) != rate_ctk) {
            snprintf(msg, sizeof(msg), "[compare_sample_rate_eeg_ctk] period/rate roundtrip %f", rate_ctk);
            ctk_log_warning(msg);
            return ok;
        }

        snprintf(msg, sizeof(msg), "[compare_sample_rate_eeg_ctk] %f != %f", rate_eeg, rate_ctk);
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


enum summary compare_sample_rate_eeg(eeg_t* x, eeg_t* y) {
    char msg[512];
    const double period_x = eep_get_period(x);
    const double period_y = eep_get_period(y);

    if (period_x != period_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_rate_eeg] %f != %f", 1.0 / period_x, 1.0 / period_y);
        ctk_log_error(msg);
        return header_srate;
    }

    return ok;
}

enum summary compare_sample_count_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    char msg[512];

    const uint64_t samples_eeg = eep_get_samplec(eeg);
    const int64_t samples_ctk = ctk_reflib_reader_sample_count(ctk);
    if (samples_ctk < 0 || samples_eeg != (uint64_t)samples_ctk) {
        snprintf(msg, sizeof(msg), "[compare_sample_count_eeg_ctk] %ld != %ld", samples_eeg, samples_ctk);
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

enum summary compare_sample_count_eeg(eeg_t* x, eeg_t* y) {
    char msg[512];

    const uint64_t samples_x = eep_get_samplec(x);
    const uint64_t samples_y = eep_get_samplec(y);
    if (samples_x != samples_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_count_eeg] %ld != %ld", samples_x, samples_y);
        ctk_log_error(msg);
        return header_smpl;
    }

    return ok;
}

enum summary compare_subject_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    const char* id_eeg = eep_get_patient_id(eeg);
    const char* name_eeg = eep_get_patient_name(eeg);
    const char* address_eeg = eep_get_patient_address(eeg);
    const char* phone_eeg = eep_get_patient_phone(eeg);
    const char sex_eeg = eep_get_patient_sex(eeg);
    const char hand_eeg = eep_get_patient_handedness(eeg);
    const struct tm* dob_eeg = eep_get_patient_day_of_birth(eeg);

    const char* id_ctk = ctk_reflib_reader_subject_id(ctk);
    const char* name_ctk = ctk_reflib_reader_subject_name(ctk);
    const char* address_ctk = ctk_reflib_reader_subject_address(ctk);
    const char* phone_ctk = ctk_reflib_reader_subject_phone(ctk);
    const char sex_ctk = ctk_reflib_reader_subject_sex(ctk);
    const char hand_ctk = ctk_reflib_reader_subject_handedness(ctk);
    const struct timespec dob_ctk = ctk_reflib_reader_subject_dob(ctk);

    return equal_subject_eeg_ctk( id_eeg, name_eeg, address_eeg, phone_eeg, sex_eeg, hand_eeg, dob_eeg
                                , id_ctk, name_ctk, address_ctk, phone_ctk, sex_ctk, hand_ctk, &dob_ctk
                                , "compare_subject_eeg_ctk") ? ok : info; // bool -> summary
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

enum summary compare_subject_eeg(eeg_t* x, eeg_t* y) {
    const char* id_x = eep_get_patient_id(x);
    const char* name_x = eep_get_patient_name(x);
    const char* address_x = eep_get_patient_address(x);
    const char* phone_x = eep_get_patient_phone(x);
    const char sex_x = eep_get_patient_sex(x);
    const char hand_x = eep_get_patient_handedness(x);
    const struct tm* dob_x = eep_get_patient_day_of_birth(x);

    const char* id_y = eep_get_patient_id(y);
    const char* name_y = eep_get_patient_name(y);
    const char* address_y = eep_get_patient_address(y);
    const char* phone_y = eep_get_patient_phone(y);
    const char sex_y = eep_get_patient_sex(y);
    const char hand_y = eep_get_patient_handedness(y);
    const struct tm* dob_y = eep_get_patient_day_of_birth(y);

    return equal_subject_eeg_eeg( id_x, name_x, address_x, phone_x, sex_x, hand_x, dob_x
                                , id_y, name_y, address_y, phone_y, sex_y, hand_y, dob_y
                                , "compare_subject_eeg_eeg") ? ok : info; // bool -> summary
}


enum summary compare_institution_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    const char* hospital_eeg = eep_get_hospital(eeg);
    const char* physician_eeg = eep_get_physician(eeg);
    const char* technician_eeg = eep_get_technician(eeg);

    const char* hospital_ctk = ctk_reflib_reader_hospital(ctk);
    const char* physician_ctk = ctk_reflib_reader_physician(ctk);
    const char* technician_ctk = ctk_reflib_reader_technician(ctk);

    return equal_institution(hospital_eeg, physician_eeg, technician_eeg
                            , hospital_ctk, physician_ctk, technician_ctk
                            , "compare_institution_eeg_ctk") ? ok : info; // bool -> summary
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

enum summary compare_institution_eeg(eeg_t* x, eeg_t* y) {
    const char* hospital_x = eep_get_hospital(x);
    const char* physician_x = eep_get_physician(x);
    const char* technician_x = eep_get_technician(x);

    const char* hospital_y = eep_get_hospital(y);
    const char* physician_y = eep_get_physician(y);
    const char* technician_y = eep_get_technician(y);

    return equal_institution(hospital_x, physician_x, technician_x
                            , hospital_y, physician_y, technician_y
                            , "compare_institution_eeg") ? ok : info; // bool -> summary
}

enum summary compare_equipment_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    const char* make_ctk = ctk_reflib_reader_machine_make(ctk);
    const char* model_ctk = ctk_reflib_reader_machine_model(ctk);
    const char* sn_ctk = ctk_reflib_reader_machine_sn(ctk);

    const char* make_eeg = eep_get_machine_make(eeg);
    const char* model_eeg = eep_get_machine_model(eeg);
    const char* sn_eeg = eep_get_machine_serial_number(eeg);

    return equal_equipment( make_eeg, model_eeg, sn_eeg
                          , make_ctk, model_ctk, sn_ctk
                          , "compare_equipment_eeg_ctk") ? ok : info; // bool -> summary
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

enum summary compare_equipment_eeg(eeg_t* x, eeg_t* y) {
    const char* make_x = eep_get_machine_make(x);
    const char* model_x = eep_get_machine_model(x);
    const char* sn_x = eep_get_machine_serial_number(x);

    const char* make_y = eep_get_machine_make(y);
    const char* model_y = eep_get_machine_model(y);
    const char* sn_y = eep_get_machine_serial_number(y);

    return equal_equipment( make_x, model_x, sn_x
                          , make_y, model_y, sn_y
                          , "compare_equipment_eeg") ? ok : info; // bool -> summary
}

enum summary compare_experiment_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    const char* name_eeg = eep_get_test_name(eeg);
    const char* serial_eeg = eep_get_test_serial(eeg);
    const char* comment_eeg = eep_get_comment(eeg);

    const char* name_ctk = ctk_reflib_reader_test_name(ctk);
    const char* serial_ctk = ctk_reflib_reader_test_serial(ctk);
    const char* comment_ctk = ctk_reflib_reader_comment(ctk);

    return equal_experiment( name_eeg, serial_eeg, comment_eeg
                           , name_ctk, serial_ctk, comment_ctk
                           , "compare_experiment_eeg_ctk") ? ok : info; // bool -> summary
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

enum summary compare_experiment_eeg(eeg_t* x, eeg_t* y) {
    const char* name_x = eep_get_test_name(x);
    const char* serial_x = eep_get_test_serial(x);
    const char* comment_x = eep_get_comment(x);

    const char* name_y = eep_get_test_name(y);
    const char* serial_y = eep_get_test_serial(y);
    const char* comment_y = eep_get_comment(y);

    return equal_experiment( name_x, serial_x, comment_x
                           , name_y, serial_y, comment_y
                           , "compare_experiment_eeg") ? ok : info; // bool -> summary
}


enum bool compare_trigger_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk, size_t i) {
    char msg[512];

    char code_ctk[9];
    int64_t sample_ctk;
    if (ctk_reflib_reader_trigger(ctk, i, &sample_ctk, code_ctk, sizeof(code_ctk)) != EXIT_SUCCESS) {
        snprintf(msg, sizeof(msg), "[compare_trigger_eeg_ctk] ctk can not obtain trigger %ld", i);
        ctk_log_error(msg);
        return nope;
    }

    uint64_t sample_eeg;
    trg_t* trg = eep_get_trg(eeg);
    const char* code_eeg = trg_get(trg, (int)i, &sample_eeg);

    return equal_trigger_u64_s64(code_eeg, sample_eeg, code_ctk, sample_ctk, "compare_trigger_eeg_ctk");
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

enum bool compare_trigger_eeg(eeg_t* x, eeg_t* y, int i) {
    trg_t* trg_x = eep_get_trg(x);
    trg_t* trg_y = eep_get_trg(y);
    if (!trg_x || !trg_y) {
        return nope;
    }

    uint64_t sample_x, sample_y;
    const char* code_x = trg_get(trg_x, i, &sample_x);
    const char* code_y = trg_get(trg_y, i, &sample_y);

    return equal_trigger_u64_u64(code_x, sample_x, code_y, sample_y, "compare_trigger_eeg");
}

enum summary compare_triggers_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    char msg[512];
    enum summary result = ok;

    trg_t* handle = eep_get_trg(eeg);
    if (!handle) {
        snprintf(msg, sizeof(msg), "[compare_triggers_eeg_ctk] eep_get_trg failed");
        ctk_log_error(msg);
        return trg;
    }
    const int count_eeg = trg_get_c(handle);
    if (count_eeg < 0) {
        snprintf(msg, sizeof(msg), "[compare_triggers_eeg_ctk] eeg invalid count %d", count_eeg);
        ctk_log_error(msg);
        return trg;
    }

    const size_t count_ctk = ctk_reflib_reader_trigger_count(ctk);
    if (count_eeg != count_ctk) {
        result = trg;
        snprintf(msg, sizeof(msg), "[compare_triggers_eeg_ctk] count %d != %ld", count_eeg, count_ctk);
        ctk_log_error(msg);
    }

    for (size_t i = 0; i < count_ctk; ++i) {
        if (!compare_trigger_eeg_ctk(eeg, ctk, i)) {
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

enum summary compare_triggers_eeg(eeg_t* x, eeg_t* y) {
    char msg[512];
    enum summary result = ok;

    trg_t* handle_x = eep_get_trg(x);
    trg_t* handle_y = eep_get_trg(y);
    if (!handle_x && !handle_y) {
        return ok;
    }

    if (!handle_x || !handle_y) {
        const char* valid_x = eep_get_trg(x) ? "present" : "absent";
        const char* valid_y = eep_get_trg(y) ? "present" : "absent";
        snprintf(msg, sizeof(msg), "[compare_triggers_eeg] handle %s != %s", valid_x, valid_y);
        ctk_log_error(msg);
        return trg;
    }

    const int count_x = trg_get_c(handle_x);
    const int count_y = trg_get_c(handle_y);
    if (count_x < 0 || count_y < 0) {
        snprintf(msg, sizeof(msg), "[compare_triggers_eeg] invalid count x %d, y %d", count_x, count_y);
        ctk_log_error(msg);
        return trg;
    }

    for (int i = 0; i < count_x; ++i) {
        if (!compare_trigger_eeg(x, y, i)) {
            result = trg;
        }
    }

    return result;
}

enum summary compare_metadata_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    enum summary status = ok;
    status |= compare_electrodes_eeg_ctk(eeg, ctk);
    status |= compare_start_time_eeg_ctk(eeg, ctk);
    status |= compare_sample_rate_eeg_ctk(eeg, ctk);
    status |= compare_sample_count_eeg_ctk(eeg, ctk);
    status |= compare_subject_eeg_ctk(eeg, ctk);
    status |= compare_institution_eeg_ctk(eeg, ctk);
    status |= compare_equipment_eeg_ctk(eeg, ctk);
    status |= compare_experiment_eeg_ctk(eeg, ctk);
    status |= compare_triggers_eeg_ctk(eeg, ctk);
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

enum summary compare_meta_data_eeg(eeg_t* x, eeg_t* y) {
    enum summary status = ok;
    status |= compare_electrodes_eeg(x, y);
    status |= compare_start_time_eeg(x, y);
    status |= compare_sample_rate_eeg(x, y);
    status |= compare_sample_count_eeg(x, y);
    status |= compare_subject_eeg(x, y);
    status |= compare_institution_eeg(x, y);
    status |= compare_equipment_eeg(x, y);
    status |= compare_experiment_eeg(x, y);
    status |= compare_triggers_eeg(x, y);
    return status;
}


enum summary compare_sample_data_eeg_ctk(eeg_t* eeg, ctk_reflib_reader* ctk) {
    char msg[512];
    enum summary result = ok;
    sraw_t* sample_eeg = NULL;
    sraw_t* sample_ctk = NULL;

    const int64_t samples_ctk = ctk_reflib_reader_sample_count(ctk);
    const size_t electrodes_ctk = ctk_reflib_reader_electrode_count(ctk);
    if (samples_ctk < 1 || electrodes_ctk < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg_ctk] ctk invalid matrix dimensions %ldx%ld", electrodes_ctk, samples_ctk);
        ctk_log_error(msg);
        return header_elc | header_smpl;
    }

    sample_eeg = (sraw_t*)malloc(electrodes_ctk * sizeof(sraw_t));
    if (!sample_eeg) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg_ctk] eeg can not allocate 1x%ld matrix", electrodes_ctk);
        ctk_log_error(msg);
        goto cleanup;
    }

    sample_ctk = (sraw_t*)malloc(electrodes_ctk * sizeof(sraw_t));
    if (!sample_ctk) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg_ctk] ctk can not allocate 1x%ld matrix", electrodes_ctk);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int relative = 0;
    int status = CNTERR_NONE;
    for (int64_t sample = 0; sample < samples_ctk; ++sample) {
        int64_t received = ctk_reflib_reader_column_major_int32(ctk, sample, 1, sample_ctk, electrodes_ctk);
        if (received != 1) {
            snprintf(msg, sizeof(msg), "[compare_sample_data_eeg_ctk] ctk can not access sample %ld", sample);
            ctk_log_warning(msg);
            continue;
        }

        status = eep_seek(eeg, DATATYPE_EEG, sample, relative);
        if(status != CNTERR_NONE) {
            snprintf(msg, sizeof(msg), "[compare_sample_data_eeg_ctk] eeg can not seek to sample %ld\n", sample);
            ctk_log_warning(msg);
            continue;
        }

        status = eep_read_sraw(eeg, DATATYPE_EEG, sample_eeg, 1);
        if(status != CNTERR_NONE) {
            snprintf(msg, sizeof(msg), "[compare_sample_data_eeg_ctk] eeg can not access sample %ld", sample);
            ctk_log_warning(msg);
            continue;
        }

        for (size_t electrode = 0; electrode < electrodes_ctk; ++electrode) {
            if (sample_eeg[electrode] != sample_ctk[electrode]) {
                result = eeg_data;
                snprintf(msg, sizeof(msg), "[compare_sample_data_eeg_ctk] data mismatch at sample %ld channel %ld: %d != %d", sample, electrode, sample_eeg[electrode], sample_ctk[electrode]);
                ctk_log_error(msg);
                goto cleanup;
            }
        }
    }

cleanup:

    if (sample_eeg) { free(sample_eeg); }
    if (sample_ctk) { free(sample_ctk); }

    return result;
}

enum summary compare_sample_data_ctk(ctk_reflib_reader* x, ctk_reflib_reader* y) {
    char msg[512];
    enum summary result = ok;
    int32_t* matrix_x = NULL;
    int32_t* matrix_y = NULL;

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
    matrix_x = (int32_t*)malloc(area * sizeof(int32_t));
    if (!matrix_x) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] x can not allocate %ldx%ld matrix", electrodes_x, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    matrix_y = (int32_t*)malloc(area * sizeof(int32_t));
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

        const int64_t received_x = ctk_reflib_reader_row_major_int32(x, sample, due, matrix_x, due_size);
        if (received_x != due) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] x can not access range [%ld, %ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        const int64_t received_y = ctk_reflib_reader_row_major_int32(y, sample, due, matrix_y, due_size);
        if (received_y != due) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] y can not access range [%ld, %ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        for (size_t i = 0; i < due_size; ++i) {
            if (matrix_x[i] != matrix_y[i]) {
                result = eeg_data;
                snprintf(msg, sizeof(msg), "[compare_sample_data_ctk] data mismatch at %ld/%ld: %d != %d", i, due_size, matrix_x[i], matrix_y[i]);
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

enum summary compare_sample_data_eeg(eeg_t* x, eeg_t* y) {
    char msg[512];
    sraw_t* matrix_x = NULL;
    sraw_t* matrix_y = NULL;
    enum summary result = ok;

    const short electrodes_x = eep_get_chanc(x);
    const short electrodes_y = eep_get_chanc(y);
    const uint64_t samples_x = eep_get_samplec(x);
    const uint64_t samples_y = eep_get_samplec(y);
    if (electrodes_x < 1 || samples_x < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] x invalid matrix dimensions %dx%ld", electrodes_x, samples_x);
        ctk_log_error(msg);
        return eeg_data;
    }

    if (electrodes_y < 1 || samples_y < 1) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] y invalid matrix dimensions %dx%ld", electrodes_y, samples_y);
        ctk_log_error(msg);
        return eeg_data;
    }

    if (samples_x != samples_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] sample count x = %ld y = %ld", samples_x, samples_y);
        ctk_log_error(msg);
        return eeg_data;
    }

    if (electrodes_x != electrodes_y) {
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] electrode count x = %d y = %d", electrodes_x, electrodes_y);
        ctk_log_error(msg);
        return eeg_data;
    }

    const long chunk = 1024 * 4;
    const size_t area = electrodes_x * (size_t)chunk;
    matrix_x = (sraw_t*)malloc(area * sizeof(sraw_t));
    if (!matrix_x) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] x can not allocate %hdx%ld matrix", electrodes_x, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    matrix_y = (sraw_t*)malloc(area * sizeof(sraw_t));
    if (!matrix_y) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] y can not allocate %hdx%ld matrix", electrodes_x, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    int status = CNTERR_NONE;
    const int relative = 0;
    for (long sample = 0; sample < (long)samples_x; sample += chunk) {
        const long remaining = (long)samples_x - sample;
        const long due = chunk < remaining ? chunk : remaining;
        const size_t due_size = (size_t)due * (size_t)electrodes_x;

        status = eep_seek(x, DATATYPE_EEG, sample, relative);
        if(status != CNTERR_NONE) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] x can not seek to sample %ld\n", sample);
            ctk_log_warning(msg);
            goto cleanup;
        }

        status = eep_read_sraw(x, DATATYPE_EEG, matrix_x, due);
        if(status != CNTERR_NONE) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] x can not access range [%ld, %ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        status = eep_seek(y, DATATYPE_EEG, sample, relative);
        if(status != CNTERR_NONE) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] y can not seek to sample %ld\n", sample);
            ctk_log_warning(msg);
            goto cleanup;
        }

        status = eep_read_sraw(y, DATATYPE_EEG, matrix_y, due);
        if(status != CNTERR_NONE) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] y can not access range [%ld, %ld)", sample, sample + due);
            ctk_log_warning(msg);
            goto cleanup;
        }

        for (size_t i = 0; i < due_size; ++i) {
            if (matrix_x[i] != matrix_y[i]) {
                result = eeg_data;
                snprintf(msg, sizeof(msg), "[compare_sample_data_eeg] data mismatch at %ld/%ld: %d != %d", i, due_size, matrix_x[i], matrix_y[i]);
                ctk_log_error(msg);
                goto cleanup;
            }
        }
    }

cleanup:

    if (matrix_x) { free(matrix_x); }
    if (matrix_y) { free(matrix_y); }

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

enum summary compare_files_eeg(const char* fname_x, const char* fname_y) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[compare_files_eeg] processing '%s' and '%s'", last_n(fname_x, 40), last_n(fname_y, 40));
    ctk_log_info(msg);

    eeg_t* reader_x = NULL;
    eeg_t* reader_y = NULL;
    FILE* f_x = NULL;
    FILE* f_y = NULL;
    enum summary result = ok;
    int status = CNTERR_NONE;

    stderr_compare_begin(fname_x, fname_y, "eeg");

    f_x = eepio_fopen(fname_x, "rb");
    if (!f_x) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_eeg] x can not open %s for reading\n", fname_x);
        ctk_log_error(msg);
        stderr_failed_reader(fname_x);
        goto cleanup;
    }

    reader_x = eep_init_from_file(fname_x, f_x, &status);
    if (status != CNTERR_NONE) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_eeg] x can not initialize from file %s\n", fname_x);
        ctk_log_error(msg);
        stderr_failed_reader(fname_x);
        goto cleanup;
    }

    f_y = eepio_fopen(fname_y, "rb");
    if (!f_y) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_eeg] y can not open %s for reading\n", fname_y);
        ctk_log_error(msg);
        stderr_failed_reader(fname_y);
        goto cleanup;
    }

    reader_y = eep_init_from_file(fname_y, f_y, &status);
    if (status != CNTERR_NONE) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_files_eeg] y can not initialize from file %s\n", fname_y);
        ctk_log_error(msg);
        stderr_failed_reader(fname_y);
        goto cleanup;
    }

    result |= compare_meta_data_eeg(reader_x, reader_y);
    result |= compare_sample_data_eeg(reader_x, reader_y);
    stderr_print_success(result);

cleanup:

    if (reader_x) eep_free(reader_x);
    if (reader_y) eep_free(reader_y);
    if (f_x) eepio_fclose(f_x);
    if (f_y) eepio_fclose(f_y);

    return result;
}

enum summary compare_reader_eeg_ctk(const char* fname) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[compare_reader_eeg_ctk] processing (eeg, ctk) '%s'", last_n(fname, 40));
    ctk_log_info(msg);

    ctk_reflib_reader* ctk = NULL;
    eeg_t* eeg = NULL;
    FILE* f = NULL;
    enum summary result = ok;
    int status = CNTERR_NONE;

    stderr_compare_1file_2readers_begin(fname, "eeg", "ctk");

    ctk = ctk_reflib_reader_make(fname);
    if (!ctk) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_reader_eeg_ctk] ctk can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        stderr_compare_1file_2readers_failed("ctk");
        goto cleanup;
    }

    f = eepio_fopen(fname, "rb");
    if (!f) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_reader_eeg_ctk] eeg can not open %s for reading\n", fname);
        ctk_log_error(msg);
        stderr_compare_1file_2readers_failed("eeg");
        goto cleanup;
    }

    eeg = eep_init_from_file(fname, f, &status);
    if (status != CNTERR_NONE) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[compare_reader_eeg_ctk] eeg can not initialize from file %s\n", fname);
        ctk_log_error(msg);
        stderr_compare_1file_2readers_failed("eeg");
        goto cleanup;
    }

    result |= compare_metadata_eeg_ctk(eeg, ctk);
    result |= compare_sample_data_eeg_ctk(eeg, ctk);
    stderr_print_success(result);

cleanup:

    ctk_reflib_reader_dispose(ctk);
    if (eeg) eep_free(eeg);
    if (f) eepio_fclose(f);

    return result;
}


enum summary copy_ctk2ctk(ctk_reflib_reader* reader, ctk_reflib_writer* writer) {
    enum summary result = ok;
    int32_t* matrix = NULL;
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
    matrix = (int32_t*)malloc(area * sizeof(int32_t));
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

        const int64_t received = ctk_reflib_reader_row_major_int32(reader, sample, due, matrix, due_size);
        if (received != due) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[copy_ctk2ctk] can not read range [%ld-%ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }

        if (ctk_reflib_writer_row_major_int32(writer, matrix, due_size) != EXIT_SUCCESS) {
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

enum summary copy_eeg2ctk(eeg_t* reader, ctk_reflib_writer* writer) {
    char msg[512];
    char stamp_str[256];
    enum summary result = ok;
    int32_t* matrix = NULL;

    const short electrodes = eep_get_chanc(reader);
    const uint64_t samples = eep_get_samplec(reader);
    if (samples < 1 || electrodes < 1) {
        result = header_elc | header_smpl;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] invalid matrix dimensions %dx%ld", electrodes, samples);
        ctk_log_error(msg);
        goto cleanup;
    }

    for (short i = 0; i < electrodes; ++i) {
        const char* label = eep_get_chan_label(reader, i);
        const char* ref = eep_get_chan_reflab(reader, i);
        const char* unit = eep_get_chan_unit(reader, i);
        const double iscale = eep_get_chan_iscale(reader, i);
        const double rscale = eep_get_chan_rscale(reader, i);

        if (ctk_reflib_writer_electrode(writer, label, ref, unit, iscale, rscale) != EXIT_SUCCESS) {
            result = header_elc;
            snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write electrode %d: '%s'-'%s' '%s' %f %f", i, label, ref, unit, iscale, rscale);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

    const double period = eep_get_period(reader);
    const double rate = 1.0 / period;
    if (ctk_reflib_writer_sampling_frequency(writer, rate) != EXIT_SUCCESS) {
        result = header_srate;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write sampling frequency %f", rate);
        ctk_log_error(msg);
        goto cleanup;
    }

    const int epoch = eep_get_epochl(reader, DATATYPE_EEG);
    if (ctk_reflib_writer_epoch_length(writer, epoch) != EXIT_SUCCESS) {
        result = header_epoch;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write epoch length %d", epoch);
        ctk_log_error(msg);
        goto cleanup;
    }

    record_info_t recinfo;
    eep_get_recording_info(reader, &recinfo);
    struct timespec stamp;
    if (ctk_dcdate2timespec(recinfo.m_startDate, recinfo.m_startFraction, &stamp) != EXIT_SUCCESS) {
        result = header_stamp;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] conversion of (%f, %f) to time failed", recinfo.m_startDate, recinfo.m_startFraction);
        ctk_log_error(msg);
        goto cleanup;
    }
    if (ctk_reflib_writer_start_time(writer, &stamp) != EXIT_SUCCESS) {
        result = header_stamp;
        print_timespec(&stamp, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write eeg start time (%f %f) [%s]", recinfo.m_startDate, recinfo.m_startFraction, stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* id = eep_get_patient_id(reader);
    const char* name = eep_get_patient_name(reader);
    const char* address = eep_get_patient_address(reader);
    const char* phone = eep_get_patient_phone(reader);
    const char sex = eep_get_patient_sex(reader);
    const char hand = eep_get_patient_handedness(reader);
    const struct tm* dob_tm = eep_get_patient_day_of_birth(reader);
    struct timespec dob;
    if (ctk_tm2timespec(dob_tm, &dob) != EXIT_SUCCESS) {
        dob.tv_sec = 0;
        dob.tv_nsec = 0;
        ctk_log_warning("[copy_eeg2ctk] dob conversion from tm, replacing with zero");
    }
    if (ctk_reflib_writer_subject(writer, id, name, address, phone, sex, hand, &dob) != EXIT_SUCCESS) {
        result = info;
        print_timespec(&dob, stamp_str, sizeof(stamp_str));
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write: id '%s', name '%s', address '%s', phone '%s', sex %c, hand %c, dob %s", id, name, address, phone, sex, hand, stamp_str);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* hospital = eep_get_hospital(reader);
    const char* physician = eep_get_physician(reader);
    const char* technician = eep_get_technician(reader);
    if (ctk_reflib_writer_institution(writer, hospital, physician, technician) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write: hospital '%s', physician '%s', technician '%s'", hospital, physician, technician);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* make = eep_get_machine_make(reader);
    const char* model = eep_get_machine_model(reader);
    const char* sn = eep_get_machine_serial_number(reader);
    if (ctk_reflib_writer_equipment(writer, make, model, sn) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write: make '%s', model '%s', sn '%s'", make, model, sn);
        ctk_log_error(msg);
        goto cleanup;
    }

    const char* test_name = eep_get_test_name(reader);
    const char* test_serial = eep_get_test_serial(reader);
    const char* comment = eep_get_comment(reader);
    if (ctk_reflib_writer_experiment(writer, test_name, test_serial, comment) != EXIT_SUCCESS) {
        result = info;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write: test name '%s', test serial '%s', comment '%s'", test_name, test_serial, comment);
        ctk_log_error(msg);
        goto cleanup;
    }


    trg_t* handle = eep_get_trg(reader);
    if (handle) {
        const int triggers = trg_get_c(handle);
        if (triggers < 0) {
            result = trg;
            snprintf(msg, sizeof(msg), "[copy_eeg2ctk] invalid trigger count %d", triggers);
            ctk_log_error(msg);
            goto cleanup;
        }

        uint64_t trigger_sample;
        for (int i = 0; i < triggers; ++i) {
            const char* trigger_code = trg_get(handle, i, &trigger_sample);

            if (LLONG_MAX < trigger_sample) {
                result = trg;
                snprintf(msg, sizeof(msg), "[copy_eeg2ctk] trigger sample %d > %lld (max)", triggers, LLONG_MAX);
                ctk_log_error(msg);
                continue;
            }

            if (ctk_reflib_writer_trigger(writer, (int64_t)trigger_sample, trigger_code) != EXIT_SUCCESS) {
                result = trg;
                snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write trigger %d: %ld '%s'", i, trigger_sample, trigger_code);
                ctk_log_error(msg);
                goto cleanup;
            }
        }
    }

    const long chunk = 1024 * 4;
    const size_t area = electrodes * (size_t)chunk;
    matrix = (int32_t*)malloc(area * sizeof(int32_t));
    if (!matrix) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not allocate %hdx%ld matrix", electrodes, chunk);
        ctk_log_error(msg);
        goto cleanup;
    }

    int status = CNTERR_NONE;
    const int relative = 0;
    for (long sample = 0; sample < (long)samples; sample += chunk) {
        const long remaining = (long)samples - sample;
        const long due = chunk < remaining ? chunk : remaining;
        const size_t due_size = (size_t)due * electrodes;

        status = eep_seek(reader, DATATYPE_EEG, sample, relative);
        if(status != CNTERR_NONE) {
            snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not seek to sample %ld\n", sample);
            ctk_log_warning(msg);
            continue;
        }

        status = eep_read_sraw(reader, DATATYPE_EEG, matrix, due);
        if(status != CNTERR_NONE) {
            snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not access range [%ld-%ld)\n", sample, sample + due);
            ctk_log_warning(msg);
            continue;
        }

        if (ctk_reflib_writer_column_major_int32(writer, matrix, due_size) != EXIT_SUCCESS) {
            result = eeg_data;
            snprintf(msg, sizeof(msg), "[copy_eeg2ctk] can not write range [%ld-%ld)", sample, sample + due);
            ctk_log_error(msg);
            goto cleanup;
        }
    }

cleanup:

    if (matrix) free(matrix);
    ctk_reflib_writer_close(writer);
    return result;
}

enum bool ctkread_ctkwrite_compareall(const char* fname) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[ctkread_ctkwrite_compareall] processing '%s'", last_n(fname, 40));
    ctk_log_info(msg);

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
        stderr_failed_writer(fname);
        goto cleanup;
    }

    result = copy_ctk2ctk(reader_ctk, writer_ctk); // closes writer_ctk
    stderr_print_success(result);

    result |= compare_reader_eeg_ctk(delme_cnt);
    result |= compare_files_eeg(fname, delme_cnt);
    result |= compare_files_ctk(fname, delme_cnt);

cleanup:

    ctk_reflib_writer_dispose(writer_ctk);
    ctk_reflib_reader_dispose(reader_ctk);
    remove(delme_cnt);

    return result == ok;
}


enum bool eegread_ctkwrite_compareall(const char* fname) {
    char msg[512];
    snprintf(msg, sizeof(msg), "[eegread_ctkwrite_compareall] processing '%s'", last_n(fname, 40));
    ctk_log_info(msg);

    ctk_reflib_writer* writer_ctk = NULL;
    eeg_t* reader_eeg = NULL;
    FILE* f = NULL;
    int status = CNTERR_NONE;
    enum summary result = ok;

    const char* delme_cnt = "eegread_ctkwrite_compareall.cnt";
    stderr_copy_begin(fname, delme_cnt, "eeg", "ctk");

    f = eepio_fopen(fname, "rb");
    if (!f) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[eegread_ctkwrite_compareall] eeg can not open '%s' for reading", last_n(fname, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname);
        goto cleanup;
    }

    reader_eeg = eep_init_from_file(fname, f, &status);
    if (status != CNTERR_NONE) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[eegread_ctkwrite_compareall] eeg can not initialize from file '%s'", last_n(fname, 40));
        ctk_log_error(msg);
        stderr_failed_reader(fname);
        goto cleanup;
    }

    const int cnt64 = 1;
    writer_ctk = ctk_reflib_writer_make(delme_cnt, cnt64);
    if (!writer_ctk) {
        result |= aux;
        snprintf(msg, sizeof(msg), "[eegread_ctkwrite_compareall] ctk can not open '%s' for writing", delme_cnt);
        ctk_log_error(msg);
        stderr_failed_writer(fname);
        goto cleanup;
    }

    result = copy_eeg2ctk(reader_eeg, writer_ctk); // closes writer_ctk
    stderr_print_success(result);

    result |= compare_reader_eeg_ctk(delme_cnt);
    result |= compare_files_eeg(fname, delme_cnt);
    result |= compare_files_ctk(fname, delme_cnt);

cleanup:

    ctk_reflib_writer_dispose(writer_ctk);
    if (reader_eeg) eep_free(reader_eeg);
    if (f) eepio_fclose(f);
    remove(delme_cnt);

    return result == ok;
}


int main(int argc, char* argv[]) {
    (void)(argc); // unused
    (void)(argv); // unused

    if (ctk_set_logger("file", "warning") != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    struct input_txt cnt_files = make_input_txt();
    const char* fname = NULL;
    /*
    FILE *problems_txt = fopen("problems.txt", "w");
    if (!problems_txt) {
        return EXIT_FAILURE;
    }
    */

    // compatibility: reader
    // reads a file with both eeg and ctk and compares the data
    stderr_intermezzo("compatibility, reader");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        if (compare_reader_eeg_ctk(fname) != ok) {
            //fprintf(problems_txt, "%s\n", fname);
        }
        fname = next(&cnt_files);
    }

    // compatibility: writer
    // reads source with ctk, writes temporary with ctk
    // compares temporary with ctk and eeg
    // compares source and temporary with ctk
    // compares source and temporary with eeg
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    stderr_intermezzo("compatibility, writer: source ctk");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        ctkread_ctkwrite_compareall(fname);
        fname = next(&cnt_files);
    }

    // compatibility: writer
    // reads source with eeg, writes temporary with ctk
    // compares temporary with ctk and eeg
    // compares source and temporary with ctk
    // compares source and temporary with eeg
    stderr_intermezzo("compatibility, writer: source eeg");
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        eegread_ctkwrite_compareall(fname);
        fname = next(&cnt_files);
    }

    // performance: reader
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    stderr_intermezzo("performance, reader");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        compare_reader_performance(fname, 1);
        compare_reader_performance(fname, 1024 * 4);
        fname = next(&cnt_files);
    }

    // performance: writer
    if (reset(&cnt_files) != EXIT_SUCCESS) { dispose(&cnt_files); return EXIT_FAILURE; }
    stderr_intermezzo("performance, writer");
    fname = next(&cnt_files);
    while(0 < strlen(fname)) {
        compare_writer_performance(fname, 1);
        compare_writer_performance(fname, 1024 * 4);
        fname = next(&cnt_files);
    }

    dispose(&cnt_files);
    //fclose(problems_txt);
    return EXIT_SUCCESS;
}

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32
