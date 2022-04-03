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

#include "logger.h"

#include <algorithm>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#ifdef _WIN32
#include "spdlog/sinks/msvc_sink.h"
#endif

#include "spdlog/fmt/bundled/core.h"
#include "spdlog/fmt/bundled/chrono.h"

#include "exception.h"


namespace ctk { namespace impl {

    // in case the enumeration in the implementation changes
    static constexpr const log_level levels_ctk[] {
        log_level::trace,
        log_level::debug,
        log_level::info,
        log_level::warning,
        log_level::error,
        log_level::critical,
        log_level::off
    };
    static constexpr const spdlog::level::level_enum levels_spd[] {
        spdlog::level::level_enum::trace,
        spdlog::level::level_enum::debug,
        spdlog::level::level_enum::info,
        spdlog::level::level_enum::warn,
        spdlog::level::level_enum::err,
        spdlog::level::level_enum::critical,
        spdlog::level::level_enum::off
    };
    static constexpr const char* levels_str[] {
        "trace",
        "debug",
        "info",
        "warning",
        "error",
        "critical",
        "off"
    };


    static
    auto enum2level(log_level x) -> spdlog::level::level_enum {
        constexpr const size_t size_ctk{ sizeof(levels_ctk) / sizeof(levels_ctk[0]) };
        constexpr const size_t size_spd{ sizeof(levels_spd) / sizeof(levels_spd[0]) };
        static_assert(size_ctk == size_spd);

        constexpr const auto first{ levels_ctk };
        constexpr const auto last{ levels_ctk + size_ctk };
        const auto i{ std::find(first, last, x) };
        assert(i != last);

        return levels_spd[std::distance(first, i)];
    }

    static
    auto str2level(const std::string& x) -> log_level {
        constexpr const size_t size_str{ sizeof(levels_str) / sizeof(levels_str[0]) };
        constexpr const size_t size_ctk{ sizeof(levels_ctk) / sizeof(levels_ctk[0]) };
        static_assert(size_str == size_ctk);

        constexpr const auto first{ levels_str };
        constexpr const auto last{ levels_str + size_str };
        const auto i{ std::find(first, last, x) };
        if (i == last) {
            std::ostringstream oss;
            oss << "[str2level, logger] '" << x << "' invalid, expected one of";
            for (const auto& str : levels_str) {
                oss << " '" << str << "'";
            }
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw ctk::api::v1::CtkLimit{ e };
        }

        return levels_ctk[std::distance(first, i)];
    }

    static
    auto str2type(const std::string& x) -> log_type {
        constexpr const char* types_str[] { "console", "file", "visual studio" };
        constexpr const log_type types_enum[] { log_type::console, log_type::file, log_type::visual_studio };

        constexpr const size_t size_str{ sizeof(types_str) / sizeof(types_str[0]) };
        constexpr const size_t size_type{ sizeof(types_enum) / sizeof(types_enum[0]) };
        static_assert(size_str == size_type);

        const auto first{ types_str };
        const auto last{ types_str + size_str };
        const auto i{ std::find(first, last, x) };
        if (i == last) {
            std::ostringstream oss;
            oss << "[str2type, logger] '" << x << "' invalid, expected one of";
            for (const auto& str : types_str) {
                oss << " '" << str << "'";
            }
            const auto e{ oss.str() };
            ctk_log_error(e);
            throw ctk::api::v1::CtkLimit{ e };
        }

        return types_enum[std::distance(first, i)];
    }


    auto set_logger(log_type kind, log_level level) -> void {
        switch (kind)
        {
        case log_type::console: {
            auto log{ spdlog::stdout_color_st("ctk") };
            spdlog::set_default_logger(log);
            spdlog::set_level(enum2level(level));
            spdlog::info("Console logger, level {}", levels_str[unsigned(level)]);
            break;
        }
        case log_type::file: {
            auto log{ spdlog::daily_logger_st("ctk", "logs/ctk.txt", 0, 0) };
            spdlog::set_default_logger(log);
            spdlog::set_level(enum2level(level));
            spdlog::info("Daily logger, changes the file at 00:00 am, level {}", levels_str[unsigned(level)]);
            break;
        }
        case log_type::visual_studio: {
#ifdef _WIN32
            auto sink{ std::make_shared<spdlog::sinks::msvc_sink_st>() };
            auto log{ std::make_shared<spdlog::logger>("ctk", sink) };
            spdlog::set_default_logger(log);
            spdlog::set_level(enum2level(level));
            spdlog::info("Visual studio logger, level {}", levels_str[unsigned(level)]);
#else
            auto log{ spdlog::stderr_color_st("ctk") };
            spdlog::set_default_logger(log);
            spdlog::set_level(enum2level(level));
            spdlog::info("Console logger, level {}", levels_str[unsigned(level)]);
#endif
        }
        }
    }

    auto set_logger(const std::string& kind, const std::string& level) -> void {
        set_logger(str2type(kind), str2level(level));
    }

    auto ctk_log_trace(const std::string& x) -> void {
        spdlog::trace(x);
    }

    auto ctk_log_debug(const std::string& x) -> void {
        spdlog::debug(x);
    }

    auto ctk_log_info(const std::string& x) -> void {
        spdlog::info(x);
    }

    auto ctk_log_warning(const std::string& x) -> void {
        spdlog::warn(x);
    }

    auto ctk_log_error(const std::string& x) -> void {
        spdlog::error(x);
    }

    auto ctk_log_critical(const std::string& x) -> void {
        spdlog::critical(x);
    }

    scoped_log::scoped_log(log_type kind, log_level level) {
        // TODO: is it possible to obtain the current default logger?
        set_logger(kind, level);
    }

    scoped_log::~scoped_log() {
        spdlog::info("End logger");
        spdlog::set_default_logger(spdlog::stdout_color_st("console"));
        spdlog::drop("ctk");
    }


} /* namespace impl */ } /* namespace ctk */

