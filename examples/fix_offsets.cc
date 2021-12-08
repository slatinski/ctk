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

#include <iostream>
#include <filesystem>
#include "ctk.h"


static
auto is_broken(const std::filesystem::path& ifile) -> bool {
    try
    {
        ctk::CntReaderReflib reader{ ifile.string() };
        const int64_t total{ reader.sampleCount() };
        for (int64_t i{ 0 }; i < total; ++i) {
            reader.rangeRowMajor(i, 1);
        }
        reader.triggers();
    }
    catch(const std::exception&) {
        return true;
    }

    return false;
}


static
auto recover(const std::filesystem::path& ifile, const std::filesystem::path& ofile) -> bool {
    try {
        ctk::CntReaderReflib reader{ ifile.string(), true };
        const int64_t total{ reader.sampleCount() };
        int64_t accessible{ 0 };

        ctk::CntWriterReflib writer{ ofile.string(), ctk::RiffType::riff64, reader.history() };

        try {
            writer.addTimeSignal(reader.description());
            writer.recordingInfo(reader.information());
        }
        catch(const std::exception&) {
            writer.close();
            std::cerr << " [header recovery failed]";
            return false;
        }

        bool success{ true };
        try {
            for (int64_t i{ 0 }; i < total; ++i) {
                writer.rangeRowMajor(reader.rangeRowMajor(i, 1));
                ++accessible;
            }
        }
        catch(const std::exception&) {
            const int64_t lost{ total - accessible };
            const double freq{ reader.description().sampling_frequency };
            const double seconds_recovered{ accessible / freq };
            const double seconds_lost{ lost / freq };
            success = lost <= reader.description().epoch_length; // success even if the last epoch is unreadable
            std::cerr << " [eeg recovered " << seconds_recovered << " sec, lost " << seconds_lost << " sec]";
        }

        try {
            writer.triggers(reader.triggers());
            std::cerr << " [triggers recovered]";
        }
        catch(const std::exception&) {
            success = false;
            std::cerr << " [trigger recovery failed]";
        }

        writer.close();
        return success;
    }
    catch(const std::exception&) {
        std::cerr << " [I/O error]";
    }

    return false;
}


static
auto depth_first_copy_files(std::filesystem::path idir, std::filesystem::path odir) -> void {
    using namespace std::filesystem;

    for (const directory_entry& entry : directory_iterator(idir)) {
        const path ifile{ entry.path() };

        if (entry.is_directory()) {
            const path output_dir{ odir/ifile.filename() };
            create_directory(output_dir);

            depth_first_copy_files(ifile, output_dir);
        }
        else if (entry.is_regular_file()) {
            const path ofile{ odir / ifile.filename() };

            std::cerr << ifile;
            if (ifile.extension() != ".cnt") {
                std::cerr << ": verbatim copy\n";
                copy_file(ifile, ofile, copy_options::update_existing);
                continue;
            }

            if (!is_broken(ifile)) {
                std::cerr << ": not broken. verbatim copy\n";
                copy_file(ifile, ofile, copy_options::update_existing);
                continue;
            }

            std::cerr << ": broken";
            if (!recover(ifile, ofile)) {
                std::cerr << " FAILED\n";
            }
            else {
                std::cerr << ": recovered\n";
            }
        }
    }
}


static
auto is_directory_readable(const std::filesystem::perms& x) -> bool {
    using namespace std::filesystem;

    const bool user_readable{ (x & perms::owner_read) != perms::none && (x & perms::owner_exec) != perms::none }; 
    const bool group_readable{ (x & perms::group_read) != perms::none && (x & perms::group_exec) != perms::none }; 
    const bool others_readable{ (x & perms::others_read) != perms::none && (x & perms::others_exec) != perms::none }; 

    return user_readable || group_readable || others_readable;
}


static
auto is_directory_writable(const std::filesystem::perms& x) -> bool {
    using namespace std::filesystem;

    const bool user_writable{ (x & perms::owner_write) != perms::none && (x & perms::owner_exec) != perms::none }; 
    const bool group_writable{ (x & perms::group_write) != perms::none && (x & perms::group_exec) != perms::none }; 
    const bool others_writable{ (x & perms::others_write) != perms::none && (x & perms::others_exec) != perms::none }; 

    return user_writable || group_writable || others_writable;
}


auto main(int argc, char* argv[]) -> int {
    using namespace std::filesystem;

    if (argc < 3) {
        std::cerr << "USAGE: " << argv[0] << " INPUT_DIRECTORY OUTPUT_DIRECTORY\n";
        std::cerr << "INPUT_DIRECTORY  should exist and be readable\n";
        std::cerr << "OUTPUT_DIRECTORY might exist and it should be writable\n";
        std::cerr << "\n" << argc - 1 << " parameters received:\n";
        for (int i{ 1 }; i < argc; ++i) {
            std::cerr << "\t" << argv[i] << "\n";
        }
        return 0;
    }
    const std::string idir{ argv[1] };
    const std::string odir{ argv[2] };

    try {
        const file_status istat{ status(idir) };
        if (!exists(istat)) {
            std::cerr << "Input directory '" << idir << "' does not exist\n";
            return 1;
        }

        if (!is_directory(istat)) {
            std::cerr << "Input '" << idir << "' is not a directory\n";
            return 1;
        }

        if (!is_directory_readable(istat.permissions())) {
            std::cerr << "Input directory '" << idir << "' is not readable\n";
            return 1;
        }

        const file_status ostat{ status(odir) };
        if (exists(ostat)) {
            if (!is_directory(ostat)) {
                std::cerr << "Output '" << odir << "' is not a directory\n";
                return 1;
            }
        }
        else {
            create_directory(odir);
        }

        if (!is_directory_writable(ostat.permissions())) {
            std::cerr << "Output directory '" << odir << "' is not writable\n";
            return 1;
        }

        depth_first_copy_files(idir, odir);
    }
    catch(const std::exception& x) {
        std::cerr << x.what() << "\n";
        return 1;
    }

    return 0;
}


