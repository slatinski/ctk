import ctkpy as lib
import numpy as np
from datetime import datetime

def write(cnt):
    writer = lib.writer_reflib(cnt)

    # setup: mandatory settings
    writer.param.sampling_frequency = 2048
    writer.param.start_time = datetime.utcnow()
    # option 1) all electrodes at once. default unit uV, iscale 1, rscale 1/256 (3.9nV LSB, 16.75V p2p for 32-bit signed integrals)
    writer.param.electrodes = lib.electrodes([("1", "ref"), ("2", "ref")])
    # option 2) one electrode at a time
    writer.add_electrode(("3", "ref"))
    writer.add_electrode(lib.electrode("4", "ref", "uV", 1, 1/256))
    # note: for compatibility reasons do not use the unit "V". on the other hand "uV", "nV", etc are fine.

    # setup: optional settings
    writer.param.epoch_length = 1024
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
    writer.info.subject_dob = datetime.utcnow()
    writer.info.comment = "history/medications"

    # data input
    # scaling is applied to the input as follows:
    # f(x) = x / (electrode.iscale * electrode.rscale)
    # the result is rounded to the nearest integer, converted to signed 32-bit integral, compressed and stored.
    # note: changes to fields in writer.param (the mandatory setup settings) will either fail
    # or have no effect after the execution of any of the following data insertion operations

    # option 1) the input matrix is in column major format (fortran style)
    xs = np.array([[11, 21, 31, 41], [12, 22, 32, 42]])
    print("input [column major]: samples", xs.shape[0], "| electrodes", xs.shape[1])
    print(xs)
    print("")
    writer.column_major(xs)

    # option 2) the input matrix is in row major format (c style)
    ys = np.array([[13, 14], [23, 24], [33, 34], [43, 44]])
    print("input [row major]: samples", ys.shape[1], "| electrodes", ys.shape[0])
    print(ys)
    print("")
    writer.row_major(ys)
    print("two more column major samples\n")
    writer.column_major(xs)

    # option 1) appending multiple triggers at once
    writer.triggers([(0, "128"), (3, "129")]) # sample index, trigger code
    # option 2) appending one trigger at a time
    writer.trigger((5, "128"))

    writer.impedance(lib.event_impedance(datetime.utcnow(), [128000, 41000, 73000, 99000])) # ohm, scaled and stored as kohm
    duration = 0.13 # seconds
    trigger_code = 128
    writer.video(lib.event_video(datetime.utcnow(), duration, trigger_code))
    offset = -2.02 # seconds
    writer.epoch(lib.event_epoch(datetime.utcnow(), duration, offset, trigger_code))
    # plural forms also available for the above, eg writer.epochs.
    # additional fields are available, eg event_epoch_object.condition_label.
    # missing: event construction from tuples


    # IMPORTANT: the cnt/evt files are constructed at this stage.
    # the invocation of close() should NOT be omited.
    writer.close()


def read(cnt):
    reader = lib.reader_reflib(cnt)

    # fetches all samples starting at position 0
    xs = reader.column_major(0, reader.sample_count)
    print("output [column major]: samples", xs.shape[0], "| electrodes", xs.shape[1])
    print(xs)
    print("")

    # fetches all samples starting at position 0
    ys = reader.row_major(0, reader.sample_count)
    print("output [row major]: samples", ys.shape[1], "| electrodes", ys.shape[0])
    print(ys)
    print("")

    print("electrodes", reader.param.electrodes)
    print("start time", reader.param.start_time, "UTC")
    print("impedance ", reader.impedances[0].stamp, reader.impedances[0].values)
    print("triggers  ", reader.triggers)
    print("videos    ", reader.videos)
    print("epochs    ", reader.epochs)


temporary = "example.cnt"
write(temporary)
read(temporary)

