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

import pytest
import pickle
import ctkpy as lib
from copy import copy
import numpy as np
from datetime import datetime
from datetime import timedelta


def test_version():
    x = lib.file_version(1, 2)
    assert x.major == 1
    assert x.minor == 2

    y = lib.file_version(1, 2)
    assert y == x

    y.major = 2
    assert y != x

    y = copy(x)
    assert y == x

    y.minor = 3
    assert y != x

    with pytest.raises(TypeError):
        y.major = -3
        y.minor = -3
        y.major = 1.2
        y.minor = 1.2
        z = lib.file_version(0.12, 2)
        z = lib.file_version(2, 0.12)
        z = lib.file_version(-2, 2)
        z = lib.file_version(2, -2)


def test_trigger():
    x = lib.trigger(1, "rare")
    assert x.sample == 1
    assert x.code == "rare"

    y = lib.trigger(1, "rare")
    assert y == x

    y.sample = 2
    assert y != x

    y = copy(x)
    assert y == x

    y.code = "frequent"
    assert y != x

    y = copy(x)
    with pytest.raises(RuntimeError):
        y.code = "way too long"
    assert y == x


def test_electrode():
    x = lib.electrode("fp1", "ref", "uV", 1, 1/256)
    assert x.label == "fp1"
    assert x.reference == "ref"
    assert x.unit == "uV"
    assert x.iscale == 1 
    assert x.rscale == 1/256 

    ys = lib.electrodes([("fp1", "ref"), ("fp1", "ref")])
    assert ys[0] == x
    assert ys[1] == x

    zs = lib.electrodes([("fp1", "ref", "uV"), ("fp1", "ref", "uV")])
    assert zs[0] == x
    assert zs[1] == x

    zs[0].label = "fp2"
    assert zs[0] != x

    zs[0] = copy(x)
    assert zs[0] == x
    zs[0].reference = "other"
    assert zs[0] != x

    zs[0] = copy(x)
    zs[0].unit = "other"
    assert zs[0] != x

    zs[0] = copy(x)
    zs[0].iscale = 3
    assert zs[0] != x

    zs[0] = copy(x)
    zs[0].rscale = 3
    assert zs[0] != x


def test_time_series():
    stamp = datetime.utcnow()
    elc = lib.electrodes([("fp1", "ref"), ("fp2", "ref")])

    x = lib.time_series(stamp, 2048, elc, 4096)
    assert x.start_time == stamp 
    assert x.sampling_frequency == 2048
    assert x.electrodes == copy(elc)
    assert x.epoch_length == 4096

    y = copy(x)
    assert y == x
    y.start_time = stamp + timedelta(seconds = 1)
    assert y != x
    assert y.start_time != x.start_time 

    y = copy(x)
    y.sampling_frequency = 1024
    assert y != x
    assert y.sampling_frequency != x.sampling_frequency 

    y = copy(x)
    y.electrodes[0].rscale = 1
    assert y != x
    assert y.electrodes != x.electrodes
    assert y.electrodes[0] != x.electrodes[0]

    y = copy(x)
    y.epoch_length = 0
    assert y != x
    assert y.epoch_length != x.epoch_length


def test_info():
    x = lib.information()
    assert len(x.hospital) == 0
    assert len(x.test_name) == 0
    assert len(x.test_serial) == 0
    assert len(x.physician) == 0
    assert len(x.technician) == 0
    assert len(x.machine_make) == 0
    assert len(x.machine_model) == 0
    assert len(x.machine_sn) == 0
    assert len(x.subject_name) == 0
    assert len(x.subject_id) == 0
    assert len(x.subject_address) == 0
    assert len(x.subject_phone) == 0
    assert x.subject_sex == lib.sex.unknown
    assert x.subject_handedness == lib.handedness.unknown
    # tz offset
    # assert x.subject_dob == datetime(year = 1970, month = 1, day = 1)
    assert len(x.comment) == 0

    y = copy(x)
    assert y == x
    y.hospital = "a"
    assert y != x
    assert y.hospital != x.hospital

    y = copy(x)
    y.test_name = "a"
    assert y != x
    assert y.test_name != x.test_name

    y = copy(x)
    y.test_serial = "a"
    assert y != x
    assert y.test_serial != x.test_serial

    y = copy(x)
    y.physician = "a"
    assert y != x
    assert y.physician != x.physician

    y = copy(x)
    y.technician = "a"
    assert y != x
    assert y.technician != x.technician

    y = copy(x)
    y.machine_make = "a"
    assert y != x
    assert y.machine_make != x.machine_make

    y = copy(x)
    y.machine_model = "a"
    assert y != x
    assert y.machine_model != x.machine_model

    y = copy(x)
    y.machine_sn = "a"
    assert y != x
    assert y.machine_sn != x.machine_sn

    y = copy(x)
    y.subject_name = "a"
    assert y != x
    assert y.subject_name != x.subject_name

    y = copy(x)
    y.subject_id = "a"
    assert y != x
    assert y.subject_id != x.subject_id

    y = copy(x)
    y.subject_address = "a"
    assert y != x
    assert y.subject_address != x.subject_address

    y = copy(x)
    y.subject_phone = "a"
    assert y != x
    assert y.subject_phone != x.subject_phone

    y = copy(x)
    y.subject_sex = lib.sex.female
    assert y != x
    assert y.subject_sex != x.subject_sex

    y = copy(x)
    y.subject_handedness = lib.handedness.right
    assert y != x
    assert y.subject_handedness != x.subject_handedness

    y = copy(x)
    y.subject_dob = datetime.utcnow()
    assert y != x
    assert y.subject_dob != x.subject_dob

    y = copy(x)
    y.comment = "a"
    assert y != x
    assert y.comment != x.comment


def test_event_impedance():
    stamp = datetime.utcnow()
    x = lib.event_impedance(stamp, [1, 2, 3])
    assert x.stamp == stamp 
    assert x.values == [1, 2, 3] 

    y = copy(x)
    assert y == x
    y.stamp = stamp + timedelta(seconds = 1)
    assert y != x
    assert y.stamp != x.stamp
    assert y.values == x.values

    y = copy(x)
    y.values = [2, 3, 4]
    assert y != x
    assert y.stamp == x.stamp
    assert y.values != x.values

def test_event_video():
    stamp = datetime.utcnow()
    x = lib.event_video(stamp, 2.3, 1)
    assert x.stamp == stamp 
    assert x.duration == 2.3
    assert x.trigger_code == 1
    assert len(x.condition_label) == 0
    assert len(x.description) == 0
    assert len(x.video_file) == 0

    y = copy(x)
    assert y == x
    y.stamp = stamp + timedelta(seconds = 1)
    assert y != x
    assert y.stamp != x.stamp

    y = copy(x)
    y.duration = 1
    assert y != x
    assert y.duration != x.duration

    y = copy(x)
    y.trigger_code = 2
    assert y != x
    assert y.trigger_code != x.trigger_code

    y = copy(x)
    y.condition_label = "a"
    assert y != x
    assert y.condition_label != x.condition_label

    y = copy(x)
    y.description = "a"
    assert y != x
    assert y.description != x.description

    y = copy(x)
    y.video_file = "a"
    assert y != x
    assert y.video_file != x.video_file

def test_event_epoch():
    stamp = datetime.utcnow()
    x = lib.event_epoch(stamp, 1.2, 2.3, 4)
    assert x.stamp == stamp 
    assert x.duration == 1.2
    assert x.offset == 2.3
    assert x.trigger_code == 4
    assert len(x.condition_label) == 0

    y = copy(x)
    assert y == x
    y.stamp = stamp + timedelta(seconds = 1)
    assert y != x
    assert y.stamp != x.stamp

    y = copy(x)
    y.duration = 1
    assert y != x
    assert y.duration != x.duration

    y = copy(x)
    y.offset = 1
    assert y != x
    assert y.offset != x.offset

    y = copy(x)
    y.trigger_code = 2
    assert y != x
    assert y.trigger_code != x.trigger_code

    y = copy(x)
    y.condition_label = "a"
    assert y != x
    assert y.condition_label != x.condition_label

