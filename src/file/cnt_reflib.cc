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


namespace ctk { namespace impl {

auto flat2riff(const std::filesystem::path& input_name, const std::filesystem::path& output_name, const std::vector<tagged_file>& tokens, const std::vector<external_file>& user) -> void {
    const epoch_reader_flat reader{ input_name, tokens };

    // additional consistency check: accessibility of the last epoch
    const auto epochs{ reader.common_epoch_reader().count() };
    if (epochs == 0) {
        throw api::v1::CtkData{ "flat2riff: empty time signal" };
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

auto cnt_writer_reflib_riff::add_time_signal(const api::v1::TimeSeries& description) -> cnt_writer_reflib_flat* {
    if (flat_writer) {
        throw api::v1::CtkLimit{ "cnt_writer_reflib_riff::add_time_signal: one segment only" };
    }
    flat_writer.reset(new cnt_writer_reflib_flat{ fname_flat(file_name), description, riff });
    assert(flat_writer);

    flat_writer->info(information);
    flat_writer->history(notes);
    return flat_writer.get();
}

auto cnt_writer_reflib_riff::embed(std::string label, const std::filesystem::path& fname) -> void {
    label = as_string(as_label(label)); // clamp label

    // refh and imp are recognized by libeep but not used.
    // nsh, vish, egih, egig, egiz, binh are mentioned in http://eeprobe.ant-neuro.com/doc/cnt_riff.txt but not referenced in the libeep code.
    const std::vector<std::string> reserved{ "eeph", "info", "evt ", "raw3", "rawf", "stdd", "tfh ", "tfd ", "refh", "imp ", "nsh ", "vish", "egih", "egig", "egiz", "binh" };
    const auto verboten{ std::find(begin(reserved), end(reserved), label) };
    if (verboten != end(reserved)) {
        throw api::v1::CtkLimit{ "cnt_writer_reflib_riff::embed: label reserved by libeep" };
    }

    const auto already_present = [label](const external_file& x) -> bool { return x.label == label; };
    const auto used_up{ std::find_if(begin(user), end(user), already_present) };
    if (used_up != end(user)) {
        throw api::v1::CtkLimit{ "cnt_writer_reflib_riff::embed: duplicate label" };
    }

    user.emplace_back(label, fname);
}

auto cnt_writer_reflib_riff::commited() const -> measurement_count {
    if (!flat_writer) {
        throw api::v1::CtkLimit{ "cnt_writer_reflib_riff::commited: invalid operation" };
    }

    return flat_writer->commited();
}

auto cnt_writer_reflib_riff::range_row_major(measurement_count i, measurement_count samples) -> std::vector<int32_t> {
    if (!flat_writer) {
        throw api::v1::CtkLimit{ "cnt_writer_reflib_riff::range_row_major: invalid operation" };
    }

    return flat_writer->range_row_major(i, samples);
}

auto cnt_writer_reflib_riff::range_column_major(measurement_count i, measurement_count samples) -> std::vector<int32_t> {
    if (!flat_writer) {
        throw api::v1::CtkLimit{ "cnt_writer_reflib_riff::range_column_major: invalid operation" };
    }

    return flat_writer->range_column_major(i, samples);
}

auto cnt_writer_reflib_riff::flush() -> void {
    if (!flat_writer) {
        throw api::v1::CtkLimit{ "cnt_writer_reflib_riff::flush: invalid operation" };
    }

    flat_writer->flush();
}

auto cnt_writer_reflib_riff::close() -> void {
    if (!flat_writer) {
        return;
    }

    const auto tokens{ flat_writer->file_tokens() };
    flat_writer->close();
    flat_writer.reset(nullptr);

    flat2riff(fname_flat(file_name), file_name, tokens, user);

    const auto get_fname = [](const auto& x) -> std::filesystem::path { return x.file_name; };

    std::vector<std::filesystem::path> temporary_files(tokens.size());
    std::transform(begin(tokens), end(tokens), begin(temporary_files), get_fname);
    if (!delete_files(temporary_files)) {
        throw api::v1::CtkData{ "cnt_writer_reflib_riff::close: cannot delete temporary files" };
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

    const auto inverse = [](double d) -> double { return 1.0 / d; };
    std::transform(begin(ys), end(ys), begin(ys), inverse);

    return ys;

}


} /* namespace impl */ } /* namespace ctk */

