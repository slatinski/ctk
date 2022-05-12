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

#include "file/cnt_reflib.h"

#include "logger.h"


namespace ctk { namespace impl {

auto flat2riff(const std::filesystem::path& input_name, const std::filesystem::path& output_name, const std::vector<tagged_file>& tokens, const std::vector<external_file>& user) -> void {
    const epoch_reader_flat reader{ input_name, tokens };

    // additional consistency check: accessibility of the last epoch
    const auto epochs{ reader.common_epoch_reader().count() };
    if (epochs == 0) {
        const std::string e{ "[flat2riff, cnt_reflib] no compressed data" };
        ctk_log_error(e);
        throw api::v1::CtkData{ e };
    }
    reader.common_epoch_reader().epoch(epochs - epoch_count{ 1 });

    riff_list root{ reader.writer_map() };
    const auto t{ reader.common_epoch_reader().cnt_type() };
    const int64_t no_offset{ 0 }; // verbatim copy of the whole file
    for (const auto& x : user) {
        root.push_back(riff_node{ riff_file{ data_chunk(t, x.label), x.file_name, no_offset } });
    }

    const std::vector<std::pair<std::string, std::string>> satelites{
        {"evt", "xevt" }, // file extension, top-level chunk name
        {"seg", "xseg" },
        {"sen", "xsen"},
        {"trg", "xtrg"}};
    for (const auto& satelite : satelites) {
        const std::filesystem::path satelite_name{ std::filesystem::path{ output_name }.replace_extension(satelite.first) };
        if (!std::filesystem::exists(satelite_name)) {
            continue;
        }
        root.push_back(riff_node{ riff_file{ data_chunk(t, satelite.second), satelite_name, no_offset } });
    }

    auto f{ open_w(output_name) };

    riff_list phony{ list_chunk(api::v1::RiffType::riff32, "xxxx") };
    phony.push_back(root);
    phony.back().write(f.get());
}


double2int::double2int(double f)
: factor{ f } {
}

auto double2int::operator()(double x) const -> int32_t {
    return static_cast<int32_t>(std::round(x * factor));
}

int2double::int2double(double f)
: factor{ f } {
}

auto int2double::operator()(int32_t x) const -> double {
    return x * factor;
}


cnt_writer_reflib_riff::cnt_writer_reflib_riff(const std::filesystem::path& fname, api::v1::RiffType riff)
    : riff{ riff }
    , file_name{ fname } {
}

auto cnt_writer_reflib_riff::recording_info(const api::v1::Info& x) -> void {
    information = x;

    if (!flat_writer) {
        return;
    }
    flat_writer->info(x);
}

auto cnt_writer_reflib_riff::history(const std::string& x) -> void {
    notes = x;

    if (!flat_writer) {
        return;
    }
    flat_writer->history(x);
}

auto cnt_writer_reflib_riff::add_time_signal(const api::v1::TimeSeries& param) -> cnt_writer_reflib_flat* {
    if (flat_writer) {
        const std::string e{ "[cnt_writer_reflib_riff::add_time_signal, cnt_reflib] one segment only" };
        ctk_log_critical(e);
        throw api::v1::CtkBug{ e };
    }
    flat_writer.reset(new cnt_writer_reflib_flat{ fname_flat(file_name), param, riff });
    assert(flat_writer);

    flat_writer->info(information);
    flat_writer->history(notes);
    return flat_writer.get();
}

auto cnt_writer_reflib_riff::embed(std::string label, const std::filesystem::path& fname) -> void {
    label = as_string(as_label(label)); // clamp label

    // refh and imp are recognized by libeep but not used.
    // nsh, vish, egih, egig, egiz, binh are mentioned in http://eeprobe.ant-neuro.com/doc/cnt_riff.txt but not referenced in the libeep code.
    // xevt, xseg, xsen and xtrg are used by this library.
    const std::vector<std::string> reserved{ "eeph", "info", "evt ", "raw3", "rawf", "stdd", "tfh ", "tfd ", "refh", "imp ", "nsh ", "vish", "egih", "egig", "egiz", "binh", "xevt", "xseg", "xsen", "xtrg" };
    const auto verboten{ std::find(begin(reserved), end(reserved), label) };
    if (verboten != end(reserved)) {
        std::ostringstream oss;
        oss << "[cnt_writer_reflib_riff::embed, cnt_reflib] reserved label " << *verboten;
        const auto e{ oss.str() };
        ctk_log_error(e);
        throw api::v1::CtkLimit{ e };
    }

    const auto already_present = [label](const external_file& x) -> bool { return x.label == label; };
    const auto used_up{ std::find_if(begin(user), end(user), already_present) };
    if (used_up != end(user)) {
        std::ostringstream oss;
        oss << "[cnt_writer_reflib_riff::embed, cnt_reflib] duplicated label " << used_up->label;
        const auto e{ oss.str() };
        ctk_log_error(e);
        throw api::v1::CtkLimit{ e };
    }

    user.emplace_back(label, fname);
}

auto cnt_writer_reflib_riff::commited() const -> measurement_count {
    if (!flat_writer) {
        const std::string e{ "[cnt_writer_reflib_riff::commited, cnt_reflib] invalid operation" };
        ctk_log_error(e);
        throw api::v1::CtkBug{ e };
    }

    return flat_writer->commited();
}

auto cnt_writer_reflib_riff::range_row_major(measurement_count i, measurement_count samples) -> std::vector<double> {
    if (!flat_writer) {
        const std::string e{ "[cnt_writer_reflib_riff::range_row_major, cnt_reflib] invalid operation" };
        ctk_log_error(e);
        throw api::v1::CtkBug{ e };
    }

    return flat_writer->range_row_major(i, samples);
}

auto cnt_writer_reflib_riff::range_column_major(measurement_count i, measurement_count samples) -> std::vector<double> {
    if (!flat_writer) {
        const std::string e{ "[cnt_writer_reflib_riff::range_column_major, cnt_reflib] invalid operation" };
        ctk_log_error(e);
        throw api::v1::CtkBug{ e };
    }

    return flat_writer->range_column_major(i, samples);
}

auto cnt_writer_reflib_riff::range_row_major_int32(measurement_count i, measurement_count samples) -> std::vector<int32_t> {
    if (!flat_writer) {
        const std::string e{ "[cnt_writer_reflib_riff::range_row_major_int32, cnt_reflib] invalid operation" };
        ctk_log_error(e);
        throw api::v1::CtkBug{ e };
    }

    return flat_writer->range_row_major_int32(i, samples);
}

auto cnt_writer_reflib_riff::range_column_major_int32(measurement_count i, measurement_count samples) -> std::vector<int32_t> {
    if (!flat_writer) {
        const std::string e{ "[cnt_writer_reflib_riff::range_column_major_int32, cnt_reflib] invalid operation" };
        ctk_log_error(e);
        throw api::v1::CtkBug{ e };
    }

    return flat_writer->range_column_major_int32(i, samples);
}

auto cnt_writer_reflib_riff::flush() -> void {
    if (!flat_writer) {
        const std::string e{ "[cnt_writer_reflib_riff::flush, cnt_reflib] invalid operation" };
        ctk_log_error(e);
        throw api::v1::CtkBug{ e };
    }

    flat_writer->flush();
}

auto cnt_writer_reflib_riff::close() -> void {
    if (!flat_writer) {
        return;
    }

    flat_writer->close();
    const auto tokens{ flat_writer->file_tokens() };
    const auto written{ flat_writer->sample_count() };
    flat_writer.reset(nullptr);

    if (written != 0) {
        flat2riff(fname_flat(file_name), file_name, tokens, user);
    }

    const auto get_fname = [](const auto& x) -> std::filesystem::path { return x.file_name; };

    std::vector<std::filesystem::path> temporary_files(tokens.size());
    std::transform(begin(tokens), end(tokens), begin(temporary_files), get_fname);
    if (!delete_files(temporary_files)) {
        const std::string e{ "[cnt_writer_reflib_riff::close, cnt_reflib] cannot delete temporary files" };
        ctk_log_error(e);
        throw api::v1::CtkData{ e };
    }
}


auto operator<<(std::ostream& os, const external_file& x) -> std::ostream& {
    os << "[" << x.label << "]: " << x.file_name;
    return os;
}

auto reader_scales(const std::vector<api::v1::Electrode>& xs) -> std::vector<double> {
    std::vector<double> ys(xs.size());

    const auto scale = [](const api::v1::Electrode& e) -> double { return e.IScale * e.RScale; };
    std::transform(begin(xs), end(xs), begin(ys), scale);

    return ys;
}

auto writer_scales(const std::vector<api::v1::Electrode>& xs) -> std::vector<double> {
    std::vector<double> ys(reader_scales(xs));

    const auto inverse = [](double d) -> double { return 1 / d; };
    std::transform(begin(ys), end(ys), begin(ys), inverse);

    return ys;

}


} /* namespace impl */ } /* namespace ctk */

