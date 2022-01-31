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

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../catch.hpp"

#include <vector>
#include <numeric>
#include <algorithm>
#include <memory>
#include <iostream>
#include <filesystem>
#include <chrono>

#include "test/util.h"
#include "ctk/api_data.h"
#include "ctk/container/file_reflib.h"

namespace libeep {
extern "C" {
#include "libcnt/cnt.c"
}
}

namespace ctk { namespace impl { namespace test {

using namespace ctk::api::v1;

struct close_cnt
{
    void operator()(libeep::eeg_t* cnt) const { if (!cnt) return; libeep::eep_free(cnt); }
};
using cnt_ptr = std::unique_ptr<libeep::eeg_t, close_cnt>;

struct close_trg
{
    void operator()(libeep::trg_t* trg) const { if (!trg) return; libeep::trg_free(trg); }
};
using trg_ptr = std::unique_ptr<libeep::trg_t, close_trg>;

struct mem_free
{
    void operator()(void* p) const { free(p); }
};


auto info2recordinfo(const Info& x) -> libeep::record_info_t {
    libeep::record_info_t result;
    memset(&result, 0, sizeof(libeep::record_info_t));

    std::strncpy(result.m_szHospital, x.hospital.c_str(), sizeof(result.m_szHospital) - 1);
    std::strncpy(result.m_szTestName , x.test_name.c_str(), sizeof(result.m_szTestName) - 1);
    std::strncpy(result.m_szTestSerial, x.test_serial.c_str(), sizeof(result.m_szTestSerial) - 1);
    std::strncpy(result.m_szPhysician, x.physician.c_str(), sizeof(result.m_szPhysician) - 1);
    std::strncpy(result.m_szTechnician, x.technician.c_str(), sizeof(result.m_szTechnician) - 1);
    std::strncpy(result.m_szMachineMake, x.machine_make.c_str(), sizeof(result.m_szMachineMake) - 1);
    std::strncpy(result.m_szMachineModel, x.machine_model.c_str(), sizeof(result.m_szMachineModel) - 1);
    std::strncpy(result.m_szMachineSN, x.machine_sn.c_str(), sizeof(result.m_szMachineSN) - 1);
    std::strncpy(result.m_szName, x.subject_name.c_str(), sizeof(result.m_szName) - 1);
    std::strncpy(result.m_szID, x.subject_id.c_str(), sizeof(result.m_szID) - 1);
    std::strncpy(result.m_szAddress, x.subject_address.c_str(), sizeof(result.m_szAddress) - 1);
    std::strncpy(result.m_szPhone, x.subject_phone.c_str(), sizeof(result.m_szPhone) - 1);
    result.m_chSex = sex2char(x.subject_sex);
    result.m_chHandedness = handedness2char(x.subject_handedness);
    result.m_DOB = x.subject_dob;
    std::strncpy(result.m_szComment, x.comment.c_str(), sizeof(result.m_szComment) - 1);

    return result;
}

auto recordinfo2info(const libeep::record_info_t& x) -> Info {
    Info result;

    result.hospital = x.m_szHospital;
    result.test_name = x.m_szTestName;
    result.test_serial = x.m_szTestSerial;
    result.physician = x.m_szPhysician;
    result.technician = x.m_szTechnician;
    result.machine_make = x.m_szMachineMake;
    result.machine_model = x.m_szMachineModel;
    result.machine_sn = x.m_szMachineSN;
    result.subject_name = x.m_szName;
    result.subject_id = x.m_szID;
    result.subject_address = x.m_szAddress;
    result.subject_phone = x.m_szPhone;
    result.subject_sex = char2sex(x.m_chSex);
    result.subject_handedness = char2handedness(x.m_chHandedness);
    result.subject_dob = x.m_DOB;
    result.comment = x.m_szComment;

    return result;
}


class cnt_reader_libeep_riff
{
    file_ptr f;
    cnt_ptr cnt;

public:

    explicit cnt_reader_libeep_riff(const std::string& fname)
    : f{ open_r(fname) } {
        int status{ CNTERR_NONE };
        cnt.reset(libeep::eep_init_from_file(fname.c_str(), f.get(), &status));
        if (!cnt || status != CNTERR_NONE) {
            throw ctk_data{ "cnt_reader_libeep_riff: can not initialize" };
        }
        assert(cnt);
    }

    auto get(measurement_count start, measurement_count length, column_major2row_major) -> std::vector<int32_t> {
        if (start < 0 || length < 0) {
            throw ctk_bug{ "cnt_reader_libeep_riff::get: invalid input" };
        }
        std::vector<int32_t> result(as_sizet_unchecked(matrix_size(channel_count(), length)));

        const sint si{ start };
        const uint64_t i{ cast(si, uint64_t{}, guarded{}) };
        const int absolute{ 0 };
        if (libeep::eep_seek(cnt.get(), libeep::DATATYPE_EEG, i, absolute) != CNTERR_NONE) {
            throw ctk_data{ "cnt_reader_libeep_riff::get: can not seek" };
        }

        const sint sn{ length };
        const uint64_t n{ cast(sn, uint64_t{}, guarded{}) };
        if (libeep::eep_read_sraw(cnt.get(), libeep::DATATYPE_EEG, result.data(), n) != CNTERR_NONE) {
            throw ctk_data{ "cnt_reader_libeep_riff::get: can not read" };
        }

        return result;
    }

    auto range_column_major(measurement_count start, measurement_count length) -> std::vector<int32_t> {
        constexpr const column_major2row_major transpose;
        return get(start, length, transpose);
    }

    auto channel_count() const -> sensor_count {
        return sensor_count{ eep_get_chanc(cnt.get()) };
    }

    auto channel_order() const -> std::vector<int16_t> {
        using free_ptr = std::unique_ptr<short, mem_free>;

        free_ptr seq{ eep_get_chanseq(cnt.get(), libeep::DATATYPE_EEG) };
        if (!seq) {
            throw ctk_data{ "cnt_reader_libeep_riff::channel_order: can not obtain channel order" };
        }

        const auto size{ static_cast<size_t>(static_cast<sint>(channel_count())) };
        std::vector<int16_t> result(size);
        if (std::copy(seq.get(), seq.get() + size, begin(result)) != end(result)) {
            throw ctk_bug{ "cnt_reader_libeep_riff::channel_order: can not copy" };
        }

        return result;
    }

    auto sample_count() const -> measurement_count {
        const uint64_t samplec{ libeep::eep_get_samplec(cnt.get()) };
        return measurement_count{ cast(samplec, sint{}, ok{}) };
    }

    auto epoch_length() const -> measurement_count {
        const int epochl{ libeep::eep_get_epochl(cnt.get(), libeep::DATATYPE_EEG) };
        return measurement_count{ cast(epochl, sint{}, ok{}) };
    }

    auto sampling_frequency() const -> double {
        return 1.0 / libeep::eep_get_period(cnt.get());
    }

    auto channels() const -> std::vector<Electrode> {
        std::vector<Electrode> result;

        const short count{ libeep::eep_get_chanc(cnt.get()) };
        for (short i{ 0 }; i < count; ++i) {
            Electrode e;

            e.label = libeep::eep_get_chan_label(cnt.get(), i);
            e.reference = libeep::eep_get_chan_reflab(cnt.get(), i);
            e.unit = libeep::eep_get_chan_unit(cnt.get(), i);
            e.status = libeep::eep_get_chan_status(cnt.get(), i);
            e.type = libeep::eep_get_chan_type(cnt.get(), i);
            e.iscale = libeep::eep_get_chan_iscale(cnt.get(), i);
            e.rscale = libeep::eep_get_chan_rscale(cnt.get(), i);

            result.push_back(e);
        }

        return result;
    }

    auto triggers() const -> std::vector<Trigger> {
        const libeep::trg_t* t{ eep_get_trg(cnt.get()) };

        if (!t) {
            return {};
        }
        const auto count{ t->c };

        std::vector<Trigger> result;
        result.reserve(count);

        for (uint64_t i{ 0 }; i < count; ++i) {
            result.emplace_back(cast(t->v[i].sample, int64_t{}, ok{}), from_trgcode(t->v[i].code)); // ctk_data
        }

        return result;
    }

    auto from_trgcode(const libeep::trgcode_t& c) const -> std::array<char, evt_label_size> {
        std::array<char, evt_label_size> code;
        std::copy(c, c + evt_label_size, begin(code));
        return code;
    }

    auto information() const -> Info {
        libeep::record_info_t x;
        libeep::eep_get_recording_info(cnt.get(), &x);
        return recordinfo2info(x);
    }

    auto history() const -> std::string {
        if (!libeep::eep_has_history(cnt.get())) {
            return "";
        }

        return libeep::eep_get_history(cnt.get());
    }

    auto segment_start_time() const -> DcDate {
        libeep::record_info_t x;
        libeep::eep_get_recording_info(cnt.get(), &x);
        return { x.m_startDate, x.m_startFraction};
    }

    auto file_version() const -> FileVersion {
        const int major{ libeep::eep_get_fileversion_major(cnt.get()) };
        const int minor{ libeep::eep_get_fileversion_minor(cnt.get()) };

        return { cast(major, uint8_t{}, ok{}), cast(minor, uint8_t{}, ok{}) };
    }

    auto description() const -> time_signal {
        return { api::dcdate2timepoint(segment_start_time()), sampling_frequency(), channels(), epoch_length(), 0 };
    }

    auto measure_get(ptrdiff_t start, ptrdiff_t length) -> std::chrono::microseconds {
        const measurement_count samples{ cast(length, sint{}, ok{}) };
        std::vector<int32_t> result(as_sizet_unchecked(matrix_size(channel_count(), samples)));
        const int absolute{ 0 };

        const auto s{ std::chrono::steady_clock::now() };

        if (libeep::eep_seek(cnt.get(), libeep::DATATYPE_EEG, start, absolute) != CNTERR_NONE) {
            const auto e{ std::chrono::steady_clock::now() };
            return std::chrono::duration_cast<std::chrono::microseconds>(e - s);
        }

        if (libeep::eep_read_sraw(cnt.get(), libeep::DATATYPE_EEG, result.data(), length) != CNTERR_NONE) {
            const auto e{ std::chrono::steady_clock::now() };
            return std::chrono::duration_cast<std::chrono::microseconds>(e - s);
        }

        const auto e{ std::chrono::steady_clock::now() };
        return std::chrono::duration_cast<std::chrono::microseconds>(e - s);
    }


};


struct libeep_writer
{
    libeep::eeg_t* cnt;
    sint height;

    explicit
    libeep_writer(libeep::eeg_t* cnt)
    : cnt{ cnt }
    , height{ libeep::eep_get_chanc(cnt) } {
        if (height <= 0) {
            throw ctk_data{ "libeep_writer: invalid channel count" };
        }
    }

    auto range_row_major(const std::vector<int32_t>&) -> void {
        throw ctk_limit{ "libeep_writer::range_row_major: invalid operation" };
    }

    auto range_column_major(const std::vector<int32_t>& client) -> void {
        const sint area{ vsize(client) };
        const auto [length, rem]{ std::div(area, height) };
        if (rem != 0) {
            throw ctk_limit{ "libeep_writer::range_column_major: invalid operation" };
        }

        if (libeep::eep_write_sraw(cnt, client.data(), cast(length, uint64_t{}, guarded{})) != CNTERR_NONE) {
            throw ctk_limit{ "libeep_writer::range_column_major: eep_write_sraw failed" };
        }
    }

    auto epoch_row_major(const std::vector<int32_t>&) -> void {
        throw ctk_bug{ "libeep_writer::epoch_row_major: invalid operation" };
    }

    auto epoch_column_major(const std::vector<int32_t>&) -> void {
        throw ctk_bug{ "TODO" };
    }

    auto trigger(const Trigger&) -> void {
        throw ctk_bug{ "libeep_writer::trigger: invalid operation" };
    }

    auto triggers(const std::vector<Trigger>& triggers) -> void {
        trg_ptr trg{ libeep::trg_init() };
        if (!trg) {
            throw ctk_limit{ "libeep_writer::triggers: trg_init failed" };
        }

        using free_ptr = std::unique_ptr<libeep::trgentry_t, mem_free>;

        const auto trigger_size{ triggers.size() };
        free_ptr entries{ (libeep::trgentry_t*)malloc(trigger_size * sizeof(libeep::trgentry_t)) };
        if (!entries) {
            throw ctk_limit{ "libeep_writer::triggers: can not allocate memory" };
        }
        libeep::trgentry_t* v{ entries.get() };

        for (size_t i{ 0 }; i < trigger_size; ++i) {
            v[i].sample = triggers[i].sample;
            std::strncpy(v[i].code, &(triggers[i].code[0]), TRG_CODE_LENGTH);
            v[i].code[TRG_CODE_LENGTH] = '\0';
        }

        trg->v = entries.release();
        trg->c = trigger_size;
        libeep::eep_set_trg(cnt, trg.release());
    }

    auto flush() -> void {
    }

    auto is_closed() const -> bool {
        return true;
    }

    auto close() -> void {
    }

    auto set_info(const DcDate& start_time, const Info& data) -> void {
        libeep::record_info_t i{ info2recordinfo(data) };
        i.m_startDate = start_time.date;
        i.m_startFraction = start_time.fraction;
        eep_set_recording_info(cnt, &i);
    }

    auto files() const -> std::vector<std::string> {
        return {};
    }

    auto sample_count() const -> measurement_count {
        return measurement_count{ 0 };
    }
};


class cnt_writer_libeep_riff
{
    using writer_ptr = std::unique_ptr<libeep_writer>;

    RiffType t;
    file_ptr f;
    cnt_ptr cnt;
    writer_ptr writer;
    std::string file_name;
    DcDate start_time;
    std::string history;

public:

    cnt_writer_libeep_riff(const std::string& fname, RiffType t, const std::string& h)
    : f{ open_w(fname) }
    , t{ t }
    , file_name{ fname }
    , history{ h } {
    }

    ~cnt_writer_libeep_riff() = default;
    cnt_writer_libeep_riff(const cnt_writer_libeep_riff&) = delete;
    cnt_writer_libeep_riff& operator=(const cnt_writer_libeep_riff&) = delete;

    auto close() -> void {
        writer.reset(nullptr);
        if (libeep::eep_finish_file(cnt.release()) != CNTERR_NONE) {
            throw ctk_limit{ "cnt_writer_libeep_riff::close: eep_fclose failed" };
        }
        cnt.reset(nullptr);
        f.reset(nullptr);
    }

    auto flush() -> void {
    }

    auto recording_info(const Info& x) -> void {
        if (!writer) {
            throw ctk_limit{ "not implemented" };
        }

        writer->set_info(start_time, x);
    }

    auto add_time_signal(const time_signal& x) -> libeep_writer* {
        start_time = api::timepoint2dcdate(x.ts.start_time);

        using free_ptr = std::unique_ptr<libeep::eegchan_t, mem_free>;

        const short chanc{ cast(x.ts.electrodes.size(), short{}, ok{}) };
        free_ptr elc{ (libeep::eegchan_t*)malloc(size_t(chanc) * sizeof(libeep::eegchan_t)) };
        if (!elc) {
            throw ctk_limit{ "cnt_writer_libeep_riff::add_time_signal: can not allocate elc" };
        }
        libeep::eegchan_t* e{ elc.get() };
        for (size_t i{ 0 }; i < x.ts.electrodes.size(); ++i) {
            std::strncpy(e[i].lab, x.ts.electrodes[i].label.c_str(), sizeof(e[i].lab) - 1);
            e[i].iscale = x.ts.electrodes[i].iscale;
            e[i].rscale = x.ts.electrodes[i].rscale;
            std::strncpy(e[i].runit, x.ts.electrodes[i].unit.c_str(), sizeof(e[i].runit) - 1);
            std::strncpy(e[i].reflab, x.ts.electrodes[i].reference.c_str(), sizeof(e[i].reflab) - 1);
            std::strncpy(e[i].status, x.ts.electrodes[i].status.c_str(), sizeof(e[i].status) - 1);
            std::strncpy(e[i].type, x.ts.electrodes[i].type.c_str(), sizeof(e[i].type) - 1);
        }

        const auto period{ 1.0 / x.ts.sampling_frequency };
        cnt.reset(libeep::eep_init_from_values(period, chanc, elc.release()));
        if (!cnt) {
            throw ctk_data{ "cnt_writer_libeep_riff::add_time_signal: eep_init_from_values failed" };
        }
        if (t == RiffType::riff32) {
            if (libeep::eep_create_file(cnt.get(), file_name.c_str(), f.get(), nullptr, 0, "") != CNTERR_NONE) {
                throw ctk_data{ "cnt_writer_libeep_riff::add_time_signal: eep_create_file failed" };
            }
        }
        else {
            if (libeep::eep_create_file64(cnt.get(), file_name.c_str(), f.get(), "") != CNTERR_NONE) {
                throw ctk_data{ "cnt_writer_libeep_riff::add_time_signal: eep_create_file64 failed" };
            }
        }

        auto chanv{ natural_row_order(sensor_count{ chanc }) };
        const sint length{ x.ts.epoch_length };
        if (libeep::eep_prepare_to_write(cnt.get(), libeep::DATATYPE_EEG, cast(length, uint64_t{}, ok{}), chanv.data()) != CNTERR_NONE) {
            throw ctk_data{ "cnt_writer_libeep_riff::add_time_signal: eep_prepare_to_write failed" };
        }

        libeep::eep_set_history(cnt.get(), history.c_str());

        writer.reset(new libeep_writer{ cnt.get() });
        if (!writer) {
            throw ctk_limit{ "cnt_writer_libeep_riff::add_time_signal: can not allocate writer" };
        }
        return writer.get();
    }
};


template<typename EegReader1, typename EegReader2>
auto compare_readers(EegReader1& r1, EegReader2& r2, bool ignore_trailing_ws) -> void {
    REQUIRE(r1.sample_count() == r2.sample_count());
    REQUIRE(r1.channels() == r2.channels());
    REQUIRE(r1.triggers() == r2.triggers());
    REQUIRE(r1.description() == r2.description());
    if (ignore_trailing_ws) {
        REQUIRE(trim(r1.history()) == trim(r2.history()));
    }
    else {
        REQUIRE(r1.history() == r2.history());
    }
    REQUIRE(r1.information() == r2.information());
    const FileVersion u0{ r1.file_version() };
    const FileVersion u1{ r2.file_version() };

    // different chunk sizes? computed here or as parameter?
    const measurement_count chunk{ 1 };
    const auto samples{ r1.sample_count() };
    for (measurement_count i{ 0 }; i < samples; ++i) {
        const auto v1{ r1.range_column_major(i, chunk) };
        const auto v2{ r2.range_column_major(i, chunk) };
        REQUIRE(v1 == v2);
    }
}




auto make_chunk_sizes_small_input_files(measurement_count epoch_length, measurement_count sample_count) -> std::vector<measurement_count> {
    const measurement_count two{ 2 };

    std::vector<measurement_count> result;
    for (auto chunk{ sample_count }; chunk != 1; chunk /= two) {
        result.push_back(chunk);
    }
    result.push_back(measurement_count{ 1 });
    result.push_back(epoch_length);

    std::sort(begin(result), end(result));
    return result;
}


auto make_chunk_sizes(measurement_count epoch_length, measurement_count sample_count) -> std::vector<measurement_count> {
    const measurement_count two{ 2 };
    const measurement_count three_epochs{ epoch_length * two + epoch_length };
    const measurement_count max_size{ std::min(three_epochs, sample_count) };

    std::vector<measurement_count> result;
    for (auto chunk{ max_size }; chunk != 1; chunk /= two) {
        result.push_back(chunk);
    }
    result.push_back(measurement_count{ 1 });
    result.push_back(epoch_length);

    std::sort(begin(result), end(result));
    return result;
}


template<typename Lib>
auto consume_samples(Lib& lib, measurement_count sample_count, measurement_count chunk) -> void {
    constexpr const column_major2row_major transpose; // reflib

    auto leftover{ sample_count };
    for (measurement_count i{ 0 }; i < sample_count && leftover != 0; i += chunk) {
        const auto stride{ std::min(chunk, leftover) };
        leftover -= stride;

        const auto n{ static_cast<sint>(i) };
        const auto amount{ static_cast<sint>(stride) };
        lib.get(n, amount, transpose);
    }
}


template<typename Lib>
auto access_in_variable_sized_chunks(Lib& lib) -> void {
    const auto count{ lib.sample_count() };
    const auto length{ lib.epoch_length() };
    const auto sizes{ make_chunk_sizes(length, count) };

    for (auto chunk : sizes) {
        consume_samples(lib, count, chunk);
    }
}


// asks libeep to do the work performed by read_cnt in epoch_reader.h
auto is_constructable(const std::string& fname) -> bool {
    cnt_reader_libeep_riff eeplib{ fname };
    const auto samples{ eeplib.sample_count() };
    const auto order{ eeplib.channel_order() };
    const auto channels{ eeplib.channels() };
    const auto sampling_frequency{ eeplib.sampling_frequency() };
    return 0 < samples && 0.00001 < sampling_frequency && !order.empty() && !channels.empty();
}



/*
TEST_CASE("libeep/reflib file access: no data comparison", "[compatibility] [read]") {
    const size_t fname_width{ 50 };
    input_txt input;
    std::string fname{ input.next() };

    while (!fname.empty()) {
        bool expected_to_fail{ false }; // if libeep fails with this input then it's ok for reflib to fail as well
        try {
            std::cerr << s2s(fname, fname_width) << " ";
            expected_to_fail = !is_constructable(fname);
        }
        catch(const std::exception&) {
            expected_to_fail = true;
        }

        try {
            cnt_reader_libeep_riff eeplib{ fname };
            access_in_variable_sized_chunks(eeplib);
        }
        catch (const std::exception&) {
            expected_to_fail = true;
            ignore_expected();
        }

        try {
            cnt_reader_reflib_riff reflib{ fname };
            access_in_variable_sized_chunks(reflib);
        }
        catch (const std::exception&) {
            if (!expected_to_fail) {
                FILE* f{ fopen("compatibility_bugs.txt", "a+") };
                if (!f) {
                    std::cerr << "can not open bug file for appending\n";
                }
                else {
                    write(f, begin(fname), end(fname));
                    write(f, '\n');
                    fclose(f);
                }
                std::cerr << "COMPATIBILITY BUG\n";
            }

            ignore_expected();
            fname = input.next();
            continue;
        }

        std::cerr << "ok\n";
        fname = input.next();
    }
}
*/


auto measure_read_samples(cnt_reader_libeep_riff& eeplib, measurement_count sample_count, measurement_count chunk) -> std::chrono::microseconds {
    std::chrono::microseconds sum_time{ 0 };

    auto leftover{ sample_count };
    for (measurement_count i{ 0 }; i < sample_count && leftover != 0; i += chunk) {
        const auto stride{ std::min(chunk, leftover) };
        leftover -= stride;

        const auto n{ static_cast<sint>(i) };
        const auto amount{ static_cast<sint>(stride) };
        sum_time += eeplib.measure_get(n, amount);
    }

    return sum_time;
}


auto measure_read_samples(cnt_reader_reflib_riff& reflib, measurement_count sample_count, measurement_count chunk) -> std::chrono::microseconds {
    std::chrono::microseconds sum_time{ 0 };

    auto leftover{ sample_count };
    for (measurement_count i{ 0 }; i < sample_count && leftover != 0; i += chunk) {
        const auto stride{ std::min(chunk, leftover) };
        leftover -= stride;

        const auto s{ std::chrono::steady_clock::now() };
        const auto v = reflib.range_column_major(i, stride);
        const auto e{ std::chrono::steady_clock::now() };

        sum_time += std::chrono::duration_cast<std::chrono::microseconds>(e - s);

    }

    return sum_time;
}


using execution_time = std::pair<double, measurement_count>;

auto print(const std::string& op, const dimensions& x, measurement_count samples, std::vector<execution_time> execution_times) -> void {
    std::cerr
    << " " << x.height << "x" << x.length
    << ", " << samples
    << ", " << op << ", [time r/l ";

    const auto first{ begin(execution_times) };
    const auto last{ end(execution_times) };
    if (first == last) {
        std::cerr << "f: no chunks\n";
        return;
    }

    const auto faster = [](const auto& x, const auto& y) -> bool { return x.first < y.first; };
    const auto [fastest, slowest]{ std::minmax_element(first, last, faster) };

    const auto min_time{ fastest->first };
    const auto min_size{ fastest->second }; // chunk size which used to produce the fastest decoding
    const auto max_time{ slowest->first };
    const auto max_size{ slowest->second }; // chunk size which used to produce the slowest decoding

    const auto pick_time = [](const auto& x) -> double { return x.first; };
    std::vector<double> times(execution_times.size());
    std::transform(first, last, begin(times), pick_time);

    const auto mean_time{ average(begin(times), end(times)) };
    const auto stddev{ standard_deviation(begin(times), end(times)) };
    
    std::cerr
        << "min " << d2s(min_time) << "%" // << " (" << min_size << ")"
        << ", max " << d2s(max_time) << "%" //<< " (" << max_size << ")"
        << ", avg " << d2s(mean_time) << "%"
        << ", stddev " << d2s(stddev) << "%]";
        //<< ", [size c/u " << d2s(comp_uncomp * 100) << "%]";

    const auto median{ std::distance(first, last) / 2 };
    std::sort(first, last, faster);
    std::cerr << ", [median " << d2s(execution_times[median].first) << "%: " << execution_times[median].second;

    const execution_time mean{ mean_time, measurement_count{ 0 } };
    const auto avg{ std::upper_bound(first, last, mean, faster) };
    if (avg == last) {
        std::cerr << ", mean n/a: n/a ]\n";
        return;
    }

    std::cerr << ", mean " << d2s(avg->first) << "%: " << avg->second << "]\n";
}


auto compare_libeep_reflib_readers(const std::string& fname) -> void {
    cnt_reader_reflib_riff reflib{ fname };
    cnt_reader_libeep_riff eeplib{ fname };
    //cnt_reader_reflib_riff eeplib{ fname };
    const bool ignore_trailing_ws{ false };
    compare_readers(eeplib, reflib, ignore_trailing_ws);

    const auto samples{ eeplib.sample_count() };
    const auto length{ eeplib.epoch_length() };
    const auto order{ eeplib.channel_order() };

    //const auto sizes{ make_chunk_sizes_small_input_files(length, samples) };
    const auto sizes{ make_chunk_sizes(length, samples) };
    using execution_time = std::pair<double, measurement_count>;
    std::vector<execution_time> execution_times;
    execution_times.reserve(sizes.size());

    for (auto chunk : sizes) {
        const auto l_time{ measure_read_samples(eeplib, samples, chunk) };
        const auto r_time{ measure_read_samples(reflib, samples, chunk) };

        const double ref_eep{ 100.0 * r_time.count() / l_time.count() };
        execution_times.emplace_back(ref_eep, chunk);
    }

    print("read", { eeplib.channel_count(), length }, samples, execution_times);
}


TEST_CASE("libeep/reflib data comparison", "[compatibility] [performance] [read]") {
    const size_t fname_width{ 7 };
    input_txt input;
    std::string fname{ input.next() };

    while (!fname.empty()) {
        try {
            std::cerr << s2s(fname, fname_width);
            compare_libeep_reflib_readers(fname);
        }
        catch(const std::exception& e) {
            FILE* f{ fopen("errors.txt", "at") };
            fprintf(f, "%s: %s\n", fname.c_str(), e.what());
            fclose(f);
            ignore_expected();
        }

        fname = input.next();
    }
}


auto replace_extension(const std::string& fname, const std::string& extension) -> std::string {
    std::filesystem::path p{ fname };
    std::filesystem::path cnt_name{ p.filename() };
    std::filesystem::path stem{ cnt_name.stem() };
    auto name{ stem };
    name += '.';
    name += extension;
    return p.replace_filename(name).string();
}

auto add_user_chunks(const std::string& fname, cnt_writer_reflib_riff& writer) -> bool {
    const char extensions[][4] { "evt", "seg", "sen", "trg" };
    bool appended{ false };

    for (const auto* extension : extensions) {
        const std::string satelite_name{ replace_extension(fname, extension) };
        std::string label{ extension };
        std::reverse(begin(label), end(label));

        try {
            file_ptr f{ open_r(satelite_name) };
        }
        catch(const api::v1::ctk_data&) {
            continue;
        }

        writer.embed(label, satelite_name);
        appended = true;
    }

    return appended;
}

auto compare_file_content(const std::string& x, const std::string& y) -> void {
    file_ptr fx{ open_r(x) };
    file_ptr fy{ open_r(y) };

    constexpr const int64_t stride{ 1024 * 4 }; // arbitrary
    std::array<uint8_t, stride> bx;
    std::array<uint8_t, stride> by;

    auto sx{ file_size(fx.get()) };
    auto sy{ file_size(fy.get()) };
    if (sx != sy) {
        throw ctk_bug{ "compare_file_content: different file sizes" };
    }

    auto chunk{ std::min(sx, stride) };
    while (0 < chunk) {
        const auto first_x{ begin(bx) };
        const auto last_x{ begin(bx) + chunk };
        read(fx.get(), first_x, last_x);

        const auto first_y{ begin(by) };
        const auto last_y{ begin(by) + chunk };
        read(fy.get(), first_y, last_y);

        const auto[mx, my]{ std::mismatch(first_x, last_x, first_y) };
        if (mx != last_x || my != last_y) {
            throw ctk_bug{ "compare_file_content: different file content" };
        }

        sx -= chunk;
        chunk = std::min(sx, stride);
    }
}

auto compare_user_chunks(const std::string& destname, cnt_reader_reflib_riff& reader, bool has_user, const std::string& srcname) -> bool {
    const auto labels{ reader.embedded_files() };
    if (!has_user) {
        return labels.empty();
    }

    if (labels.empty()) {
        throw ctk_bug{ "compare_user_chunks: no user chunks" };
    }

    for (const auto& label : labels) {
        std::string extension{ label.c_str(), 3 };
        std::reverse(begin(extension), end(extension));

        const std::string src_satelite_name{ replace_extension(srcname, extension) };
        const std::string dest_satelite_name{ replace_extension(destname, extension) };
        reader.extract_embedded_file(label, dest_satelite_name);
        compare_file_content(src_satelite_name, dest_satelite_name);
        if (std::remove(dest_satelite_name.c_str()) != 0) {
            std::cerr << "compare_user_chunks: can not delete " << dest_satelite_name << "\n";
        }
    }

    return true;
}

auto writer_consistency_compatibility(const std::string& fname, measurement_count sample_count, measurement_count chunk, RiffType riff) -> void {
    const std::string temp_name{ "delme.cnt" };

    // scope needed on windows. else delme.cnt cannot be deleted.
    {
        cnt_reader_reflib_riff input{ fname };
        cnt_writer_reflib_riff output{ temp_name, riff, input.history() };
        output.recording_info(input.information());

        // REMEMBER MISSING: input.channel_order()
        auto* segment{ output.add_time_signal(input.description()) };
        segment->triggers(input.triggers());

        auto stride{ std::min(sample_count, chunk) };
        measurement_count i{ 0 };
        while (stride > 0) {
            segment->range_column_major(input.range_column_major(i, stride));

            i += stride;
            sample_count -= stride;
            stride = std::min(sample_count, chunk);
        }

        const bool has_user{ add_user_chunks(fname, output) };

        output.close();

        bool ignore_trailing_ws{ false };
        cnt_reader_reflib_riff reflib{ temp_name };
        compare_readers(input, reflib, ignore_trailing_ws); // consistency
        REQUIRE(compare_user_chunks(temp_name, reflib, has_user, fname));

        ignore_trailing_ws = true;
        cnt_reader_libeep_riff eeplib{ temp_name };
        compare_readers(eeplib, reflib, ignore_trailing_ws); // compatibility
    }

    if (std::remove(temp_name.c_str()) != 0) {
        std::cerr << "writer_consistency_compatibility: can not delete " << temp_name << "\n";
    }
}

auto test_writer(const std::string& fname) -> void {
    std::vector<measurement_count> sizes;
    measurement_count count;
    {
        cnt_reader_reflib_riff lib{ fname };
        count = lib.sample_count();
        const auto length = lib.epoch_length();
        sizes = make_chunk_sizes(length, count);
    }

    std::cerr << " writing in chunks of ";
    for (auto chunk : sizes) {
        std::cerr << chunk << " ";
        writer_consistency_compatibility(fname, count, chunk, RiffType::riff32);
        writer_consistency_compatibility(fname, count, chunk, RiffType::riff64);
    }
}



TEST_CASE("test writer", "[consistency] [compatibility] [write]") {
    const size_t fname_width{ 7 };
    input_txt input;
    std::string fname{ input.next() };

    while (!fname.empty()) {
        try {
            std::cerr << s2s(fname, fname_width);
            test_writer(fname);
            std::cerr << "ok\n";
        }
        catch(const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
    }
}


template<typename Reader, typename Writer>
auto writer_speed(Reader& reader, Writer& writer, measurement_count sample_count, measurement_count chunk, const std::string& fname, bool ignore_trailing_ws) -> std::chrono::microseconds {
    const auto s{ std::chrono::steady_clock::now() };

    // REMEMBER MISSING: reader.channel_order()
    auto* segment{ writer.add_time_signal(reader.description()) };
    segment->triggers(reader.triggers());

    writer.recording_info(reader.information());

    auto stride{ std::min(sample_count, chunk) };
    measurement_count i{ 0 };
    while (stride > 0) {
        segment->range_column_major(reader.range_column_major(i, stride));

        i += stride;
        sample_count -= stride;
        stride = std::min(sample_count, chunk);
    }

    writer.close();

    const auto e{ std::chrono::steady_clock::now() };

    cnt_reader_reflib_riff control{ fname };
    compare_readers(reader, control, ignore_trailing_ws);

    return std::chrono::duration_cast<std::chrono::microseconds>(e - s);
}


auto test_writer_speed(const std::string& fname) -> void {

    cnt_reader_reflib_riff reader{ fname };
    const auto count{ reader.sample_count() };
    const auto length{ reader.epoch_length() };
    const auto sizes{ make_chunk_sizes(length, count) };

    std::vector<execution_time> execution_times;
    execution_times.reserve(sizes.size());

    for (auto chunk : sizes) {
        bool ignore_trailing_ws{ false };
        cnt_writer_reflib_riff writer_reflib{ "reflib.cnt", RiffType::riff64, reader.history() };
        const auto r_time{ writer_speed(reader, writer_reflib, count, chunk, "reflib.cnt", ignore_trailing_ws) };

        ignore_trailing_ws = true;
        cnt_writer_libeep_riff writer_libeep{ "libeep.cnt", RiffType::riff64, reader.history() };
        const auto l_time{ writer_speed(reader, writer_libeep, count, chunk, "libeep.cnt", ignore_trailing_ws) };

        const double ref_eep{ 100.0 * r_time.count() / l_time.count() };
        execution_times.emplace_back(ref_eep, chunk);
    }
    if (std::remove("reflib.cnt") != 0) { std::cerr << "test_writer_speed: can not delete reflib.cnt\n"; }
    if (std::remove("libeep.cnt") != 0) { std::cerr << "test_writer_speed: can not delete libeep.cnt\n"; }

    const auto chan{ vsize(reader.channels()) };
    const sensor_count channels{ chan };
    print("write", { channels, reader.epoch_length() }, reader.sample_count(), execution_times);
}



TEST_CASE("test writer speed", "[performance] [write]") {
    const size_t fname_width{ 7 };
    input_txt input;
    std::string fname{ input.next() };

    while (!fname.empty()) {
        try {
            std::cerr << s2s(fname, fname_width);
            test_writer_speed(fname);
        }
        catch(const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
    }
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */


