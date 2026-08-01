//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

#define SPDLOG_SOURCE_LOCATION 1
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>

#define PORTAL_ACTIVE_LOG_LEVEL PORTAL_LOG_LEVEL_OFF
#include "portal/core/log_macros.h"

import portal.core;

import std;

using portal::log::LogLevel;

// PORTAL_ACTIVE_LOG_LEVEL is forced to Off for this translation unit, so even LOG_FATAL_NAMED
// must be compiled out.
SCENARIO("Raising PORTAL_ACTIVE_LEVEL to Off compiles out every LOG_*_NAMED macro, including Fatal", "[unit][core][log][macros]")
{
    GIVEN("a named logger capturing to a stream, with its runtime level left at Trace")
    {
        std::ostringstream stream;
        auto& logger = portal::log::get_logger("test_log_macros_off_level");
        const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
        logger.sinks() = {sink};
        logger.set_pattern("%v");
        logger.set_level(LogLevel::Trace);

        THEN("LOG_FATAL_NAMED emits nothing, proving the gate is a compile-time check, not a runtime one")
        {
            LOG_TRACE_NAMED("test_log_macros_off_level", "trace");
            LOG_DEBUG_NAMED("test_log_macros_off_level", "debug");
            LOG_INFO_NAMED("test_log_macros_off_level", "info");
            LOG_WARN_NAMED("test_log_macros_off_level", "warn");
            LOG_ERROR_NAMED("test_log_macros_off_level", "error");
            LOG_FATAL_NAMED("test_log_macros_off_level", "fatal");
            logger.flush();
            const auto res = stream.str();
            INFO(res);
            REQUIRE(res.empty());
        }
    }
}