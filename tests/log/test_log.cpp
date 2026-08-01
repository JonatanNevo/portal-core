//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

#define SPDLOG_SOURCE_LOCATION 1
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ostream_sink.h>

import portal.core;
import portal.core.log.platform;
import std;

// Captures `call`'s output and the physical line it executed on, in one statement, so the
// line reported here always matches the line where the logging call itself appears.
#define CAPTURE_LOCATION(call) \
    [&]() -> std::pair<std::string, int> { stream.str(""); call; logger.flush(); return {stream.str(), __LINE__}; }()

using portal::log::LogLevel;

SCENARIO("Logger writes formatted messages through its sink", "[unit][core][log]")
{
    GIVEN("a Logger backed by an ostream sink")
    {
        std::ostringstream stream;
        const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
        portal::log::Logger logger("test_log_ostream", sink);
        logger.set_level(LogLevel::Trace);
        logger.set_pattern("%v");

        THEN("a formatted message is written to the sink")
        {
            logger.info("hello {}", 42);
            logger.flush();
            REQUIRE(stream.str() == "hello 42\n");
        }

        THEN("a plain message is written to the sink")
        {
            logger.warn(std::string("plain message"));
            logger.flush();
            REQUIRE(stream.str() == "plain message\n");
        }

        THEN("messages below the configured level are dropped")
        {
            logger.set_level(LogLevel::Error);
            logger.info("should not appear");
            logger.flush();
            REQUIRE(stream.str().empty());
        }
    }
}

SCENARIO("Logger::enable mutes and restores the previous level", "[unit][core][log]")
{
    GIVEN("a Logger at a known level")
    {
        portal::log::Logger logger("test_log_enable_member");
        logger.set_level(LogLevel::Warn);

        THEN("enable(true) mutes the logger")
        {
            logger.enable(true);
            REQUIRE(logger.level() == spdlog::level::off);

            AND_THEN("enable(false) restores the previous level")
            {
                logger.enable(false);
                REQUIRE(logger.level() == spdlog::level::warn);
            }
        }
    }
}

SCENARIO("get_logger registers and caches loggers by name", "[unit][core][log]")
{
    GIVEN("a unique logger name")
    {
        const std::string name = "test_log_get_logger_cache";

        THEN("repeated lookups return the same instance")
        {
            auto& first = portal::log::get_logger(name);
            auto& second = portal::log::get_logger(name);

            REQUIRE(&first == &second);
            REQUIRE(first.name() == name);
        }
    }

    GIVEN("two distinct logger names")
    {
        THEN("get_logger creates distinct instances")
        {
            auto& a = portal::log::get_logger("test_log_get_logger_a");
            auto& b = portal::log::get_logger("test_log_get_logger_b");

            REQUIRE(&a != &b);
            REQUIRE(a.name() != b.name());
        }
    }
}

SCENARIO("set_logger_level changes the level of a registered logger", "[unit][core][log]")
{
    GIVEN("a registered logger")
    {
        const std::string name = "test_log_set_logger_level";
        auto& logger = portal::log::get_logger(name);
        logger.set_level(LogLevel::Trace);

        THEN("set_logger_level updates its level")
        {
            portal::log::set_logger_level(name, LogLevel::Warn);
            REQUIRE(logger.level() == spdlog::level::warn);
        }
    }

    GIVEN("an unregistered logger name")
    {
        THEN("set_logger_level does not throw")
        {
            REQUIRE_NOTHROW(portal::log::set_logger_level("test_log_does_not_exist", LogLevel::Error));
        }
    }
}

SCENARIO("set_default_log_level(apply_to_all=false) only changes the default logger", "[unit][core][log]")
{
    GIVEN("a registered logger untouched by the call")
    {
        const std::string name = "test_log_default_level_scoped";
        auto& logger = portal::log::get_logger(name);
        logger.set_level(LogLevel::Trace);

        THEN("the default logger's level changes but the named logger's does not")
        {
            portal::log::set_default_log_level(LogLevel::Error, false);

            REQUIRE(spdlog::default_logger()->level() == spdlog::level::err);
            REQUIRE(logger.level() == spdlog::level::trace);
        }
    }
}

SCENARIO("LogLevel values map onto the underlying spdlog levels", "[unit][core][log]")
{
    THEN("each LogLevel casts to its matching spdlog::level::level_enum")
    {
        REQUIRE(static_cast<spdlog::level::level_enum>(LogLevel::Trace) == spdlog::level::trace);
        REQUIRE(static_cast<spdlog::level::level_enum>(LogLevel::Debug) == spdlog::level::debug);
        REQUIRE(static_cast<spdlog::level::level_enum>(LogLevel::Info) == spdlog::level::info);
        REQUIRE(static_cast<spdlog::level::level_enum>(LogLevel::Warn) == spdlog::level::warn);
        REQUIRE(static_cast<spdlog::level::level_enum>(LogLevel::Error) == spdlog::level::err);
        REQUIRE(static_cast<spdlog::level::level_enum>(LogLevel::Fatal) == spdlog::level::critical);
        REQUIRE(static_cast<spdlog::level::level_enum>(LogLevel::Off) == spdlog::level::off);
    }
}

SCENARIO("Logger captures the caller's source_location for every level and log() overload", "[unit][core][log]")
{
    GIVEN("a Logger whose pattern renders only the captured source_location")
    {
        std::ostringstream stream;
        const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
        portal::log::Logger logger("test_log_source_location", sink);
        logger.set_level(LogLevel::Trace);

        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<portal::source_location_flag_formatter>('*', 1);
        formatter->set_pattern("%*");
        logger.set_formatter(std::move(formatter));

        const auto expect_here = [](const std::pair<std::string, int>& result)
        {
            INFO("captured location: " << result.first);
            REQUIRE_THAT(result.first, Catch::Matchers::ContainsSubstring("test_log.cpp:" + std::to_string(result.second)));
        };

        THEN("the message-only overload captures the call site for every level")
        {
            expect_here(CAPTURE_LOCATION(logger.trace("m")));
            expect_here(CAPTURE_LOCATION(logger.debug("m")));
            expect_here(CAPTURE_LOCATION(logger.info("m")));
            expect_here(CAPTURE_LOCATION(logger.warn("m")));
            expect_here(CAPTURE_LOCATION(logger.error("m")));
            expect_here(CAPTURE_LOCATION(logger.fatal("m")));
        }

        THEN("the format-string overload captures the call site for every level")
        {
            expect_here(CAPTURE_LOCATION(logger.trace("m {}", 1)));
            expect_here(CAPTURE_LOCATION(logger.debug("m {}", 1)));
            expect_here(CAPTURE_LOCATION(logger.info("m {}", 1)));
            expect_here(CAPTURE_LOCATION(logger.warn("m {}", 1)));
            expect_here(CAPTURE_LOCATION(logger.error("m {}", 1)));
            expect_here(CAPTURE_LOCATION(logger.fatal("m {}", 1)));
        }

        THEN("Logger::log() dispatches to the same captured call site as the level helpers")
        {
            expect_here(CAPTURE_LOCATION(logger.log(LogLevel::Info, "m")));
            expect_here(CAPTURE_LOCATION(logger.log(LogLevel::Info, "m {}", 1)));
        }
    }
}

#undef CAPTURE_LOCATION

SCENARIO("init() registers a default logger reachable through get_default_logger", "[unit][core][log]")
{
    GIVEN("logging is initialized with the default Config")
    {
        portal::log::init();

        THEN("the default logger is registered under the default name")
        {
            auto& logger = portal::log::get_default_logger();
            REQUIRE(logger.name() == "default");
            REQUIRE(portal::log::has_logger("default"));
        }

        portal::log::shutdown();
    }
}

SCENARIO("init(Config) applies the configured logger name and default log level", "[unit][core][log]")
{
    GIVEN("a Config with a non-default logger name and level")
    {
        portal::log::Config config;
        config.default_logger_name = "test_log_init_custom";
        config.default_log_level = LogLevel::Error;

        THEN("the created default logger uses that name and level")
        {
            portal::log::init(config);

            auto& logger = portal::log::get_default_logger();
            REQUIRE(logger.name() == "test_log_init_custom");
            REQUIRE(logger.level() == spdlog::level::err);

            portal::log::shutdown();
        }
    }
}

SCENARIO("shutdown() clears the logger registry", "[unit][core][log]")
{
    GIVEN("an initialized environment with an extra named logger")
    {
        portal::log::init();
        portal::log::get_logger("test_log_shutdown_extra");
        REQUIRE(portal::log::has_logger("test_log_shutdown_extra"));

        THEN("shutdown() drops every registered logger, default included")
        {
            portal::log::shutdown();
            REQUIRE_FALSE(portal::log::has_logger("test_log_shutdown_extra"));
            REQUIRE_FALSE(portal::log::has_logger("default"));
        }
    }
}

SCENARIO("has_logger reports whether a logger is registered", "[unit][core][log]")
{
    GIVEN("a freshly registered logger")
    {
        const std::string name = "test_log_has_logger_present";
        portal::log::get_logger(name);

        THEN("has_logger returns true")
        {
            REQUIRE(portal::log::has_logger(name));
        }
    }

    GIVEN("a name that was never registered")
    {
        THEN("has_logger returns false")
        {
            REQUIRE_FALSE(portal::log::has_logger("test_log_has_logger_absent"));
        }
    }
}

SCENARIO("enable_logger mutes and restores a registered logger", "[unit][core][log]")
{
    GIVEN("a registered logger at a known level")
    {
        const std::string name = "test_log_enable_logger";
        auto& logger = portal::log::get_logger(name);
        logger.set_level(LogLevel::Warn);

        THEN("enable_logger(name, true) mutes it")
        {
            portal::log::enable_logger(name, true);
            REQUIRE(logger.level() == spdlog::level::off);

            AND_THEN("enable_logger(name, false) un-mutes it")
            {
                portal::log::enable_logger(name, false);
                REQUIRE(logger.level() != spdlog::level::off);
            }
        }
    }

    GIVEN("a name that was never registered")
    {
        THEN("enable_logger does not throw")
        {
            REQUIRE_NOTHROW(portal::log::enable_logger("test_log_enable_logger_missing", true));
            REQUIRE_NOTHROW(portal::log::enable_logger("test_log_enable_logger_missing", false));
        }
    }
}

SCENARIO("get_platform_sinks returns a shared, non-null distribution sink", "[unit][core][log][platform]")
{
    GIVEN("the platform sink factory")
    {
        THEN("it returns a non-null sink")
        {
            const auto sink = portal::platform::get_platform_sinks(".");
            REQUIRE(sink != nullptr);
        }

        THEN("repeated calls return the same cached instance regardless of the folder argument")
        {
            const auto first = portal::platform::get_platform_sinks(".");
            const auto second = portal::platform::get_platform_sinks("some/other/folder");
            REQUIRE(first == second);
        }
    }
}