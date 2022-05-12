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

#include "comparison.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "api_c.h"

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4477)
#endif // WIN32


enum bool equal_strings(const char* x, const char* y, const char* func) {
    if (x && y) {
        if (strcmp(x, y) == 0) {
            return yep;
        }
        return nope;
    }
    else if (!x && !y) {
        // NULL == NULL
        return yep;
    }
    else if (x && !x[0] && !y) {
        // '\0' == NULL
        return yep;
    }
    else if (!x && y && !y[0]) {
        // NULL == '\0'
        return yep;
    }
    else {
        return nope;
    }
}


enum bool equal_unit(const char* x, const char* y, const char* func) {
    if (x == NULL || y == NULL) {
        return nope;
    }

    const size_t len_x = strlen(x);
    const size_t len_y = strlen(y);
    if (len_x == 2 && len_y == 2) {
        if (x[1] == 'V' && y[1] == 'V') {
            if (x[0] == 'u' && y[0] == -75) {
                return yep;
            }
            if (x[0] == -75 && y[0] == 'u') {
                return yep;
            }
        }
    }

    return strcmp(x, y) == 0;
}


static
enum bool known_sex(char x) {
    return x == 'f' || x == 'F' || x == 'm' || x == 'M' || x == 0;
}

enum bool equal_sex(char x, char y, const char* func) {
    if (known_sex(x) && known_sex(y) && x == y) {
        return yep;
    }

    return nope;
}

static
enum bool known_handedness(char x) {
    return x == 'l' || x == 'L' || x == 'r' || x == 'R' || x == 'm' || x == 'M' || x == 0;
}

enum bool equal_handedness(char x, char y, const char* func) {
    if (known_handedness(x) && known_handedness(y) && x == y) {
        return yep;
    }

    return nope;
}

enum bool equal_date(double date_x, double frac_x, double date_y, double frac_y, const char* func) {
    const double seconds_per_day = 60 * 60 * 24;
    const double dseconds = fabs(round((date_x - date_y) * seconds_per_day));
    const double dsubseconds = fabs(frac_x - frac_y);
    const double delta = dseconds + dsubseconds;
    const double max_delta = 1e-7; // 100ns ticks on windows

    if (delta <= max_delta) {
        return yep;
    }

    return nope;
}

enum bool is_zero_tm(const struct tm* x) {
    assert(x);
    return x->tm_year == 0
        && x->tm_mon == 0
        && x->tm_mday == 0
        && x->tm_hour == 0
        && x->tm_min == 0
        && x->tm_sec == 0;
}

enum bool both_zero_tm(const struct tm* x, const struct tm* y) {
    if (!x && !y) {
        return yep;
    }
    else if (!x && y) {
        return is_zero_tm(y);
    }
    else if (x && !y) {
        return is_zero_tm(x);
    }

    return nope;
}

enum bool equal_tm(const struct tm* x, const struct tm* y, const char* func) {
    char msg[512];

    if (both_zero_tm(x, y)) {
        return yep;
    }

    if (!x || !y) {
        return nope;
    }

    return x->tm_year == y->tm_year &&
           x->tm_mon == y->tm_mon &&
           x->tm_mday == y->tm_mday &&
           x->tm_hour == y->tm_hour &&
           x->tm_min == y->tm_min &&
           x->tm_sec == y->tm_sec;
}

enum bool equal_timespec(const struct timespec* x, const struct timespec* y, const char* func) {
    if (!x && !y) {
        return yep;
    }

    if (!x || !y) {
        return nope;
    }

    if (x->tv_sec != y->tv_sec || x->tv_nsec != y->tv_nsec) {
        return nope;
    }

    return yep;
}

enum bool equal_date_timespec(double date_x, double frac_x, const struct timespec* time_y, const char* func) {
    struct timespec time_x;
    if (ctk_dcdate2timespec(date_x, frac_x, &time_x) != EXIT_SUCCESS) {
        return nope;
    }

    double date_y, frac_y;
    if (ctk_timespec2dcdate(time_y, &date_y, &frac_y) != EXIT_SUCCESS) {
        return nope;
    }

    return equal_date(date_x, frac_x, date_y, frac_y, func) && equal_timespec(&time_x, time_y, func);
}

static
enum bool electrode_strings(const char* label_x, const char* ref_x, const char* unit_x
                          , const char* label_y, const char* ref_y, const char* unit_y
                          , const char* func) {
    char msg[512];

    snprintf(msg, sizeof(msg), "%s label", func);
    if (!equal_strings(label_x, label_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s reference", func);
    if (!equal_strings(ref_x, ref_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s unit", func);
    if (!equal_unit(unit_x, unit_y, msg)) {
        return nope;
    }

    return yep;
}

enum bool equal_electrode(const char* label_x, const char* ref_x, const char* unit_x, double iscale_x, double rscale_x
                        , const char* label_y, const char* ref_y, const char* unit_y, double iscale_y, double rscale_y
                        , const char* func) {
    if (!electrode_strings(label_x, ref_x, unit_x, label_y, ref_y, unit_y, func)) {
        return nope;
    }

    if (iscale_x != iscale_y) {
        return nope;
    }

    if (rscale_x != rscale_y) {
        return nope;
    }

    return yep;
}


enum bool equal_electrode_v4(const char* label_v4, const char* ref_v4, const char* unit_v4, double scale
                           , const char* label_ctk, const char* ref_ctk, const char* unit_ctk, double iscale, double rscale
                           , const char* func, enum bool* v4_truncated_scale) {
    if (!electrode_strings(label_v4, ref_v4, unit_v4, label_ctk, ref_ctk, unit_ctk, func)) {
        return nope;
    }

    if (scale == iscale * rscale) {
        return yep;
    }

    if (scale == (float)(iscale * rscale)) {
        /*
        double eep_get_chan_scale();
        float libeep_get_channel_scale() { return (float)eep_get_chan_scale(); }
        */
        *v4_truncated_scale = yep;
        return yep;
    }

    return nope;
}

static
enum bool subject_strings(const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x
                      , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y
                      , const char* func) {
    char msg[512];

    snprintf(msg, sizeof(msg), "%s id", func);
    if (!equal_strings(id_x, id_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s name", func);
    if (!equal_strings(name_x, name_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s address", func);
    if (!equal_strings(addr_x, addr_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s phone", func);
    if (!equal_strings(phone_x, phone_y, msg)) {
        return nope;
    }

    if (!equal_sex(sex_x, sex_y, func)) {
        return nope;
    }

    if (!equal_handedness(hand_x, hand_y, func)) {
        return nope;
    }

    return yep;
}

enum bool equal_subject(const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, const struct timespec* dob_x
                      , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct timespec* dob_y
                      , const char* func) {
    if (!subject_strings( id_x, name_x, addr_x, phone_x, sex_x, hand_x
                        , id_y, name_y, addr_y, phone_y, sex_y, hand_y
                        , func)) {
        return nope;
    }

    return equal_timespec(dob_x, dob_y, func);
}

enum bool equal_subject_v4( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, int year_x, int month_x, int day_x
                          , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct timespec* dob_y
                          , const char* func) {
    if (!subject_strings( id_x, name_x, addr_x, phone_x, sex_x, hand_x
                        , id_y, name_y, addr_y, phone_y, sex_y, hand_y
                        , func)) {
        return nope;
    }

    const struct tm* dob_tm = gmtime(&dob_y->tv_sec);
    if (!dob_tm) {
        return nope;
    }
    const int year_y = dob_tm->tm_year + 1900;
    const int month_y = dob_tm->tm_mon + 1;
    const int day_y = dob_tm->tm_mday;

    if (year_x !=  year_y || month_x != month_y || day_x != day_y) {
        return nope;
    }

    return yep;
}

enum bool equal_subject_v4_v4( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, int year_x, int month_x, int day_x
                             , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, int year_y, int month_y, int day_y
                             , const char* func) {
    if (!subject_strings( id_x, name_x, addr_x, phone_x, sex_x, hand_x
                        , id_y, name_y, addr_y, phone_y, sex_y, hand_y
                        , func)) {
        return nope;
    }

    if (year_x !=  year_y || month_x != month_y || day_x != day_y) {
        return nope;
    }

    return yep;
}

enum bool equal_subject_eeg_ctk( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, const struct tm* dob_x
                               , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct timespec* dob_y
                               , const char* func) {
    if (!subject_strings( id_x, name_x, addr_x, phone_x, sex_x, hand_x
                        , id_y, name_y, addr_y, phone_y, sex_y, hand_y
                        , func)) {
        return nope;
    }

    assert(dob_y);
    if (!dob_x || is_zero_tm(dob_x)) {
        if (dob_y && dob_y->tv_sec == 0 && dob_y->tv_nsec == 0) {
            return yep;
        }

        return nope;
    }

    struct timespec dob_ts;
    if (ctk_tm2timespec(dob_x, &dob_ts) != EXIT_SUCCESS) {
        return nope;
    }

    struct tm dob_tm;
    if (ctk_timespec2tm(dob_y, &dob_tm) != EXIT_SUCCESS) {
        return nope;
    }

    return equal_tm(dob_x, &dob_tm, func) && equal_timespec(&dob_ts, dob_y, func);
}


enum bool equal_subject_eeg_eeg( const char* id_x, const char* name_x, const char* addr_x, const char* phone_x, char sex_x, char hand_x, const struct tm* dob_x
                               , const char* id_y, const char* name_y, const char* addr_y, const char* phone_y, char sex_y, char hand_y, const struct tm* dob_y
                               , const char* func) {
    char msg[512];

    if (!subject_strings( id_x, name_x, addr_x, phone_x, sex_x, hand_x
                        , id_y, name_y, addr_y, phone_y, sex_y, hand_y
                        , func)) {
        return nope;
    }

    if (both_zero_tm(dob_x, dob_y)) {
        return yep;
    }

    if (!dob_x || !dob_y) {
        return nope;
    }

    return dob_x->tm_year == dob_y->tm_year
        && dob_x->tm_mon == dob_y->tm_mon
        && dob_x->tm_mday == dob_y->tm_mday
        && dob_x->tm_hour == dob_y->tm_hour
        && dob_x->tm_min == dob_y->tm_min
        && dob_x->tm_sec == dob_y->tm_sec;
}


enum bool equal_institution(const char* hospital_x, const char* physician_x, const char* technician_x
                      , const char* hospital_y, const char* physician_y, const char* technician_y
                      , const char* func) {
    char msg[512];

    snprintf(msg, sizeof(msg), "%s hospital", func);
    if (!equal_strings(hospital_x, hospital_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s physician", func);
    if (!equal_strings(physician_x, physician_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s technician", func);
    if (!equal_strings(technician_x, technician_y, msg)) {
        return nope;
    }

    return yep;
}

enum bool equal_equipment( const char* make_x, const char* model_x, const char* sn_x
                         , const char* make_y, const char* model_y, const char* sn_y
                         , const char* func) {
    char msg[512];

    snprintf(msg, sizeof(msg), "%s machine make", func);
    if (!equal_strings(make_x, make_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s machine model", func);
    if (!equal_strings(model_x, model_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s machine sn", func);
    if (!equal_strings(sn_x, sn_y, msg)) {
        return nope;
    }

    return yep;
}

enum bool equal_experiment( const char* name_x, const char* serial_x, const char* comment_x
                          , const char* name_y, const char* serial_y, const char* comment_y
                          , const char* func) {
    char msg[512];

    snprintf(msg, sizeof(msg), "%s test name", func);
    if (!equal_strings(name_x, name_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s test serial", func);
    if (!equal_strings(serial_x, serial_y, msg)) {
        return nope;
    }

    snprintf(msg, sizeof(msg), "%s comment", func);
    if (!equal_strings(comment_x, comment_y, msg)) {
        return nope;
    }

    return yep;
}

enum bool equal_trigger(const char* code_x, int64_t sample_x, const char* code_y, int64_t sample_y, const char* func) {
    char msg[512];

    snprintf(msg, sizeof(msg), "%s code", func);
    if (!equal_strings(code_x, code_y, msg)) {
        return nope;
    }

    if (sample_x < 0 || sample_y < 0) {
        return nope;
    }

    if (sample_x != sample_y) {
        return nope;
    }

    return yep;
}


enum bool equal_trigger_u64_s64(const char* code_x, uint64_t sample_x, const char* code_y, int64_t sample_y, const char* func) {
    char msg[512];

    if (LLONG_MAX < sample_x) {
        return nope;
    }

    return equal_trigger(code_x, (int64_t)sample_x, code_y, sample_y, "equal_trigger_u64_s64");
}


enum bool equal_trigger_u64_u64(const char* code_x, uint64_t sample_x, const char* code_y, uint64_t sample_y, const char* func) {
    char msg[512];

    if (LLONG_MAX < sample_x) {
        return nope;
    }

    if (LLONG_MAX < sample_y) {
        return nope;
    }

    return equal_trigger(code_x, (int64_t)sample_x, code_y, (int64_t)sample_y, "equal_trigger_u64_u64");
}


void print_timespec(const struct timespec* x, char* output, size_t size) {
    if (!x || !output || size == 0) {
        return;
    }

    char str[64];
    strftime(str, sizeof(str), "%F %T", gmtime(&x->tv_sec));
    snprintf(output, size, "%s.%ld UTC", str, x->tv_nsec);
}

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32
