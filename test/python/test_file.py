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


input_stamp = datetime.utcnow()
input_rate = 1024.0
input_electrodes_v4 = [("1", "ref", "uV"), ("2", "ref", "uV"), ("3", "ref", "uV"), ("4", "ref", "uV")]
input_electrodes = lib.electrodes([("1", "ref"), ("2", "ref"), ("3", "ref"), ("4", "ref")])
input_epoch = 6
input_hospital = "Institution"
input_tname = "routine eeg"
input_tserial = "trial 001"
input_physician = "Doctor A"
input_technician = "Operator B"
input_make = "eego"
input_model = "ee-411"
input_sn = "0000"
input_name = "Person C"
input_sex = lib.sex.male;
input_hand = lib.handedness.left
input_phone = "000-0000-0000"
input_addr = "somewhere"
input_comment = "history/medications"
input_triggers = [lib.trigger(0, "Rare"), lib.trigger(3, "Frequent"), lib.trigger(5, "End")]
input_impedances = [lib.event_impedance(input_stamp, [128000, 41000, 73000, 99000])]
input_videos = [lib.event_video(input_stamp, 0.13, 128)]
input_epochs = [lib.event_epoch(input_stamp, 0.13, -2.02, 128)]
input_col_major = np.array([[11, 21, 31, 41], [12, 22, 32, 42]])
input_row_major_1 = np.array([[13, 14], [23, 24], [33, 34], [43, 44]])
input_row_major_2 = np.array([[15, 16], [25, 26], [35, 36], [45, 46]])
result_column_major = np.array([[11, 21, 31, 41]
                              , [12, 22, 32, 42]
                              , [13, 23, 33, 43]
                              , [14, 24, 34, 44]
                              , [15, 25, 35, 45]
                              , [16, 26, 36, 46]])
result_row_major = np.array([[11, 12, 13, 14, 15, 16]
                           , [21, 22, 23, 24, 25, 26]
                           , [31, 32, 33, 34, 35, 36]
                           , [41, 42, 43, 44, 45, 46]])


def write(cnt):
    writer = lib.writer_reflib(cnt)

    writer.param.sampling_frequency = input_rate
    writer.param.electrodes = input_electrodes
    writer.param.start_time = input_stamp 
    writer.param.epoch_length = input_epoch
    writer.info.hospital = input_hospital
    writer.info.test_name = input_tname
    writer.info.test_serial = input_tserial
    writer.info.physician = input_physician
    writer.info.technician = input_technician
    writer.info.machine_make = input_make
    writer.info.machine_model = input_model
    writer.info.machine_sn = input_sn
    writer.info.subject_name = input_name
    writer.info.subject_sex = input_sex
    writer.info.subject_handedness = input_hand
    writer.info.subject_phone = input_phone
    writer.info.subject_address = input_addr
    writer.info.subject_dob = input_stamp
    writer.info.comment = input_comment 

    writer.column_major(input_col_major)
    writer.row_major(input_row_major_1)
    writer.row_major(input_row_major_2)

    writer.triggers(input_triggers[:2])
    writer.trigger(input_triggers[2])

    writer.impedance(input_impedances[0])
    writer.video(input_videos[0])
    writer.epoch(input_epochs[0])
    # TODO: plural forms
    # TODO: additional fields

    writer.close()

def read(cnt):
    reader = lib.reader_reflib(cnt)
    assert reader.sample_count == result_row_major.shape[1]
    assert reader.param.start_time == input_stamp
    assert reader.param.sampling_frequency == input_rate
    assert reader.param.epoch_length == input_epoch
    assert reader.param.electrodes == input_electrodes
    assert reader.info.hospital == input_hospital
    assert reader.info.test_name == input_tname
    assert reader.info.test_serial == input_tserial
    assert reader.info.physician == input_physician
    assert reader.info.technician == input_technician
    assert reader.info.machine_make == input_make
    assert reader.info.machine_model == input_model
    assert reader.info.machine_sn == input_sn
    assert reader.info.subject_name == input_name
    assert reader.info.subject_sex == input_sex
    assert reader.info.subject_handedness == input_hand
    assert reader.info.subject_phone == input_phone
    assert reader.info.subject_address == input_addr
    assert reader.info.comment == input_comment 
    assert reader.triggers == input_triggers
    assert reader.impedances == input_impedances
    assert reader.videos == input_videos
    assert reader.epochs == input_epochs

    assert (reader.column_major(0, reader.sample_count) == result_column_major).all()
    assert (reader.row_major(0, reader.sample_count) == result_row_major).all()

    assert reader.epoch_count == 1
    assert (reader.epoch_column_major(0) == result_column_major).all()
    assert (reader.epoch_row_major(0) == result_row_major).all()

    bs = reader.epoch_compressed(0)
    decomp = lib.decompress_reflib()
    decomp.sensors = result_row_major.shape[0]
    samples = result_row_major.shape[1]

    assert (decomp.column_major(bs, samples) * (1/256) == result_column_major).all()
    assert (decomp.row_major(bs, samples) * (1/256) == result_row_major).all()

    comp = lib.compress_reflib()
    comp.sensors = result_row_major.shape[0]
    assert comp.column_major(result_column_major * 256) == bs
    assert comp.row_major(result_row_major * 256) == bs


def write_v4(cnt):
    riff64 = 0
    writer = lib.write_cnt(cnt, input_rate, input_electrodes_v4, riff64)
    for sample in result_column_major:
        writer.add_samples(sample)
    writer.close()


def read_v4(cnt):
    reader = lib.read_cnt(cnt)
    assert reader.get_channel_count() == result_row_major.shape[0]
    for i in range(reader.get_channel_count()):
        assert reader.get_channel(i) == input_electrodes_v4[i]

    assert reader.get_sample_frequency() == input_rate
    assert reader.get_sample_count() == result_row_major.shape[1]
    assert reader.get_trigger_count() == 0

    i = 0
    for sample in result_column_major:
        result = reader.get_samples(i, 1)
        assert (result == sample).all()
        i += 1


def test_write_read_cnt_evt(tmp_path):
    tmp_cnt = tmp_path / "reflib.cnt"
    write(str(tmp_cnt))
    read(str(tmp_cnt))

    tmp_v4 = tmp_path / "v4.cnt"
    write_v4(str(tmp_v4))
    read_v4(str(tmp_v4))


def write_evt(evt):
    writer = lib.event_writer(evt)

    writer.impedances(input_impedances)
    writer.videos(input_videos)
    writer.epochs(input_epochs)
    # TODO: singular forms
    # TODO: additional fields

    writer.close()

def read_evt(evt):
    reader = lib.event_reader(evt)
    assert reader.impedances() == input_impedances
    assert reader.videos() == input_videos
    assert reader.epochs() == input_epochs


def test_write_read_evt(tmp_path):
    tmp_evt = tmp_path / "evt.evt"
    write_evt(str(tmp_evt))
    read_evt(str(tmp_evt))

