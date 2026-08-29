//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <cerrno>
#include <cstdlib>
#include <pwd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "portal/core/log_macros.h"

module portal.core.os.platform;

import std.compat;
import portal.core.log;

namespace portal::os
{

DECLARE_LOGGER("portal.os.linux");

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

std::filesystem::path get_root_path()
{
    return std::filesystem::current_path();
}

namespace
{
    void add_to_platform_folders_from_file(
            const std::filesystem::path& filename,
            std::unordered_map<std::string, std::filesystem::path>& folders
    )
    {
        std::ifstream infile(filename.c_str());
        std::string line;
        while (std::getline(infile, line))
        {
            if (line.empty() || line.at(0) == '#' || line.starts_with("XDG_") || line.contains("_DIR"))
            {
                continue;
            }
            try
            {
                const std::size_t split_pos = line.find('=');
                const std::string key = line.substr(0, split_pos);
                const std::size_t value_start = line.find('"', split_pos);
                const std::size_t value_end = line.find('"', value_start + 1);
                const std::string value = line.substr(value_start + 1, value_end - value_start - 1);
                folders[key] = value;
            }
            catch (std::exception& e)
            {
                LOGGER_WARN(
                        "WARNING: Failed to process \"{}\" from \"{}\". Error: {}",
                        line,
                        filename.string(),
                        e.what()
                );
            }
        }
    }

    std::expected<std::filesystem::path, std::error_code> get_home()
    {
        std::filesystem::path res;

        const unsigned int uid = getuid();
        const auto home_env = Environment::get("HOME", "");
        if (uid != 0 && !home_env.empty())
        {
            // Ignore "HOME" if root
            res = home_env;
            return res;
        }

        passwd* pw = nullptr;
        passwd pwd{};

        int64_t buffer_size = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (buffer_size < 1)
        {
            buffer_size = 16384; // NOLINT (readability-magic-numbers)
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

        if (error_code != 0)
            return std::unexpected{std::error_code{error_code, std::system_category()}};

        const char* temp_res = pw->pw_dir;
        if (temp_res == nullptr)
            return std::unexpected{std::error_code{ENOENT, std::system_category()}};

        res = temp_res;
        return res;
    }

    std::unordered_map<std::string, std::filesystem::path>& get_platform_folders()
    {
        static std::unordered_map<std::string, std::filesystem::path> folders;

        if (folders.empty())
        {
            const auto home_res = get_home();
            if (!home_res)
            {
                LOGGER_WARN("Failed to get home directory");
                return folders;
            }

            const auto& home = home_res.value();
            folders["XDG_DOCUMENTS_DIR"] = home / "Documents";
            folders["XDG_DESKTOP_DIR"] = home / "Desktop";
            folders["XDG_DOWNLOAD_DIR"] = home / "Downloads";
            folders["XDG_MUSIC_DIR"] = home / "Music";
            folders["XDG_PICTURES_DIR"] = home / "Pictures";
            folders["XDG_PUBLICSHARE_DIR"] = home / "Public";
            folders["XDG_TEMPLATES_DIR"] = home / ".Templates";
            folders["XDG_VIDEOS_DIR"] = home / "Videos";
            add_to_platform_folders_from_file(get_config_home() / "user-dirs.dirs", folders);
        }

        return folders;
    }

    std::expected<std::filesystem::path, std::error_code> get_linux_folder_default(
            const char* env_name,
            const char* default_relative_path
    )
    {
        auto res = Environment::get(env_name);
        if (!res.has_value())
        {
            return get_home().transform([&default_relative_path](auto&& home) { return home / default_relative_path; });
        }

        if (res.value()[0] != '/')
        {
            LOGGER_WARN("Home env var '{}' is relative", env_name);
            return get_home().transform([&default_relative_path](auto&& home) { return home / default_relative_path; });
        }

        return *res;
    }
} // namespace

std::filesystem::path get_data_home(std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_DATA_HOME", ".local/share")
                   .or_else(
                           [&app_name](const std::error_code error_code)
                                   -> std::expected<std::filesystem::path, std::error_code>
                           {
                               LOGGER_ERROR(
                                       "Failed to get data home directory for app '{}', returning a temp dir. Error: "
                                       "{}",
                                       app_name.string(),
                                       error_code.message()
                               );
                               return std::filesystem::temp_directory_path();
                           }
                   )
                   .value() /
           "portal" / app_name;
}

std::filesystem::path get_config_home(std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_CONFIG_HOME", ".config")
                   .or_else(
                           [&app_name](const std::error_code error_code)
                                   -> std::expected<std::filesystem::path, std::error_code>
                           {
                               LOGGER_ERROR(
                                       "Failed to get config home directory for app '{}', returning a temp dir. Error: "
                                       "{}",
                                       app_name.string(),
                                       error_code.message()
                               );
                               return std::filesystem::temp_directory_path();
                           }
                   )
                   .value() /
           "portal" / app_name;
}

std::filesystem::path get_cache_dir(std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_CACHE_HOME", ".cache")
                   .or_else(
                           [&app_name](const std::error_code error_code)
                                   -> std::expected<std::filesystem::path, std::error_code>
                           {
                               LOGGER_ERROR(
                                       "Failed to get cache home directory for app '{}', returning a temp dir. Error: "
                                       "{}",
                                       app_name.string(),
                                       error_code.message()
                               );
                               return std::filesystem::temp_directory_path();
                           }
                   )
                   .value() /
           "portal" / app_name;
}

std::filesystem::path get_state_dir(std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_STATE_HOME", ".local/state")
                   .or_else(
                           [&app_name](const std::error_code error_code)
                                   -> std::expected<std::filesystem::path, std::error_code>
                           {
                               LOGGER_ERROR(
                                       "Failed to get state home directory for app '{}', returning a temp dir. Error: "
                                       "{}",
                                       app_name.string(),
                                       error_code.message()
                               );
                               return std::filesystem::temp_directory_path();
                           }
                   )
                   .value() /
           "portal" / app_name;
}

std::filesystem::path get_desktop_folder()
{
    return get_platform_folders()["XDG_DESKTOP_DIR"];
}

std::filesystem::path get_documents_folder()
{
    return get_platform_folders()["XDG_DOCUMENTS_DIR"];
}

std::filesystem::path get_download_folder()
{
    return get_platform_folders()["XDG_DOWNLOAD_DIR"];
}

std::filesystem::path get_pictures_folder()
{
    return get_platform_folders()["XDG_PICTURES_DIR"];
}

std::filesystem::path get_public_folder()
{
    return get_platform_folders()["XDG_PUBLICSHARE_DIR"];
}

std::filesystem::path get_music_folder()
{
    return get_platform_folders()["XDG_MUSIC_DIR"];
}

std::filesystem::path get_video_folder()
{
    return get_platform_folders()["XDG_VIDEOS_DIR"];
}

} // namespace portal::os
