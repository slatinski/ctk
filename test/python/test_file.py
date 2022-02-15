# Copyright 2015-2021 Velko Hristov
# This file is part of CntToolKit.
# SPDX-License-Identifier: LGPL-3.0+
# 
# CntToolKit is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# CntToolKit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
#You should have received a copy of the GNU Lesser General Public License
#along with CntToolKit.  If not, see <http://www.gnu.org/licenses/>.

import ctkpy as lib
import numpy as np
from datetime import datetime

def write(cnt):
    stamp = datetime.utcnow()
    writer = lib.writer_reflib(cnt)

    writer.param.sampling_frequency = 1024
    writer.param.electrodes = lib.electrodes([("1", "ref"), ("2", "ref"), ("3", "ref"), ("4", "ref")])
    writer.param.start_time = stamp 
    writer.param.epoch_length = 2048
    writer.info.hospital = "Institution"
    writer.info.test_name = "routine eeg"
    writer.info.test_serial = "trial 001"
    writer.info.physician = "Doctor A"
    writer.info.technician = "Operator B"
    writer.info.machine_make = "eego"
    writer.info.machine_model = "ee-411"
    writer.info.machine_sn = "0000"
    writer.info.subject_name = "Person C"
    writer.info.subject_sex = lib.sex.male;
    writer.info.subject_handedness = lib.handedness.left
    writer.info.subject_phone = "000-0000-0000"
    writer.info.subject_address = "somewhere"
    writer.info.subject_dob = stamp
    writer.info.comment = "history/medications"

    xs = np.array([[11, 21, 31, 41], [12, 22, 32, 42]])
    writer.column_major(xs)

    ys = np.array([[13, 14], [23, 24], [33, 34], [43, 44]])
    writer.row_major(ys)

    ys = np.array([[15, 16], [25, 26], [35, 36], [45, 46]])
    writer.row_major(ys)

    writer.triggers([(0, "Rare"), (3, "Frequent")])
    writer.trigger((5, "End"))

    writer.impedance(lib.event_impedance(stamp, [128000, 41000, 73000, 99000]))
    duration = 0.13
    trigger_code = 128
    writer.video(lib.event_video(stamp, duration, trigger_code))
    offset = -2.02
    writer.epoch(lib.event_epoch(stamp, duration, offset, trigger_code))
    # TODO: plural forms
    # TODO: additional fields

    writer.close()
    return stamp

def read(cnt, stamp):
    reader = lib.reader_reflib(cnt)
    assert reader.sample_count == 6

    assert reader.param.start_time == stamp
    assert reader.param.sampling_frequency == 1024
    assert reader.param.epoch_length == 2048
    assert len(reader.param.electrodes) == 4
    assert reader.param.electrodes[0] == lib.electrode("1", "ref", "uV", 1, 1/256)
    assert reader.param.electrodes[1] == lib.electrode("2", "ref", "uV", 1, 1/256)
    assert reader.param.electrodes[2] == lib.electrode("3", "ref", "uV", 1, 1/256)
    assert reader.param.electrodes[3] == lib.electrode("4", "ref", "uV", 1, 1/256)
    assert reader.info.hospital == "Institution"
    assert reader.info.test_name == "routine eeg"
    assert reader.info.test_serial == "trial 001"
    assert reader.info.physician == "Doctor A"
    assert reader.info.technician == "Operator B"
    assert reader.info.machine_make == "eego"
    assert reader.info.machine_model == "ee-411"
    assert reader.info.machine_sn == "0000"
    assert reader.info.subject_name == "Person C"
    assert reader.info.subject_sex == lib.sex.male;
    assert reader.info.subject_handedness == lib.handedness.left
    assert reader.info.subject_phone == "000-0000-0000"
    assert reader.info.subject_address == "somewhere"
    assert reader.info.comment == "history/medications"
    assert len(reader.triggers) == 3
    assert reader.triggers[0].sample == 0
    assert reader.triggers[0].code == "Rare"
    assert reader.triggers[1].sample == 3
    assert reader.triggers[1].code == "Frequent"
    assert reader.triggers[2].sample == 5
    assert reader.triggers[2].code == "End"
    assert len(reader.impedances) == 1
    assert reader.impedances[0].stamp == stamp
    assert reader.impedances[0].values == [128000, 41000, 73000, 99000]
    assert len(reader.videos) == 1
    assert reader.videos[0].stamp == stamp
    assert reader.videos[0].duration == 0.13
    assert reader.videos[0].trigger_code == 128
    assert len(reader.epochs) == 1
    assert reader.epochs[0].stamp == stamp
    assert reader.epochs[0].duration == 0.13
    assert reader.epochs[0].offset == -2.02
    assert reader.epochs[0].trigger_code == 128

    xs = reader.column_major(0, reader.sample_count)
    assert xs.shape[0] == 6
    assert xs.shape[1] == 4
    assert xs[0][0] == 11
    assert xs[0][1] == 21
    assert xs[0][2] == 31
    assert xs[0][3] == 41
    assert xs[1][0] == 12
    assert xs[1][1] == 22
    assert xs[1][2] == 32
    assert xs[1][3] == 42
    assert xs[2][0] == 13
    assert xs[2][1] == 23
    assert xs[2][2] == 33
    assert xs[2][3] == 43
    assert xs[3][0] == 14
    assert xs[3][1] == 24
    assert xs[3][2] == 34
    assert xs[3][3] == 44
    assert xs[4][0] == 15
    assert xs[4][1] == 25
    assert xs[4][2] == 35
    assert xs[4][3] == 45
    assert xs[5][0] == 16
    assert xs[5][1] == 26
    assert xs[5][2] == 36
    assert xs[5][3] == 46

    ys = reader.row_major(0, reader.sample_count)
    assert ys.shape[0] == 4
    assert ys.shape[1] == 6
    assert ys[0][0] == 11
    assert ys[0][1] == 12
    assert ys[0][2] == 13
    assert ys[0][3] == 14
    assert ys[0][4] == 15
    assert ys[0][5] == 16
    assert ys[1][0] == 21
    assert ys[1][1] == 22
    assert ys[1][2] == 23
    assert ys[1][3] == 24
    assert ys[1][4] == 25
    assert ys[1][5] == 26
    assert ys[2][0] == 31
    assert ys[2][1] == 32
    assert ys[2][2] == 33
    assert ys[2][3] == 34
    assert ys[2][4] == 35
    assert ys[2][5] == 36
    assert ys[3][0] == 41
    assert ys[3][1] == 42
    assert ys[3][2] == 43
    assert ys[3][3] == 44
    assert ys[3][4] == 45
    assert ys[3][5] == 46

    assert reader.epoch_count == 1
    x1s = reader.epoch_column_major(0)
    y1s = reader.epoch_row_major(0)
    assert (x1s == xs).all()
    assert (y1s == ys).all()

    bs = reader.epoch_compressed(0)
    decomp = lib.decompress_reflib()
    decomp.sensors = 4
    samples = 6
    x2s = decomp.column_major(bs, samples) * (1/256)
    y2s = decomp.row_major(bs, samples) * (1/256)
    assert (x2s == xs).all()
    assert (y2s == ys).all()

    comp = lib.compress_reflib()
    comp.sensors = 4
    b1s = comp.column_major(xs * 256)
    b2s = comp.row_major(ys * 256)
    assert b1s == bs
    assert b2s == bs


def test_write_read_cnt_evt(tmp_path):
    temporary = tmp_path / "cnt_evt.cnt"
    stamp = write(str(temporary))
    read(str(temporary), stamp)


def write_evt(evt):
    stamp = datetime.utcnow()
    writer = lib.event_writer(evt)

    writer.impedance(lib.event_impedance(stamp, [128000, 41000, 73000, 99000]))
    duration = 0.13
    trigger_code = 128
    writer.video(lib.event_video(stamp, duration, trigger_code))
    offset = -2.02
    writer.epoch(lib.event_epoch(stamp, duration, offset, trigger_code))
    # TODO: plural forms
    # TODO: additional fields

    writer.close()
    return stamp

def read_evt(evt, stamp):
    reader = lib.event_reader(evt)

    xs = reader.impedances
    assert xs[0] == lib.event_impedance(stamp, [128000, 41000, 73000, 99000])

    ys = reader.videos
    assert ys[0] == lib.event_video(stamp, 0.13, 128)

    zs = reader.epochs
    assert zs[0] == lib.event_epoch(stamp, 0.13, -2.02, 128)


def test_write_read_evt(tmp_path):
    temporary = tmp_path / "evt.evt"
    stamp = write_evt(str(temporary))
    read_evt(str(temporary), stamp)

