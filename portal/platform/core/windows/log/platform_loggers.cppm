//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dist_sink.h>

export module portal.core.log.platform:windows;

import std;

export namespace portal::platform
{
spdlog::sink_ptr get_platform_sinks(const std::filesystem::path& log_folder)
{
    static spdlog::sink_ptr dist_sink;
    static std::vector<spdlog::sink_ptr> sinks;
    if (sinks.empty() || dist_sink == nullptr)
    {
        sinks = {
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
            std::make_shared<spdlog::sinks::basic_file_sink_mt>((log_folder / "portal.log").generic_string(), true)
        };

        dist_sink = std::make_shared<spdlog::sinks::dist_sink_mt>(sinks);
    }

    return dist_sink;
}
}