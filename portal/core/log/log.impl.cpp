//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#define SPDLOG_SOURCE_LOCATION 1
#include <spdlog/cfg/env.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/spdlog.h>

#include "portal/core/log/types.h"
#include "portal/core/log_macros.h"

module portal.core.log;

import portal.core.concurrency;
import portal.core.log.platform;

namespace portal::log
{
// Format: [date] [#thread_id] [file:line function] [name] colored{[level] message} extra
constexpr auto default_pattern = "[%Y-%m-%d %H:%M:%S.%f] [%t] [%*] [%-12n] %^[%=7l] %v%$ %&";

Logger::Logger(const std::string& name) : spdlog::logger(name)
{}

Logger::Logger(const std::string& name, spdlog::sink_ptr sink_ptr) : spdlog::logger(name, std::move(sink_ptr))
{}

void Logger::enable(const bool enabled)
{
    if (enabled)
    {
        prev_level = static_cast<LogLevel>(spdlog::logger::level());
        spdlog::logger::set_level(static_cast<spdlog::level::level_enum>(LogLevel::Off));
    }
    else
    {
        spdlog::logger::set_level(static_cast<spdlog::level::level_enum>(prev_level));
    }
}

void Logger::set_level(LogLevel level)
{
    logger::set_level(static_cast<spdlog::level::level_enum>(level));
}

void init()
{
    init(Config{});
}

void init(const Config& config)
{
    // Baseline level from config; env var (below) takes precedence when set.
    spdlog::set_level(static_cast<spdlog::level::level_enum>(config.default_log_level));

    // Find log levels in environment variables
    spdlog::cfg::load_env_levels(LOG_LEVEL_ENV_VAR);
    spdlog::set_automatic_registration(true);

    // Create a custom formatter and register the custom flag
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<source_location_flag_formatter>('*', source_location_flag_formatter::DEFAULT_WIDTH);
    formatter->set_pattern(default_pattern);

    spdlog::set_formatter(std::move(formatter));

    auto dist_sink = platform::get_platform_sinks("");

    const auto default_logger = std::make_shared<Logger>(std::string(config.default_logger_name), dist_sink);
    spdlog::initialize_logger(default_logger);
    spdlog::set_default_logger(default_logger);

    LOG_INFO("Logger initialized");
}

void shutdown()
{
    LOG_INFO("Shutting down logger");
    spdlog::apply_all([&](const auto& l) { l->flush(); });
    spdlog::shutdown();
}

Logger& get_logger(const std::string& logger_name)
{
    auto logger = std::dynamic_pointer_cast<Logger>(spdlog::get(logger_name));
    if (logger != nullptr)
        return *logger;

    auto dist_sink = platform::get_platform_sinks("");
    logger = std::make_shared<Logger>(logger_name, dist_sink);
    spdlog::initialize_logger(logger);
    return *logger;
}

Logger& get_default_logger()
{
    const auto logger = std::dynamic_pointer_cast<Logger>(spdlog::default_logger());
    // TODO: assert on empty default logger
    return *logger;
}

void set_default_log_level(LogLevel level, const bool apply_to_all)
{
    if (apply_to_all)
    {
        spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
    }
    else
    {
        spdlog::default_logger()->set_level(static_cast<spdlog::level::level_enum>(level));
    }
}

bool has_logger(const std::string& logger_name)
{
    return spdlog::get(logger_name) != nullptr;
}

void set_logger_level(const std::string& logger_name, LogLevel level)
{
    const auto logger = spdlog::get(logger_name);
    if (logger)
        logger->set_level(static_cast<spdlog::level::level_enum>(level));
}

void enable_logger(const std::string& logger_name, const bool enable)
{
    const auto logger = spdlog::get(logger_name);
    if (logger)
    {
        if (enable)
            logger->set_level(spdlog::level::off);
        else
            spdlog::apply_logger_env_levels(logger);
    }
}
} // namespace portal::log
