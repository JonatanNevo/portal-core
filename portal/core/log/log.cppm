//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#define SPDLOG_SOURCE_LOCATION 1
#include <spdlog/spdlog.h>

#include "portal/core/core_includes.h"
#include "portal/core/log/types.h"

export module portal.core.log;
export import :formatters;

import std;

namespace portal::detail
{
template<typename... Args>
struct log_format_string // NOLINT (misc-use-internal-linkage)
{
    spdlog::format_string_t<Args...> fmt;
    std::source_location loc;

    template<typename T>
        requires std::convertible_to<const T&, spdlog::format_string_t<Args...>>
    constexpr log_format_string(const T& s, const std::source_location l = std::source_location::current()) :
        fmt(s), // NOLINT (cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        loc(l)
    {}

    // Nested alias: makes this a non-deduced context (like fmt::format_string's `::t` indirection),
    // so Args is deduced from the trailing Args&&... pack instead of failing to match this type directly.
    using type = log_format_string;
};

template<typename... Args>
using log_format_string_t = log_format_string<Args...>::type;
} // namespace portal::detail


export namespace portal::log
{
// Exporting defined from types.h
using log::LOG_LEVEL_ENV_VAR; // NOLINT(misc-unused-using-decls)
using log::LogLevel;

class Logger : public spdlog::logger, public std::enable_shared_from_this<Logger>
{
public:
    explicit Logger(const std::string& name);
    Logger(const std::string& name, spdlog::sink_ptr sink_ptr);

    template<typename... Args>
    PORTAL_FORCE_INLINE void log(LogLevel level, detail::log_format_string_t<Args...> format, Args&&... args)
    {
        // spdlog v2 when? :(
        const spdlog::source_loc spdlog_loc{
                format.loc.file_name(),
                static_cast<int>(format.loc.line()),
                format.loc.function_name()
        };

        spdlog::logger::log(
                spdlog_loc,
                static_cast<spdlog::level::level_enum>(level),
                std::move(format.fmt),
                std::forward<Args>(args)...
        );
    }

    template<typename T>
    PORTAL_FORCE_INLINE void log(
            LogLevel level,
            const T& message,
            const std::source_location loc = std::source_location::current()
    )
    {
        // spdlog v2 when? :(
        const spdlog::source_loc spdlog_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()};

        spdlog::logger::log(
                spdlog_loc,
                static_cast<spdlog::level::level_enum>(level),
                message // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        );
    }

    template<typename... Args>
    PORTAL_FORCE_INLINE void trace(detail::log_format_string_t<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Trace, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    PORTAL_FORCE_INLINE void debug(detail::log_format_string_t<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Debug, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    PORTAL_FORCE_INLINE void info(detail::log_format_string_t<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Info, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    PORTAL_FORCE_INLINE void warn(detail::log_format_string_t<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Warn, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    PORTAL_FORCE_INLINE void error(detail::log_format_string_t<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Error, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    PORTAL_FORCE_INLINE void fatal(detail::log_format_string_t<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Fatal, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename T>
    PORTAL_FORCE_INLINE void trace(const T& msg, const std::source_location loc = std::source_location::current())
    {
        log(LogLevel::Trace, msg, loc);
    }

    template<typename T>
    PORTAL_FORCE_INLINE void debug(const T& msg, const std::source_location loc = std::source_location::current())
    {
        log(LogLevel::Debug, msg, loc);
    }

    template<typename T>
    PORTAL_FORCE_INLINE void info(const T& msg, const std::source_location loc = std::source_location::current())
    {
        log(LogLevel::Info, msg, loc);
    }

    template<typename T>
    PORTAL_FORCE_INLINE void warn(const T& msg, const std::source_location loc = std::source_location::current())
    {
        log(LogLevel::Warn, msg, loc);
    }

    template<typename T>
    PORTAL_FORCE_INLINE void error(const T& msg, const std::source_location loc = std::source_location::current())
    {
        log(LogLevel::Error, msg, loc);
    }

    template<typename T>
    PORTAL_FORCE_INLINE void fatal(const T& msg, const std::source_location loc = std::source_location::current())
    {
        log(LogLevel::Fatal, msg, loc);
    }

    void enable(bool enabled);

    void set_level(LogLevel level);

private:
    LogLevel prev_level = LogLevel::Off;
};

constexpr std::string_view DEFAULT_LOGGER_NAME = "default";

struct Config
{
    LogLevel default_log_level = LogLevel::Trace;
    std::string_view default_logger_name = DEFAULT_LOGGER_NAME;
    std::string_view application_name;
};

void init();
void init(const Config& config);
void shutdown();

Logger& get_logger(const std::string& logger_name);
Logger& get_default_logger();

void set_default_log_level(LogLevel level, bool apply_to_all = true);

bool has_logger(const std::string& logger_name);
void set_logger_level(const std::string& logger_name, LogLevel level);
void enable_logger(const std::string& logger_name, bool enable = true);
} // namespace portal::log
