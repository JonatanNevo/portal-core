//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <spdlog/spdlog.h>

export module portal.core.log.platform;

import std;

export namespace portal::platform
{
spdlog::sink_ptr get_platform_sinks(const std::filesystem::path& log_folder);
}
