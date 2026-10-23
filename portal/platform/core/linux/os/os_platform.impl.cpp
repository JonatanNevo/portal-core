//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
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

    // Owns a file descriptor and closes it on destruction.
    class UniqueFd
    {
    public:
        UniqueFd() = default;
        explicit UniqueFd(const int fd) : m_fd(fd) {}

        UniqueFd(const UniqueFd&) = delete;
        UniqueFd& operator=(const UniqueFd&) = delete;
        UniqueFd(UniqueFd&& other) noexcept : m_fd(std::exchange(other.m_fd, -1)) {}
        UniqueFd& operator=(UniqueFd&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                m_fd = std::exchange(other.m_fd, -1);
            }
            return *this;
        }
        ~UniqueFd() { reset(); }

        [[nodiscard]] int get() const { return m_fd; }

        void reset()
        {
            if (m_fd >= 0)
            {
                ::close(m_fd);
                m_fd = -1;
            }
        }

    private:
        int m_fd = -1;
    };

    // RAII wrapper around posix_spawn_file_actions_t.
    class SpawnFileActions
    {
    public:
        SpawnFileActions() { posix_spawn_file_actions_init(&m_actions); }

        SpawnFileActions(const SpawnFileActions&) = delete;
        SpawnFileActions& operator=(const SpawnFileActions&) = delete;
        ~SpawnFileActions() { posix_spawn_file_actions_destroy(&m_actions); }

        [[nodiscard]] posix_spawn_file_actions_t* get() { return &m_actions; }

    private:
        posix_spawn_file_actions_t m_actions{};
    };

    constexpr std::size_t BUFFER_SIZE = 4096;

    std::optional<ProcessOutput> default_run_process_captured(const ProcessCommand& command)
    {
        std::array<int, 2> raw_pipe{-1, -1};
        if (::pipe(raw_pipe.data()) != 0)
        {
            // NOLINTNEXTLINE (concurrency-mt-unsafe)
            LOGGER_WARN("Failed to create a pipe for '{}': {}", command.executable, std::strerror(errno))
            return std::nullopt;
        }

        const UniqueFd read_end{raw_pipe[0]};
        UniqueFd write_end{raw_pipe[1]};

        std::vector<const char*> argv;
        argv.reserve(command.arguments.size() + 2);
        argv.push_back(command.executable.c_str());
        for (const std::string& argument: command.arguments)
        {
            argv.push_back(argument.c_str());
        }
        argv.push_back(nullptr);

        SpawnFileActions actions;
        posix_spawn_file_actions_adddup2(actions.get(), write_end.get(), STDOUT_FILENO);
        posix_spawn_file_actions_addopen(actions.get(), STDERR_FILENO, "/dev/null", O_WRONLY, 0);
        posix_spawn_file_actions_addclose(actions.get(), read_end.get());
        posix_spawn_file_actions_addclose(actions.get(), write_end.get());

        pid_t pid{};
        const int spawn_result = posix_spawnp(
                &pid,
                command.executable.c_str(),
                actions.get(),
                nullptr,
                const_cast<char* const*>(argv.data()), // NOLINT (cppcoreguidelines-pro-type-const-cast)
                environ
        );
        if (spawn_result != 0)
        {
            return std::nullopt;
        }

        // Drop our copy of the write end so read() reports EOF once the child exits.
        write_end.reset();

        std::string output;
        std::array<char, BUFFER_SIZE> buffer{};
        while (true)
        {
            const ssize_t count = ::read(read_end.get(), buffer.data(), buffer.size());
            if (count > 0)
            {
                output.append(buffer.data(), static_cast<std::size_t>(count));
            }
            else if (count == 0 || errno != EINTR)
            {
                break;
            }
        }

        int status = 0;
        while (::waitpid(pid, &status, 0) == -1 && errno == EINTR)
        {}

        while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
        {
            output.pop_back();
        }

        return ProcessOutput{
                .exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1,
                .output = std::move(output),
        };
    }

    // NOLINTNEXTLINE (cppcoreguidelines-avoid-non-const-global-variables)
    CapturedProcessRunner g_captured_process_runner;

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

std::optional<ProcessOutput> run_process_captured(const ProcessCommand& command)
{
    return g_captured_process_runner ? g_captured_process_runner(command) : default_run_process_captured(command);
}

void set_captured_process_runner(CapturedProcessRunner runner)
{
    g_captured_process_runner = std::move(runner);
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

namespace
{
    // The freedesktop icon-name matching an IconType, used for notification and
    // dialog icons across the backends.
    const char* freedesktop_icon_name(const IconType icon)
    {
        switch (icon)
        {
            case IconType::Warning:
                return "dialog-warning";
            case IconType::Error:
                return "dialog-error";
            case IconType::Question:
                return "dialog-question";
            case IconType::Info:
                break;
        }
        return "dialog-information";
    }

    bool is_severe(const IconType icon)
    {
        return icon == IconType::Warning || icon == IconType::Error;
    }

    MessageBoxButton affirmative_button(const MessageBoxType type)
    {
        switch (type)
        {
            case MessageBoxType::OK:
            case MessageBoxType::OKCancel:
                return MessageBoxButton::OK;
            case MessageBoxType::TryAgainContinueCancel:
                return MessageBoxButton::TryAgain;
            case MessageBoxType::YesNo:
            case MessageBoxType::YesNoCancel:
                break;
        }
        return MessageBoxButton::Yes;
    }

    MessageBoxButton negative_button(const MessageBoxType type)
    {
        return type == MessageBoxType::YesNo ? MessageBoxButton::No : MessageBoxButton::Cancel;
    }

    // The middle button of a three-button box, or nullopt for every other type.
    std::optional<MessageBoxButton> middle_button(const MessageBoxType type)
    {
        switch (type)
        {
            case MessageBoxType::YesNoCancel:
                return MessageBoxButton::No;
            case MessageBoxType::TryAgainContinueCancel:
                return MessageBoxButton::Continue;
            case MessageBoxType::OK:
            case MessageBoxType::OKCancel:
            case MessageBoxType::YesNo:
                break;
        }
        return std::nullopt;
    }

    // All backends are launched through ProcessCommand with a real argv, so the
    // title and message never pass through a shell and need no quoting.

    std::optional<MessageBoxButton> kdialog_message_box(
            const std::string_view title,
            const std::string_view message,
            const MessageBoxType type,
            const IconType icon
    )
    {
        ProcessCommand command{.executable = "kdialog"};
        std::vector<std::string>& args = command.arguments;

        switch (type)
        {
            case MessageBoxType::OK:
                if (icon == IconType::Error)
                {
                    args.emplace_back("--error");
                }
                else if (is_severe(icon))
                {
                    args.emplace_back("--sorry");
                }
                else
                {
                    args.emplace_back("--msgbox");
                }
                args.emplace_back(message);
                break;
            case MessageBoxType::OKCancel:
                args.emplace_back(is_severe(icon) ? "--warningyesno" : "--yesno");
                args.emplace_back(message);
                args.emplace_back("--yes-label");
                args.emplace_back("OK");
                args.emplace_back("--no-label");
                args.emplace_back("Cancel");
                break;
            case MessageBoxType::YesNo:
                args.emplace_back(is_severe(icon) ? "--warningyesno" : "--yesno");
                args.emplace_back(message);
                break;
            case MessageBoxType::YesNoCancel:
                args.emplace_back(is_severe(icon) ? "--warningyesnocancel" : "--yesnocancel");
                args.emplace_back(message);
                break;
            case MessageBoxType::TryAgainContinueCancel:
                args.emplace_back(is_severe(icon) ? "--warningyesnocancel" : "--yesnocancel");
                args.emplace_back(message);
                args.emplace_back("--yes-label");
                args.emplace_back("Try Again");
                args.emplace_back("--no-label");
                args.emplace_back("Continue");
                args.emplace_back("--cancel-label");
                args.emplace_back("Cancel");
                break;
        }

        if (!title.empty())
        {
            args.emplace_back("--title");
            args.emplace_back(title);
        }
        args.emplace_back("--icon");
        args.emplace_back(freedesktop_icon_name(icon));

        const auto result = run_process_captured(command);
        if (!result)
        {
            return std::nullopt;
        }

        // kdialog exit codes: 0 = yes/ok, 1 = no, 2 = cancel (or dismissed).
        if (result->exit_code == 0)
        {
            return affirmative_button(type);
        }
        if (const auto middle = middle_button(type); middle && result->exit_code == 1)
        {
            return *middle;
        }
        return negative_button(type);
    }

    std::optional<MessageBoxButton> zenity_message_box(
            const std::string_view title,
            const std::string_view message,
            const MessageBoxType type,
            const IconType icon
    )
    {
        ProcessCommand command{.executable = "zenity"};
        std::vector<std::string>& args = command.arguments;

        switch (type)
        {
            case MessageBoxType::OK:
                if (icon == IconType::Error)
                {
                    args.emplace_back("--error");
                }
                else if (icon == IconType::Warning)
                {
                    args.emplace_back("--warning");
                }
                else
                {
                    args.emplace_back("--info");
                }
                break;
            case MessageBoxType::OKCancel:
                args.emplace_back("--question");
                args.emplace_back("--ok-label=OK");
                args.emplace_back("--cancel-label=Cancel");
                break;
            case MessageBoxType::YesNo:
                args.emplace_back("--question");
                args.emplace_back("--ok-label=Yes");
                args.emplace_back("--cancel-label=No");
                break;
            case MessageBoxType::YesNoCancel:
                args.emplace_back("--question");
                args.emplace_back("--ok-label=Yes");
                args.emplace_back("--cancel-label=No");
                args.emplace_back("--extra-button=Cancel");
                break;
            case MessageBoxType::TryAgainContinueCancel:
                args.emplace_back("--question");
                args.emplace_back("--ok-label=Try Again");
                args.emplace_back("--cancel-label=Continue");
                args.emplace_back("--extra-button=Cancel");
                break;
        }

        args.emplace_back(std::format("--title={}", title));
        args.emplace_back(std::format("--text={}", message));

        const auto result = run_process_captured(command);
        if (!result)
        {
            return std::nullopt;
        }

        // zenity exits 0 when the primary button is pressed. The extra button
        // prints its own label to stdout and exits non-zero; a plain cancel or a
        // dismissed dialog exits non-zero with no output.
        if (type == MessageBoxType::OK || result->exit_code == 0)
        {
            return affirmative_button(type);
        }
        if (const auto middle = middle_button(type); middle && result->output != "Cancel")
        {
            return *middle;
        }
        return negative_button(type);
    }

    std::optional<MessageBoxButton> xmessage_message_box(
            const std::string_view title,
            const std::string_view message,
            const MessageBoxType type
    )
    {
        ProcessCommand command{.executable = "xmessage"};
        std::vector<std::string>& args = command.arguments;
        args.emplace_back("-center");
        args.emplace_back("-buttons");
        // Button tags: 0 = affirmative, 1 = No/Continue, 2 = Cancel. xmessage exits with
        // the pressed button's tag (a window-manager close also exits 1).
        switch (type)
        {
            case MessageBoxType::OK:
                args.emplace_back("OK:0");
                break;
            case MessageBoxType::OKCancel:
                args.emplace_back("OK:0,Cancel:2");
                break;
            case MessageBoxType::YesNo:
                args.emplace_back("Yes:0,No:1");
                break;
            case MessageBoxType::YesNoCancel:
                args.emplace_back("Yes:0,No:1,Cancel:2");
                break;
            case MessageBoxType::TryAgainContinueCancel:
                args.emplace_back("Try Again:0,Continue:1,Cancel:2");
                break;
        }
        if (!title.empty())
        {
            args.emplace_back("-title");
            args.emplace_back(title);
        }
        args.emplace_back(message.empty() ? std::string{" "} : std::string{message});

        const auto result = run_process_captured(command);
        if (!result)
        {
            return std::nullopt;
        }

        if (type == MessageBoxType::OK || result->exit_code == 0)
        {
            return affirmative_button(type);
        }
        if (const auto middle = middle_button(type); middle && result->exit_code == 1)
        {
            return *middle;
        }
        return negative_button(type);
    }

    bool notify_send_popup(const std::string_view title, const std::string_view message, const IconType icon)
    {
        ProcessCommand command{.executable = "notify-send"};
        std::vector<std::string>& args = command.arguments;
        args.emplace_back("--icon");
        args.emplace_back(freedesktop_icon_name(icon));

        // notify-send needs a non-empty summary; use the message when there is no title.
        if (title.empty())
        {
            args.emplace_back(message.empty() ? std::string{" "} : std::string{message});
        }
        else
        {
            args.emplace_back(title);
            if (!message.empty())
            {
                args.emplace_back(message);
            }
        }
        return run_process(command);
    }

    bool kdialog_passive_popup(const std::string_view title, const std::string_view message, const IconType icon)
    {
        ProcessCommand command{.executable = "kdialog"};
        std::vector<std::string>& args = command.arguments;
        args.emplace_back("--icon");
        args.emplace_back(freedesktop_icon_name(icon));
        if (!title.empty())
        {
            args.emplace_back("--title");
            args.emplace_back(title);
        }
        args.emplace_back("--passivepopup");
        args.emplace_back(message);
        args.emplace_back("5"); // seconds on screen
        return run_process(command);
    }

    bool zenity_notification(const std::string_view title, const std::string_view message)
    {
        ProcessCommand command{.executable = "zenity"};
        std::vector<std::string>& args = command.arguments;
        args.emplace_back("--notification");
        args.emplace_back(
                title.empty() ? std::format("--text={}", message) : std::format("--text={}\n{}", title, message)
        );
        return run_process(command);
    }

} // namespace

MessageBoxButton open_message_box(
        const std::string_view title,
        const std::string_view message,
        const MessageBoxType type,
        const IconType icon
)
{
    if (const auto button = kdialog_message_box(title, message, type, icon))
    {
        return *button;
    }
    if (const auto button = zenity_message_box(title, message, type, icon))
    {
        return *button;
    }
    if (const auto button = xmessage_message_box(title, message, type))
    {
        return *button;
    }

    LOGGER_WARN("No message box backend available (tried kdialog, zenity, xmessage)")
    return type == MessageBoxType::OK ? MessageBoxButton::OK : MessageBoxButton::Cancel;
}

bool open_notification_popup(const std::string_view title, const std::string_view message, const IconType icon)
{
    if (notify_send_popup(title, message, icon) || kdialog_passive_popup(title, message, icon) ||
        zenity_notification(title, message))
    {
        return true;
    }

    LOGGER_WARN("No notification backend available, falling back to a message box")
    open_message_box(title, message, MessageBoxType::OK, icon);
    return true;
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

bool is_debugger_connected()
{
    // A traced process reports a non-zero TracerPid in /proc/self/status
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line))
    {
        constexpr std::string_view tracer_key = "TracerPid:";
        if (line.starts_with(tracer_key))
        {
            const auto pid_start = line.find_first_not_of(" \t", tracer_key.size());
            return pid_start != std::string::npos && line[pid_start] != '0';
        }
    }
    return false;
}

} // namespace portal::os
