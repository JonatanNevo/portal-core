//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#define PORTAL_LEVEL_TRACE 0
#define PORTAL_LEVEL_DEBUG 1
#define PORTAL_LEVEL_INFO  2
#define PORTAL_LEVEL_WARN  3
#define PORTAL_LEVEL_ERROR 4
#define PORTAL_LEVEL_FATAL 5
#define PORTAL_LEVEL_OFF   6

#if !defined(PORTAL_ACTIVE_LEVEL)
#define PORTAL_ACTIVE_LEVEL PORTAL_LEVEL_INFO
#endif

namespace portal::log
{
constexpr auto ACTIVE_LOG_LEVEL = static_cast<LogLevel>(PORTAL_ACTIVE_LEVEL);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Named logs (prefer these!)                                                                                       //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PORTAL_LOG_IF_LEVEL_ENABLED(level, name, ...)                                     \
    if constexpr (::portal::log::LogLevel::level >= ::portal::log::ACTIVE_LOG_LEVEL)      \
    {                                                                                     \
        ::portal::log::get_logger(name).log(::portal::log::LogLevel::level, __VA_ARGS__); \
    }

#define LOG_TRACE_NAMED(name, ...) PORTAL_LOG_IF_LEVEL_ENABLED(Trace, name, __VA_ARGS__)
#define LOG_DEBUG_NAMED(name, ...) PORTAL_LOG_IF_LEVEL_ENABLED(Debug, name, __VA_ARGS__)
#define LOG_INFO_NAMED(name, ...)  PORTAL_LOG_IF_LEVEL_ENABLED(Info,  name, __VA_ARGS__)
#define LOG_WARN_NAMED(name, ...)  PORTAL_LOG_IF_LEVEL_ENABLED(Warn,  name, __VA_ARGS__)
#define LOG_ERROR_NAMED(name, ...) PORTAL_LOG_IF_LEVEL_ENABLED(Error, name, __VA_ARGS__)
#define LOG_FATAL_NAMED(name, ...) PORTAL_LOG_IF_LEVEL_ENABLED(Fatal, name, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Default logs                                                                                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PORTAL_LOG_IF_LEVEL_ENABLED_DEFAULT(level, ...)                                       \
    if constexpr (::portal::log::LogLevel::level >= ::portal::log::ACTIVE_LOG_LEVEL)          \
    {                                                                                         \
        ::spdlog::default_logger()->log(spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }, static_cast<::spdlog::level::level_enum>(::portal::log::LogLevel::level), __VA_ARGS__); \
    }

#define LOG_TRACE(...) PORTAL_LOG_IF_LEVEL_ENABLED(Trace, "default", __VA_ARGS__)
#define LOG_DEBUG(...) PORTAL_LOG_IF_LEVEL_ENABLED(Debug, "default", __VA_ARGS__)
#define LOG_INFO(...)  PORTAL_LOG_IF_LEVEL_ENABLED(Info,  "default", __VA_ARGS__)
#define LOG_WARN(...)  PORTAL_LOG_IF_LEVEL_ENABLED(Warn,  "default", __VA_ARGS__)
#define LOG_ERROR(...) PORTAL_LOG_IF_LEVEL_ENABLED(Error, "default", __VA_ARGS__)
#define LOG_FATAL(...) PORTAL_LOG_IF_LEVEL_ENABLED(Fatal, "default", __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logger logs (expects a logger named "logger" to be available in the context                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PORTAL_LOGGER_IF_LEVEL_ENABLED(level, ...)                                   \
    if constexpr (::portal::log::LogLevel::level >= ::portal::log::ACTIVE_LOG_LEVEL) \
    {                                                                                \
        logger.log(::portal::log::LogLevel::level, __VA_ARGS__);                     \
    }

#define LOGGER_TRACE(...) PORTAL_LOGGER_IF_LEVEL_ENABLED(Trace, __VA_ARGS__)
#define LOGGER_DEBUG(...) PORTAL_LOGGER_IF_LEVEL_ENABLED(Debug, __VA_ARGS__)
#define LOGGER_INFO(...)  PORTAL_LOGGER_IF_LEVEL_ENABLED(Info,  __VA_ARGS__)
#define LOGGER_WARN(...)  PORTAL_LOGGER_IF_LEVEL_ENABLED(Warn,  __VA_ARGS__)
#define LOGGER_ERROR(...) PORTAL_LOGGER_IF_LEVEL_ENABLED(Error, __VA_ARGS__)
#define LOGGER_FATAL(...) PORTAL_LOGGER_IF_LEVEL_ENABLED(Fatal, __VA_ARGS__)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
