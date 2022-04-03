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

#include <string>
#include "file/cnt_reflib.h"
#include "test/util.h"

namespace ctk { namespace impl { namespace test {
    using namespace ctk::api::v1;

    const auto get_fname = [](const auto& x) -> std::filesystem::path { return x.file_name; };

TEST_CASE("read/write flat files - compressed epochs", "[consistency]") {
    const size_t fname_width{ 20 };
    const std::filesystem::path delme_cnt{ "delme.cnt" };

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
        std::vector<tagged_file> loose_files;

		try {
			std::cerr << s2s(fname, fname_width);

            epoch_reader_riff r_riff{ fname };
            const auto reflib_count{ r_riff.common_epoch_reader().count() };

            // scope for the epoch writer
            {
                // REMEMBER MISSING: r_riff.common_epoch_reader().channel_order()
                epoch_writer_flat w_flat{ fname_flat(delme_cnt)
                                             , r_riff.common_epoch_reader().param_eeg()
                                             , r_riff.common_epoch_reader().cnt_type() };
                w_flat.info(r_riff.common_epoch_reader().information());
                w_flat.history(r_riff.common_epoch_reader().history());
                w_flat.append(r_riff.common_epoch_reader().triggers());

                for (epoch_count i{ 0 }; i < reflib_count; ++i) {
                    w_flat.append(r_riff.common_epoch_reader().epoch(i));
                }

                loose_files = w_flat.file_tokens();
                w_flat.close();
            }

            // scope for the epoch reader (if the file is still open delete_flat_files fails on windows)
            {
                epoch_reader_flat r_flat{ delme_cnt };
                const auto flat_count{ r_flat.common_epoch_reader().count() };
                REQUIRE(flat_count == reflib_count);
                REQUIRE(r_riff.common_epoch_reader().param_eeg() == r_flat.common_epoch_reader().param_eeg());
                REQUIRE(r_riff.common_epoch_reader().history() == r_flat.common_epoch_reader().history());
                REQUIRE(r_riff.common_epoch_reader().triggers() == r_flat.common_epoch_reader().triggers());
                const auto[start_time, information, is_ascii]{ parse_info(r_flat.common_epoch_reader().info_content()) };
                REQUIRE(r_riff.common_epoch_reader().information() == information);

                for (epoch_count i{ 0 }; i < flat_count; ++i) {
                    REQUIRE(r_riff.common_epoch_reader().epoch(i) == r_flat.common_epoch_reader().epoch(i));
                }
            }

            std::vector<std::filesystem::path> fnames(loose_files.size());
            std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
            delete_files(fnames);
            std::cerr << " ok\n";
        }
        catch (const std::exception&) {
            ignore_expected();
            std::vector<std::filesystem::path> fnames(loose_files.size());
            std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
            delete_files(fnames);
        }

        fname = input.next();
    }
}


auto write_in_chunks(cnt_reader_reflib_riff& reader_reflib, const std::filesystem::path& fname, sint chunk_size) -> std::vector<tagged_file> {
    // REMEMBER MISSING: reader_reflib.channel_order()
    cnt_writer_reflib_flat flat_writer{ fname_flat(fname), reader_reflib.param_eeg(), RiffType::riff64 };
    flat_writer.info(reader_reflib.information());
    flat_writer.history(reader_reflib.history());

    const sint sample_count{ reader_reflib.sample_count() };
    for (sint i{ 0 }; i < sample_count; i += chunk_size) {
        const sint amount{ std::min(chunk_size, sint(sample_count - i)) };
        const measurement_count epoch_length{ amount };
        if (amount < 0) {
            throw CtkBug("test: write_in_chunks: negative amount");
        }
        if (amount == 0) {
            std::cerr << "test: write_in_chunks: not encoding 0 samples\n";
            break;
        }

        const measurement_count start{ i };
        flat_writer.range_column_major(reader_reflib.range_column_major(start, epoch_length));        
    }

    flat_writer.close();

    return flat_writer.file_tokens();
}


TEST_CASE("read/write flat files - uncompressed epochs", "[consistency]") {
    constexpr const size_t fname_width{ 20 };
    const std::filesystem::path delme_cnt{ "delme.cnt" };

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
        std::vector<tagged_file> loose_files;
		try {
			std::cerr << s2s(fname, fname_width);

            std::vector<ptrdiff_t> chunks{ 1, 2, 3 };
            {
                cnt_reader_reflib_riff reader_reflib{ fname };
                const auto el{ static_cast<measurement_count::value_type>(reader_reflib.epoch_length()) };
                const auto sc{ static_cast<measurement_count::value_type>(reader_reflib.sample_count()) };
                const ptrdiff_t epoch_length{ cast(el, ptrdiff_t{}, ok{}) };
                const ptrdiff_t sample_count{ cast(sc, ptrdiff_t{}, ok{}) };
                if (epoch_length < 3 || sample_count < 6) {
                    std::cerr << "the test will not work, skipping\n";
                    fname = input.next();
                    continue;
                }

                chunks.push_back(std::min(epoch_length - 3, sample_count));
                chunks.push_back(std::min(epoch_length - 2, sample_count));
                chunks.push_back(std::min(epoch_length - 1, sample_count));
                chunks.push_back(std::min(epoch_length, sample_count));
                chunks.push_back(std::min(epoch_length + 1, sample_count));
                chunks.push_back(std::min(epoch_length + 2, sample_count));
                chunks.push_back(std::min(epoch_length + 3, sample_count));
            }


            for (auto stride : chunks) {
                {
                    cnt_reader_reflib_riff reader_reflib{ fname };
                    loose_files = write_in_chunks(reader_reflib, delme_cnt, stride);

                    epoch_reader_riff reflib_reader{ fname };
                    const auto reflib_count{ reflib_reader.common_epoch_reader().count() };

                    epoch_reader_flat flat_reader{ delme_cnt };
                    const auto flat_count{ flat_reader.common_epoch_reader().count() };
                    REQUIRE(flat_count == reflib_count);
                    const auto [start_time, information, is_ascii] { parse_info(flat_reader.common_epoch_reader().info_content()) };
                    REQUIRE(reflib_reader.common_epoch_reader().information() == information);
                    REQUIRE(reflib_reader.common_epoch_reader().param_eeg().StartTime == api::v1::dcdate2timepoint(start_time));

                    matrix_decoder_reflib decode;
                    decode.row_count(reflib_reader.common_epoch_reader().channel_count());

                    for (epoch_count i{ 0 }; i < flat_reader.common_epoch_reader().count(); ++i) {
                        const auto rce{ reflib_reader.common_epoch_reader().epoch(i) };
                        const auto fce{ flat_reader.common_epoch_reader().epoch(i) };

                        using transform = row_major2row_major;
                        constexpr const transform transpose;
                        const auto v_rt{ decode(rce.data, rce.length, transpose) };
                        const auto v_ft{ decode(fce.data, fce.length, transpose) };
                        REQUIRE(v_rt == v_ft);

                        using cpy = row_major2row_major;
                        constexpr const cpy copy;
                        const auto v_rc{ decode(rce.data, rce.length, copy) };
                        const auto v_fc{ decode(fce.data, fce.length, copy) };
                        REQUIRE(v_rc == v_fc);
                    }
                }

                std::vector<std::filesystem::path> fnames(loose_files.size());
                std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
                delete_files(fnames);
                fnames.clear();
            }

            std::cerr << " ok\n";
        }
        catch (const std::exception&) {
            ignore_expected();
            std::vector<std::filesystem::path> fnames(loose_files.size());
            std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
            delete_files(fnames);
        }

        fname = input.next();
    }
}

TEST_CASE("cnt_writer_reflib_riff", "[consistency]") {
    constexpr const size_t fname_width{ 20 };
    const std::filesystem::path delme_cnt{ "delme.cnt" };

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
		try {
			std::cerr << s2s(fname, fname_width);
            {
                cnt_reader_reflib_riff r_orig{ fname };
                measurement_count sample_count{ 0 };
                measurement_count ch{ 1 };

                {
                    cnt_writer_reflib_riff writer{ delme_cnt, RiffType::riff64 }; // TODO: both 32/64
                    writer.recording_info(r_orig.information());
                    writer.history(r_orig.history());
                    // REMEMBER MISSING: r_orig.channel_order()
                    auto* raw3{ writer.add_time_signal(r_orig.param_eeg()) };

                    sample_count = r_orig.sample_count();
                    for (measurement_count i{ 0 }; i < sample_count; ++i) {
                        raw3->range_column_major(r_orig.range_column_major(i, ch));
                    }
                    raw3->triggers(r_orig.triggers());

                    writer.close();
                }

                cnt_reader_reflib_riff r_temp{ delme_cnt };
                REQUIRE(r_orig.epoch_length() == r_temp.epoch_length());
                REQUIRE(ascii_sampling_frequency(r_orig.sampling_frequency()) == ascii_sampling_frequency(r_temp.sampling_frequency()));
                REQUIRE(r_orig.segment_start_time() == r_temp.segment_start_time());
                REQUIRE(r_orig.channels() == r_temp.channels());
                REQUIRE(r_orig.sample_count() == r_temp.sample_count());
                REQUIRE(r_orig.triggers() == r_temp.triggers());
                REQUIRE(r_orig.information() == r_temp.information());
                for (measurement_count i{ 0 }; i < sample_count; ++i) {
                    REQUIRE(r_orig.range_column_major(i, ch) == r_temp.range_column_major(i, ch));
                }
            }

            REQUIRE(std::filesystem::remove(delme_cnt));
            std::cerr << " ok\n";
        }
        catch(const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
    }
}


TEST_CASE("write without close", "[correct]") {
    input_txt input;
	std::string fname{ input.next() };
	if (fname.empty()) {
        return;
    }

    std::filesystem::path delme_cnt{ "delme.cnt" };
    std::vector<tagged_file> loose_files;

    cnt_reader_reflib_riff r_riff{ fname };
    const measurement_count ch{ 1 };

    {
        cnt_writer_reflib_riff writer{ delme_cnt, RiffType::riff64 };
        writer.recording_info(r_riff.information());
        writer.history(r_riff.history());
        auto* raw3{ writer.add_time_signal(r_riff.param_eeg()) };

        auto sample_count = r_riff.sample_count();
        for (measurement_count i{ 0 }; i < sample_count; ++i) {
            raw3->range_column_major(r_riff.range_column_major(i, ch));
        }
        raw3->triggers(r_riff.triggers());

        loose_files = raw3->file_tokens();
        // omits writer.close();
    }

    cnt_reader_reflib_flat r_flat{ delme_cnt };
    REQUIRE(r_riff.epoch_length() == r_flat.epoch_length());
    REQUIRE(ascii_sampling_frequency(r_riff.sampling_frequency()) == ascii_sampling_frequency(r_flat.sampling_frequency()));
    REQUIRE(r_riff.segment_start_time() == r_flat.segment_start_time());
    REQUIRE(r_riff.channels() == r_flat.channels());
    REQUIRE(r_riff.information() == r_flat.information());

    // buffers might not have been commited to disk
    const auto triggers_riff{ r_riff.triggers() };
    const auto triggers_flat{ r_flat.triggers() };
    REQUIRE(r_flat.sample_count() <= r_riff.sample_count());
    REQUIRE(triggers_flat.size() <= triggers_riff.size());
    std::ostringstream oss_samples, oss_triggers;
    oss_samples << "input samples " << r_riff.sample_count() << ", output samples " << r_flat.sample_count();
    oss_triggers << "input triggers " << triggers_riff.size() << ", output triggers " << triggers_flat.size();

    for (measurement_count i{ 0 }; i < r_flat.sample_count(); ++i) {
        REQUIRE(r_riff.range_column_major(i, ch) == r_flat.range_column_major(i, ch));
    }

    for (size_t i{ 0 }; i < triggers_flat.size(); ++i) {
        REQUIRE(triggers_riff[i] == triggers_flat[i]);
    }

    std::vector<std::filesystem::path> fnames(loose_files.size());
    std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
    delete_files(fnames);
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
