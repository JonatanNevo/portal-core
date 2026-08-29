//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <CoreFoundation/CoreFoundation.h>
#include <crt_externs.h>
#include <cstdlib>
#include <pwd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "portal/core/log_macros.h"

module portal.core.os.platform;

import std;
import portal.core.log;

static CFBundleRef s_bundle_instance; // NOLINT (cppcoreguidelines-avoid-non-const-global-variables)

namespace
{
[[maybe_unused]] std::filesystem::path get_home()
{
    std::filesystem::path res;

    const unsigned int uid = getuid();
    const char* home_env = std::getenv("HOME");
    if (uid != 0 && home_env)
    {
        // Ignore "HOME" if root
        res = home_env;
        return res;
    }

    passwd* pw = nullptr;
    passwd pwd{};

    long buffer_size = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buffer_size < 1)
    {
        buffer_size = 16384;
    }

    std::vector<char> buffer(buffer_size);
    int error_code = getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &pw);
    while (error_code == ERANGE)
    {
        // Buffer was too small
        buffer_size *= 2;
        buffer.resize(buffer_size);
        error_code = getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &pw);
    }

    if (error_code)
        throw std::runtime_error("Failed to get passwd struct");

    const char* temp_res = pw->pw_dir;
    if (!temp_res)
        throw std::runtime_error("User has no home directory");

    res = temp_res;
    return res;
}

[[maybe_unused]] auto get_bundle()
{
    if (!s_bundle_instance)
        s_bundle_instance = CFBundleGetMainBundle();
    return s_bundle_instance;
}

} // namespace

namespace portal::os
{
static auto& logger = log::get_logger("portal.os.macos"); // NOLINT (cppcoreguidelines-avoid-non-const-global-variables)

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
                *_NSGetEnviron()
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

    // Resolves `path` to an existing canonical path, logging and returning nullopt on failure.
    std::optional<std::filesystem::path> resolve_existing(const std::filesystem::path& path)
    {
        if (path.empty())
        {
            LOGGER_WARN("Cannot open an empty path")
            return std::nullopt;
        }

        std::error_code error;
        std::filesystem::path canonical_path = std::filesystem::canonical(path, error);
        if (error)
        {
            LOGGER_WARN("Cannot resolve path '{}': {}", path.string(), error.message());
            return std::nullopt;
        }
        return canonical_path;
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
    const auto resolved = resolve_existing(path);
    if (!resolved)
    {
        return false;
    }

    return run_process({.executable = "open", .arguments = {resolved->string()}});
}

bool open_directory_in_explorer(const std::filesystem::path& path)
{
    const auto resolved = resolve_existing(path);
    if (!resolved)
    {
        return false;
    }

    return run_process({.executable = "open", .arguments = {resolved->string()}});
}

bool show_file_in_explorer(const std::filesystem::path& path)
{
    const auto resolved = resolve_existing(path);
    if (!resolved)
    {
        return false;
    }

    // -R reveals (selects) the item in Finder.
    return run_process({.executable = "open", .arguments = {"-R", resolved->string()}});
}

SpinLock Environment::lock;

std::optional<std::string> Environment::get(const std::string& name)
{
    const std::unique_lock guard(lock);

    const char* val = std::getenv(name.c_str()); // NOLINT(concurrency-mt-unsafe)
    if (val == nullptr)
        return std::nullopt;

    return {val};
}

std::string Environment::get(const std::string& name, const std::string& default_value)
{
    auto val = get(name);
    if (!val)
        return default_value;

    return *val;
}

bool Environment::has(const std::string& name)
{
    const std::unique_lock guard(lock);

    return std::getenv(name.c_str()) != nullptr; // NOLINT(concurrency-mt-unsafe)
}

std::expected<void, std::error_code> Environment::set(const std::string& name, const std::string& value)
{
    const std::unique_lock guard(lock);

    const auto res = ::setenv(name.c_str(), value.c_str(), 1); // NOLINT(concurrency-mt-unsafe)
    if (res != 0)
    {
        return std::unexpected{std::error_code{errno, std::generic_category()}};
    }

    return {};
}

} // namespace portal::os
