
//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <cstdint>

// Reflects spdlog levels.
// This might break with newer spdlog versions,
// that's why there is a unit test that checks compatibility

//NOLINTBEGIN (modernize-macro-to-enum)
#define PORTAL_LOG_LEVEL_TRACE 0
#define PORTAL_LOG_LEVEL_DEBUG 1
#define PORTAL_LOG_LEVEL_INFO 2
#define PORTAL_LOG_LEVEL_WARN 3
#define PORTAL_LOG_LEVEL_ERROR 4
#define PORTAL_LOG_LEVEL_FATAL 5
#define PORTAL_LOG_LEVEL_OFF 6
//NOLINTEND (modernize-macro-to-enum)

namespace portal::log
{

inline constexpr auto LOG_LEVEL_ENV_VAR = "PORTAL_LOG_LEVELS";

enum class LogLevel : uint8_t
{
    Trace = PORTAL_LOG_LEVEL_TRACE,
    Debug = PORTAL_LOG_LEVEL_DEBUG,
    Info = PORTAL_LOG_LEVEL_INFO,
    Warn = PORTAL_LOG_LEVEL_WARN,
    Error = PORTAL_LOG_LEVEL_ERROR,
    Fatal = PORTAL_LOG_LEVEL_FATAL,
    Off = PORTAL_LOG_LEVEL_OFF,
};

} // namespace portal::log
