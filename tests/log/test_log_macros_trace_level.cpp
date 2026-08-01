//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

#define SPDLOG_SOURCE_LOCATION 1
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>

#define PORTAL_ACTIVE_LOG_LEVEL PORTAL_LOG_LEVEL_TRACE
#include "portal/core/log_macros.h"

import portal.core;

import std;

using portal::log::LogLevel;

// PORTAL_ACTIVE_LOG_LEVEL is forced to Trace for this translation unit, so every LOG_*_NAMED
// macro (down to Trace) must be compiled in, unlike the default Info threshold covered
// in test_log_macros.cpp.
SCENARIO("Lowering PORTAL_ACTIVE_LOG_LEVEL to Trace compiles in every LOG_*_NAMED macro", "[unit][core][log][macros]")
{
    GIVEN("a named logger capturing to a stream")
    {
        std::ostringstream stream;
        auto& logger = portal::log::get_logger("test_log_macros_trace_level");
        const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
        logger.sinks() = {sink};
        logger.set_pattern("%v");
        logger.set_level(LogLevel::Trace);

        THEN("LOG_TRACE_NAMED logs")
        {
            LOG_TRACE_NAMED("test_log_macros_trace_level", "trace");
            logger.flush();
            REQUIRE(stream.str() == "trace\n");
        }

        THEN("LOG_DEBUG_NAMED logs")
        {
            LOG_DEBUG_NAMED("test_log_macros_trace_level", "debug");
            logger.flush();
            REQUIRE(stream.str() == "debug\n");
        }
    }
}