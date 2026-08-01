//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

#define SPDLOG_SOURCE_LOCATION 1
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>

import portal.core;

#include "portal/core/log_macros.h"

import std;

using portal::log::LogLevel;

namespace
{
portal::log::Logger& make_captured_logger(const std::string& name, std::ostringstream& stream)
{
    auto& logger = portal::log::get_logger(name);
    const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
    logger.sinks() = {sink};
    logger.set_pattern("%v");
    logger.set_level(LogLevel::Trace);
    return logger;
}
}

SCENARIO("LOG_*_NAMED macros log through the named logger at the default (Info) compile-time threshold", "[unit][core][log][macros]")
{
    GIVEN("a named logger capturing to a stream")
    {
        std::ostringstream stream;
        auto& logger = make_captured_logger("test_log_macros_named", stream);

        THEN("LOG_TRACE_NAMED and LOG_DEBUG_NAMED are compiled out and emit nothing")
        {
            LOG_TRACE_NAMED("test_log_macros_named", "trace");
            LOG_DEBUG_NAMED("test_log_macros_named", "debug");
            logger.flush();
            REQUIRE(stream.str().empty());
        }

        THEN("LOG_INFO_NAMED logs a formatted message")
        {
            LOG_INFO_NAMED("test_log_macros_named", "info {}", 1);
            logger.flush();
            REQUIRE(stream.str() == "info 1\n");
        }

        THEN("LOG_WARN_NAMED logs a plain message")
        {
            LOG_WARN_NAMED("test_log_macros_named", std::string("warn"));
            logger.flush();
            REQUIRE(stream.str() == "warn\n");
        }

        THEN("LOG_ERROR_NAMED logs a formatted message")
        {
            LOG_ERROR_NAMED("test_log_macros_named", "error {}", 2);
            logger.flush();
            REQUIRE(stream.str() == "error 2\n");
        }

        THEN("LOG_FATAL_NAMED logs a formatted message")
        {
            LOG_FATAL_NAMED("test_log_macros_named", "fatal {}", 3);
            logger.flush();
            REQUIRE(stream.str() == "fatal 3\n");
        }

        THEN("the name argument routes to the same logger returned by get_logger")
        {
            LOG_INFO_NAMED("test_log_macros_named", "routed");
            logger.flush();
            REQUIRE(stream.str() == "routed\n");
            REQUIRE(&portal::log::get_logger("test_log_macros_named") == &logger);
        }
    }

    GIVEN("a named logger whose runtime level is raised above Warn")
    {
        std::ostringstream stream;
        auto& logger = make_captured_logger("test_log_macros_named_runtime_filter", stream);
        logger.set_level(LogLevel::Error);

        THEN("LOG_WARN_NAMED is still compiled in but dropped at runtime")
        {
            LOG_WARN_NAMED("test_log_macros_named_runtime_filter", "warn");
            logger.flush();
            REQUIRE(stream.str().empty());
        }

        THEN("LOG_ERROR_NAMED still passes the runtime filter")
        {
            LOG_ERROR_NAMED("test_log_macros_named_runtime_filter", "error");
            logger.flush();
            REQUIRE(stream.str() == "error\n");
        }
    }
}

SCENARIO("LOG_* macros target the default logger", "[unit][core][log][macros]")
{
    GIVEN("logging lib has been initialized")
    {
        portal::log::init();

        AND_GIVEN("A default logger that points to a stream")
        {
            std::ostringstream stream;
            auto& logger = portal::log::get_default_logger();
            const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
            logger.sinks() = {sink};
            logger.set_pattern("%v");
            logger.set_level(LogLevel::Trace);

            THEN("LOG_TRACE and LOG_DEBUG are compiled out and emit nothing")
            {
                LOG_TRACE("trace");
                LOG_DEBUG("debug");
                logger.flush();
                REQUIRE(stream.str().empty());
            }

            THEN("LOG_INFO logs a formatted message")
            {
                LOG_INFO("info {}", 1);
                logger.flush();
                REQUIRE(stream.str() == "info 1\n");
            }

            THEN("LOG_WARN logs a formatted message")
            {
                LOG_WARN("warn {}", 2);
                logger.flush();
                REQUIRE(stream.str() == "warn 2\n");
            }

            THEN("LOG_ERROR logs a formatted message")
            {
                LOG_ERROR("error {}", 3);
                logger.flush();
                REQUIRE(stream.str() == "error 3\n");
            }

            THEN("LOG_FATAL logs a formatted message")
            {
                LOG_FATAL("fatal {}", 4);
                logger.flush();
                REQUIRE(stream.str() == "fatal 4\n");
            }
        }

        portal::log::shutdown();
    }
}

SCENARIO("LOGGER_* macros log through a local variable named `logger`", "[unit][core][log][macros]")
{
    GIVEN("a local Logger named `logger` capturing to a stream")
    {
        std::ostringstream stream;
        const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
        portal::log::Logger logger("test_log_macros_logger_var", sink);
        logger.set_pattern("%v");
        logger.set_level(LogLevel::Trace);

        THEN("LOGGER_TRACE and LOGGER_DEBUG are compiled out and emit nothing")
        {
            LOGGER_TRACE("trace");
            LOGGER_DEBUG("debug");
            logger.flush();
            REQUIRE(stream.str().empty());
        }

        THEN("LOGGER_INFO logs a formatted message")
        {
            LOGGER_INFO("info {}", 1);
            logger.flush();
            REQUIRE(stream.str() == "info 1\n");
        }

        THEN("LOGGER_WARN logs a plain message")
        {
            LOGGER_WARN(std::string("warn"));
            logger.flush();
            REQUIRE(stream.str() == "warn\n");
        }

        THEN("LOGGER_ERROR logs a formatted message")
        {
            LOGGER_ERROR("error {}", 2);
            logger.flush();
            REQUIRE(stream.str() == "error 2\n");
        }

        THEN("LOGGER_FATAL logs a formatted message")
        {
            LOGGER_FATAL("fatal {}", 3);
            logger.flush();
            REQUIRE(stream.str() == "fatal 3\n");
        }
    }
}