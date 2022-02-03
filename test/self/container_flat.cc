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

TEST_CASE("read/write flat files - compressed epochs", "[consistency]") {
    const size_t fname_width{ 20 };
    const std::filesystem::path temp_cnt_file{ "delme.cnt" };

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
        std::vector<tagged_file> loose_files;

        const auto get_fname = [](const auto& x) -> std::filesystem::path { return x.file_name; };

		try {
			std::cerr << s2s(fname, fname_width);

            epoch_reader_riff reader{ fname };
            const auto reflib_count{ reader.data().count() };

            // scope for the epoch writer
            {
                // REMEMBER MISSING: reader.data().channel_order()
                epoch_writer_flat flat_writer{ temp_cnt_file, reader.data().description(), reader.data().cnt_type() };
                flat_writer.info(reader.data().information());
                flat_writer.history(reader.data().history());
                flat_writer.append(reader.data().triggers());

                for (epoch_count i{ 0 }; i < reflib_count; ++i) {
                    flat_writer.append(reader.data().epoch(i));
                }

                loose_files = flat_writer.file_tokens();
            }

            // scope for the epoch reader (if the file is still open delete_flat_files fails on windows)
            {
                epoch_reader_flat flat_reader{ temp_cnt_file, loose_files };
                const auto flat_count{ flat_reader.data().count() };
                REQUIRE(flat_count == reflib_count);
                REQUIRE(reader.data().description() == flat_reader.data().description());
                REQUIRE(reader.data().history() == flat_reader.data().history());
                REQUIRE(reader.data().triggers() == flat_reader.data().triggers());
                const auto[start_time, information, is_ascii]{ parse_info(flat_reader.data().info_content()) };
                REQUIRE(reader.data().information() == information);

                for (epoch_count i{ 0 }; i < flat_count; ++i) {
                    REQUIRE(reader.data().epoch(i) == flat_reader.data().epoch(i));
                }
            }

            std::vector<std::filesystem::path> fnames(loose_files.size());
            std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
            REQUIRE(delete_files(fnames));
            std::cerr << " ok\n";
        }
        catch (const std::exception&) {
            ignore_expected();
            std::vector<std::filesystem::path> fnames(loose_files.size());
            std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
            REQUIRE(delete_files(fnames));
        }

        fname = input.next();
    }
}


auto write_in_chunks(cnt_reader_reflib_riff& reader_reflib, const std::filesystem::path& fname, sint chunk_size) -> std::vector<tagged_file> {
    // REMEMBER MISSING: reader_reflib.channel_order()
    cnt_writer_reflib_flat flat_writer{ fname, reader_reflib.description(), RiffType::riff64 };
    flat_writer.info(reader_reflib.information());
    flat_writer.history(reader_reflib.history());

    const sint sample_count{ reader_reflib.sample_count() };
    for (sint i{ 0 }; i < sample_count; i += chunk_size) {
        const sint amount{ std::min(chunk_size, sint(sample_count - i)) };
        const measurement_count epoch_length{ amount };
        if (amount < 0) {
            throw ctk_bug("test: write_in_chunks: negative amount");
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
    const std::filesystem::path temp_cnt_file{ "delme.cnt" };

    input_txt input;
	std::string fname{ input.next() };
	while (!fname.empty()) {
        std::vector<tagged_file> loose_files;
        const auto get_fname = [](const auto& x) -> std::filesystem::path { return x.file_name; };

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
                    loose_files = write_in_chunks(reader_reflib, temp_cnt_file, stride);

                    epoch_reader_riff reflib_reader{ fname };
                    const auto reflib_count{ reflib_reader.data().count() };

                    epoch_reader_flat flat_reader{ fname, loose_files };
                    const auto flat_count{ flat_reader.data().count() };
                    REQUIRE(flat_count == reflib_count);
                    const auto [start_time, information, is_ascii] { parse_info(flat_reader.data().info_content()) };
                    REQUIRE(reflib_reader.data().information() == information);

                    matrix_decoder_reflib decode;
                    decode.row_count(reflib_reader.data().channel_count());

                    for (epoch_count i{ 0 }; i < flat_reader.data().count(); ++i) {
                        const auto rce{ reflib_reader.data().epoch(i) };
                        const auto fce{ flat_reader.data().epoch(i) };

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
                REQUIRE(delete_files(fnames));
                fnames.clear();
            }

            std::cerr << " ok\n";
        }
        catch (const std::exception&) {
            ignore_expected();
            std::vector<std::filesystem::path> fnames(loose_files.size());
            std::transform(begin(loose_files), end(loose_files), begin(fnames), get_fname);
            REQUIRE(delete_files(fnames));
        }

        fname = input.next();
    }
}

TEST_CASE("cnt_writer_reflib_riff", "[consistency]") {
    constexpr const size_t fname_width{ 20 };
    const std::filesystem::path temp_cnt_file{ "delme.cnt" };

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
                    cnt_writer_reflib_riff writer{ temp_cnt_file, RiffType::riff64 }; // TODO: both 32/64
                    writer.recording_info(r_orig.information());
                    writer.history(r_orig.history());
                    // REMEMBER MISSING: r_orig.channel_order()
                    auto* raw3{ writer.add_time_signal(r_orig.description()) };

                    sample_count = r_orig.sample_count();
                    for (measurement_count i{ 0 }; i < sample_count; ++i) {
                        raw3->range_column_major(r_orig.range_column_major(i, ch));
                    }
                    raw3->triggers(r_orig.triggers());

                    writer.close();
                }

                cnt_reader_reflib_riff r_temp{ temp_cnt_file };
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

            REQUIRE(std::filesystem::remove(temp_cnt_file));
            std::cerr << " ok\n";
        }
        catch(const std::exception&) {
            ignore_expected();
        }

        fname = input.next();
    }
}


} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
