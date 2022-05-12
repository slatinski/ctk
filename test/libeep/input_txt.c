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

#include "input_txt.h"
#include "api_c.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h> // iscntrl

struct input_txt make_input_txt() {
    struct input_txt x = { .f = fopen("input.txt", "r"), .more = 1, .line = { 0 } };

    if (!x.f) {
        x.more = 0;
    }
    return x;
}

void dispose(struct input_txt* x) {
    assert(x);
    if (x->f) {
        fclose(x->f);
    }
    x->f = NULL;
    x->line[0] = 0;
    x->more = 0;
}

const char* next(struct input_txt* x) {
    assert(x);

    if (!x->f || !x->more) {
        return x->line;
    }

    const char* line = fgets(x->line, sizeof(x->line), x->f);
    if (!line) {
        x->more = 0;
        x->line[0] = 0;
        return x->line;
    }

    size_t len = strlen(line);
    for (size_t i = 0; i < len; ++i) {
        if (iscntrl(line[i])) {
            x->line[i] = 0;
        }
    }

    int all_empty = 1;
    for (size_t i = 0; i < len; ++i) {
        all_empty &= (isspace(line[i]) || !line[i]);
    }

    if (all_empty) {
        x->more = 0;
        x->line[0] = 0;
    }

    return x->line;
}

int reset(struct input_txt* x) {
    if (!x || !x->f) {
        return EXIT_FAILURE;
    }

    rewind(x->f);
    x->more = 1;
    x->line[0] = 0;
    return EXIT_SUCCESS;
}


const char* last_n(const char* x, size_t n) {
    const size_t size = strlen(x);
    if (size <= n) {
        return x;
    }

    return x + (size - n);
}



void stderr_compare_1file_2readers_begin(const char* fname, const char* lib_x, const char* lib_y) {
    fprintf(stderr, "%40s %s/%s comparison", last_n(fname, 40), lib_x, lib_y);
}

void stderr_compare_1file_2readers_failed(const char* lib) {
    fprintf(stderr, " !FAIL %s reading\n", lib);
}

void stderr_copy_begin(const char* fname, const char* delme_cnt, const char* lib_reader, const char* lib_writer) {
    fprintf(stderr, "%40s %s read, %s write (-> %9s)", last_n(fname, 40), lib_reader, lib_writer, last_n(delme_cnt, 9));
}

void stderr_read_speed_begin(const char* fname, long chunk) {
    fprintf(stderr, "%40s performance read,  chunk size %4ld ", last_n(fname, 40), chunk);
}

void stderr_write_speed_begin(const char* fname, long chunk) {
    fprintf(stderr, "%40s performance write, chunk size %4ld ", last_n(fname, 40), chunk);
}

void stderr_speed_end_incomparable() {
    fprintf(stderr, "not comparable\n");
}

void stderr_speed_end(const char* lib_unit, double unit, const char* lib_compared, double compared) {
    char percent[64];
    const double ratio = (compared / unit) * 100.0;
    snprintf(percent, sizeof(percent), "%.2f%%", ratio);
    fprintf(stderr, "(%s %5.2fs, %s %5.2fs): %5s\n", lib_unit, unit, lib_compared, compared, percent);
}


void stderr_compare_begin(const char* fname_x, const char* fname_y, const char* lib_reader) {
    char msg[512];
    if (strlen(fname_x) < 18) {
        snprintf(msg, sizeof(msg), "%18s <-> %9s", last_n(fname_x, 18), last_n(fname_y, 9));
    }
    else {
        snprintf(msg, sizeof(msg), "...%18s <-> %9s", last_n(fname_x, 18), last_n(fname_y, 9));
    }
    fprintf(stderr, "%40s %s comparison    ", msg, lib_reader);
}

void stderr_failed_reader(const char* fname) {
    if (strlen(fname) < 18) {
        fprintf(stderr, " !FAIL reading %s\n", fname);
    }
    else {
        fprintf(stderr, " !FAIL reading ...%18s\n", last_n(fname, 18));
    }
}

void stderr_failed_writer(const char* fname) {
    if (strlen(fname) < 18) {
        fprintf(stderr, " !FAIL writing %s\n", fname);
    }
    else {
        fprintf(stderr, " !FAIL writing ...%18s\n", last_n(fname, 18));
    }
}


void stderr_print_success(enum summary result) {
    if (result != ok) {
        fprintf(stderr, " [FAILED]");
    }

    if(result & header_epoch) {
        fprintf(stderr, " epochl");
    }
    if(result & header_srate) {
        fprintf(stderr, " rate");
    }
    if(result & header_stamp) {
        fprintf(stderr, " stamp");
    }
    if(result & header_smpl) {
        fprintf(stderr, " smplc");
    }
    if(result & header_elc) {
        fprintf(stderr, " elc");
    }
    if(result & info) {
        fprintf(stderr, " info");
    }
    if(result & trg) {
        fprintf(stderr, " trg");
    }
    if(result & eeg_data) {
        fprintf(stderr, " eeg");
    }
    if(result & aux) {
        fprintf(stderr, " aux");
    }

    if (result == ok) {
        fprintf(stderr, " ok\n");
    }
    else {
        fprintf(stderr, "\n");
    }
}

void stderr_intermezzo(const char* text) {
    fprintf(stderr, "\n---== %s ==---\n\n", text);
}


