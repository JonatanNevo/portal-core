//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "portal/core/log_macros.h"

module portal.core.os.platform;

import std;
import portal.core.log;

namespace portal::os
{
static auto& logger = log::get_logger("portal.os.linux"); // NOLINT (cppcoreguidelines-avoid-non-const-global-variables)

namespace
{

    bool default_run_process(const ProcessCommand& command)
    {
        std::vector<const char*> argv;
        argv.reserve(command.arguments.size() + 2);

        argv.push_back(command.executable.c_str());
        for (const std::string& argument: command.arguments)
        {
            argv.push_back(argument.c_str());
        }
        argv.push_back(nullptr);

        pid_t pid{};
        const int spawn_result = posix_spawnp(
                &pid,
                command.executable.c_str(),
                nullptr,
                nullptr,
                const_cast<char* const*>(argv.data()), // NOLINT (cppcoreguidelines-pro-type-const-cast)
                environ
        );
        if (spawn_result != 0)
        {
            return false;
        }

        int status{};
        if (waitpid(pid, &status, 0) == -1)
        {
            return false;
        }

        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    // NOLINTNEXTLINE (cppcoreguidelines-avoid-non-const-global-variables)
    ProcessRunner g_process_runner;

    std::string to_file_uri(const std::filesystem::path& path)
    {
        return std::format("file://{}", path.string());
    }

    // Resolves `path` to an absolute path, logging and returning nullopt on failure.
    std::optional<std::filesystem::path> resolve_absolute(const std::filesystem::path& path)
    {
        if (path.empty())
        {
            LOGGER_WARN("Cannot open an empty path")
            return std::nullopt;
        }

        std::error_code error;
        std::filesystem::path absolute_path = std::filesystem::absolute(path, error);
        if (error)
        {
            LOGGER_WARN("Error in resolving absolute path for path '{}': {}", path.string(), error.message());
            return std::nullopt;
        }
        return absolute_path;
    }

} // namespace

bool run_process(const ProcessCommand& command)
{
    return g_process_runner ? g_process_runner(command) : default_run_process(command);
}

void set_process_runner(ProcessRunner runner)
{
    g_process_runner = std::move(runner);
}

bool open_externally(const std::filesystem::path& path)
{
    const auto absolute_path = resolve_absolute(path);
    if (!absolute_path)
    {
        return false;
    }

    return run_process({.executable = "xdg-open", .arguments = {absolute_path->string()}});
}

bool open_directory_in_explorer(const std::filesystem::path& path)
{
    const auto absolute_path = resolve_absolute(path);
    if (!absolute_path)
    {
        return false;
    }

    std::error_code error;
    if (!std::filesystem::is_directory(*absolute_path, error))
    {
        LOGGER_WARN("File '{}' is not a directory, cannot show in explorer", absolute_path->string());
        return false;
    }

    return run_process({.executable = "xdg-open", .arguments = {absolute_path->string()}});
}

bool show_file_in_explorer(const std::filesystem::path& path)
{
    const auto absolute_path = resolve_absolute(path);
    if (!absolute_path)
    {
        return false;
    }

    std::error_code error;
    if (std::filesystem::is_directory(*absolute_path, error))
    {
        return open_directory_in_explorer(*absolute_path);
    }

    const std::filesystem::path parent_path = absolute_path->parent_path();
    if (parent_path.empty())
    {
        LOGGER_WARN("Parent path of requested path '{}' is empty", absolute_path->string());
        return false;
    }

    const bool did_show_item = run_process(
            {.executable = "dbus-send",
             .arguments = {
                     "--session",
                     "--dest=org.freedesktop.FileManager1",
                     "--type=method_call",
                     "/org/freedesktop/FileManager1",
                     "org.freedesktop.FileManager1.ShowItems",
                     "array:string:" + to_file_uri(*absolute_path),
                     "string:"
             }}
    );

    if (did_show_item)
    {
        return true;
    }

    LOGGER_WARN("dbus-send failed, falling back to xdg-open")
    return open_externally(parent_path);
}

} // namespace portal::os
