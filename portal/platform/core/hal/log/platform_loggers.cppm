//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <spdlog/spdlog.h>

#include "portal/core/core_includes.h"

export module portal.core.log.platform;

#ifdef PORTAL_PLATFORM_WINDOWS
export import :windows;
#elifdef PORTAL_PLATFORM_MACOS
export import :macos;
#elifdef PORTAL_PLATFORM_LINUX
export import :linux;
#else
#error "Invalid Platform"
#endif

import std;

export namespace portal::platform
{
spdlog::sink_ptr get_platform_sinks(const std::filesystem::path& log_folder);
}
