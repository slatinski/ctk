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

#pragma once

#include <string>

namespace ctk { namespace impl {

    enum class log_type{ console, file, visual_studio };
    enum class log_level{ trace, debug, info, warning, error, critical, off };


    // replaces the standard spdlog logger with a single threaded logger named "ctk".
    // console:       stdout_color_st("ctk")
    // file:          daily_logger_st("ctk", "logs/ctk.txt", 0, 0)
    // visual studio: sinks::msvc_sink_st
    //                if !defined(_WIN32): stderr_color_st("ctk")
    // throws spdlog::spdlog_ex on error
    auto set_logger(log_type, log_level) -> void;
    auto set_logger(const std::string&, const std::string&) -> void;


    auto ctk_log_trace(const std::string&) -> void;
    auto ctk_log_debug(const std::string&) -> void;
    auto ctk_log_info(const std::string&) -> void;
    auto ctk_log_warning(const std::string&) -> void;
    auto ctk_log_error(const std::string&) -> void;
    auto ctk_log_critical(const std::string&) -> void;


    // raii for starting/stopping logging with spdlog
    struct scoped_log
    {
        // calls set_logger
        scoped_log(log_type, log_level);
        scoped_log(const std::string&, const std::string&);

        // replaces the standard spdlog logger with a multithreaded console logger named "console"
        // set_default_logger(stdout_color_st("console"));
        ~scoped_log();
    };

} /* namespace impl */ } /* namespace ctk */

