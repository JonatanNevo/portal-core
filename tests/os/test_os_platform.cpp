//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

import portal.core;
import std;

using portal::os::Environment;
using portal::os::ProcessCommand;

namespace
{

// A process-unique environment variable name so parallel test binaries and
// repeated runs never collide on real environment state.
std::string unique_env_name()
{
    static const std::uint64_t salt = std::random_device{}();
    static std::atomic<std::uint64_t> counter{0};
    return std::format("PORTAL_OS_TEST_{:X}_{}", salt, counter.fetch_add(1));
}


// Installs a capturing runner for the lifetime of the object and restores the
// platform default afterwards. `result` is what the fake runner reports for
// every command; the predicate overload allows per-command control.
class ScopedRunner
{
public:
    template<typename B>
        requires std::same_as<B, bool>
    ScopedRunner(std::vector<ProcessCommand>& sink, B result)
    {
        portal::os::set_process_runner(
                [&sink, result](const ProcessCommand& command)
                {
                    sink.push_back(command);
                    return result;
                }
        );
    }

    ScopedRunner(std::vector<ProcessCommand>& sink, std::function<bool(const ProcessCommand&)> predicate)
    {
        portal::os::set_process_runner(
                [&sink, predicate = std::move(predicate)](const ProcessCommand& command)
                {
                    sink.push_back(command);
                    return predicate(command);
                }
        );
    }

    ScopedRunner(const ScopedRunner&) = delete;
    ScopedRunner& operator=(const ScopedRunner&) = delete;

    ~ScopedRunner() { portal::os::set_process_runner(nullptr); }
};

// Installs a capturing runner for run_process_captured for the lifetime of the
// object, restoring the platform default afterwards. `responder` decides what
// each command reports (std::nullopt means "executable not available").
class ScopedCapturedRunner
{
public:
    ScopedCapturedRunner(
            std::vector<ProcessCommand>& sink,
            std::function<std::optional<portal::os::ProcessOutput>(const ProcessCommand&)> responder
    )
    {
        portal::os::set_captured_process_runner(
                [&sink, responder = std::move(responder)](const ProcessCommand& command)
                {
                    sink.push_back(command);
                    return responder(command);
                }
        );
    }

    ScopedCapturedRunner(const ScopedCapturedRunner&) = delete;
    ScopedCapturedRunner& operator=(const ScopedCapturedRunner&) = delete;

    ~ScopedCapturedRunner() { portal::os::set_captured_process_runner(nullptr); }
};

std::filesystem::path make_temp_dir()
{
    static const std::uint64_t salt = std::random_device{}();
    static std::atomic<std::uint64_t> counter{0};
    const auto dir =
            std::filesystem::temp_directory_path() / std::format("portal_os_test_{:x}_{}", salt, counter.fetch_add(1));
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

SCENARIO("os_platform open functions reject bad input without launching a process", "[unit][core][os]")
{
    std::vector<ProcessCommand> calls;
    ScopedRunner guard(calls, true);

    GIVEN("an empty path")
    {
        THEN("nothing is launched and every call fails")
        {
            REQUIRE_FALSE(portal::os::open_externally({}));
            REQUIRE_FALSE(portal::os::open_directory_in_explorer({}));
            REQUIRE_FALSE(portal::os::show_file_in_explorer({}));
            REQUIRE(calls.empty());
        }
    }

#if defined(__linux__)
    GIVEN("a regular file passed to open_directory_in_explorer")
    {
        const auto dir = make_temp_dir();
        const auto file = dir / "example.txt";
        std::ofstream(file) << "hi";

        THEN("it is rejected because the path is not a directory")
        {
            REQUIRE_FALSE(portal::os::open_directory_in_explorer(file));
            REQUIRE(calls.empty());
        }

        std::filesystem::remove_all(dir);
    }
#endif
}

SCENARIO("os_platform open functions build the expected command", "[unit][core][os]")
{
    std::vector<ProcessCommand> calls;

    const auto dir = make_temp_dir();
    const auto file = dir / "example.txt";
    std::ofstream(file) << "hi";

    WHEN("opening a file externally")
    {
        ScopedRunner guard(calls, true);
        const bool ok = portal::os::open_externally(file);

        THEN("one command runs with a single absolute-path argument")
        {
            REQUIRE(ok);
            REQUIRE(calls.size() == 1);
            REQUIRE(calls[0].arguments.size() == 1);
            REQUIRE(std::filesystem::path(calls[0].arguments[0]).is_absolute());
        }
    }

    WHEN("opening a directory through a relative path")
    {
        ScopedRunner guard(calls, true);
        const auto previous = std::filesystem::current_path();
        std::filesystem::current_path(dir);
        portal::os::open_directory_in_explorer(".");
        std::filesystem::current_path(previous);

        THEN("the argument is resolved to an absolute path")
        {
            REQUIRE(calls.size() == 1);
            REQUIRE(std::filesystem::path(calls[0].arguments.back()).is_absolute());
        }
    }

    WHEN("revealing a file in the file manager")
    {
        ScopedRunner guard(calls, true);
        portal::os::show_file_in_explorer(file);

        THEN("the platform reveal command is used")
        {
            REQUIRE(calls.size() == 1);
#if defined(__linux__)
            REQUIRE(calls[0].executable == "dbus-send");
            REQUIRE(calls[0].arguments.back() == "string:");
            REQUIRE(calls[0].arguments[calls[0].arguments.size() - 2].starts_with("array:string:file://"));
#elif defined(__APPLE__)
            REQUIRE(calls[0].executable == "open");
            REQUIRE(calls[0].arguments.front() == "-R");
#elif defined(_WIN32)
            REQUIRE(calls[0].executable == "explorer.exe");
            REQUIRE(calls[0].arguments.front().starts_with("/select,"));
#endif
        }
    }

    WHEN("revealing a directory in the file manager")
    {
        ScopedRunner guard(calls, true);
        portal::os::show_file_in_explorer(dir);

        THEN("it is opened as a directory, not revealed")
        {
            REQUIRE(calls.size() == 1);
#if defined(__linux__)
            REQUIRE(calls[0].executable == "xdg-open");
            REQUIRE(calls[0].arguments.size() == 1);
#endif
        }
    }

#if defined(__linux__)
    WHEN("the reveal command fails")
    {
        ScopedRunner guard(calls, [](const ProcessCommand& command) { return command.executable != "dbus-send"; });
        const bool ok = portal::os::show_file_in_explorer(file);

        THEN("it falls back to xdg-open on the parent directory")
        {
            REQUIRE(ok);
            REQUIRE(calls.size() == 2);
            REQUIRE(calls[0].executable == "dbus-send");
            REQUIRE(calls[1].executable == "xdg-open");
            REQUIRE(std::filesystem::path(calls[1].arguments.back()) == dir);
        }
    }
#endif

    std::filesystem::remove_all(dir);
}

SCENARIO("Environment reports missing variables consistently", "[unit][core][os]")
{
    const auto name = unique_env_name();

    GIVEN("a variable that is not defined")
    {
        REQUIRE_FALSE(Environment::has(name));

        THEN("get returns nullopt")
        {
            REQUIRE_FALSE(Environment::get(name).has_value());
        }

        THEN("get with a default returns the default")
        {
            REQUIRE(Environment::get(name, "fallback") == "fallback");
        }
    }
}

#if defined(__linux__)

using portal::os::IconType;
using portal::os::MessageBoxButton;
using portal::os::MessageBoxType;
using portal::os::ProcessOutput;

SCENARIO("open_message_box maps a backend's exit code to the pressed button", "[unit][core][os]")
{
    std::vector<ProcessCommand> calls;

    GIVEN("kdialog answers a yes/no box")
    {
        THEN("exit 0 is Yes and a non-zero exit is No")
        {
            {
                ScopedCapturedRunner guard(calls, [](const ProcessCommand&) { return ProcessOutput{.exit_code = 0}; });
                REQUIRE(portal::os::open_message_box("T", "M", MessageBoxType::YesNo, IconType::Info) ==
                        MessageBoxButton::Yes);
            }
            REQUIRE(calls.size() == 1);
            REQUIRE(calls[0].executable == "kdialog");
            REQUIRE(calls[0].arguments.front() == "--yesno");

            calls.clear();
            ScopedCapturedRunner guard(calls, [](const ProcessCommand&) { return ProcessOutput{.exit_code = 1}; });
            REQUIRE(portal::os::open_message_box("T", "M", MessageBoxType::YesNo, IconType::Info) ==
                    MessageBoxButton::No);
        }
    }

    GIVEN("kdialog answers a yes/no/cancel box")
    {
        auto answer = [&calls](const int exit_code)
        {
            calls.clear();
            ScopedCapturedRunner guard(
                    calls,
                    [exit_code](const ProcessCommand&) { return ProcessOutput{.exit_code = exit_code}; }
            );
            return portal::os::open_message_box("T", "M", MessageBoxType::YesNoCancel, IconType::Warning);
        };

        THEN("0/1/2 map to Yes/No/Cancel")
        {
            REQUIRE(answer(0) == MessageBoxButton::Yes);
            REQUIRE(answer(1) == MessageBoxButton::No);
            REQUIRE(answer(2) == MessageBoxButton::Cancel);
            REQUIRE(calls[0].executable == "kdialog");
            REQUIRE(calls[0].arguments.front() == "--warningyesnocancel");
        }
    }

    GIVEN("zenity reports the extra Cancel button on stdout")
    {
        ScopedCapturedRunner guard(
                calls,
                [](const ProcessCommand& command) -> std::optional<ProcessOutput>
                {
                    if (command.executable == "kdialog")
                        return std::nullopt; // kdialog not installed
                    return ProcessOutput{.exit_code = 1, .output = "Cancel"};
                }
        );

        THEN("the box falls through to zenity and returns Cancel")
        {
            REQUIRE(portal::os::open_message_box("T", "M", MessageBoxType::YesNoCancel, IconType::Info) ==
                    MessageBoxButton::Cancel);
            REQUIRE(calls.size() == 2);
            REQUIRE(calls[0].executable == "kdialog");
            REQUIRE(calls[1].executable == "zenity");
        }
    }

    GIVEN("no backend is available")
    {
        ScopedCapturedRunner guard(calls, [](const ProcessCommand&) { return std::nullopt; });

        THEN("every backend is tried and a safe default is returned")
        {
            // No backend means "nothing was chosen": Cancel for anything with a
            // choice, OK for a plain acknowledgement box.
            REQUIRE(portal::os::open_message_box("T", "M", MessageBoxType::YesNo, IconType::Info) ==
                    MessageBoxButton::Cancel);
            REQUIRE(portal::os::open_message_box("T", "M", MessageBoxType::OK, IconType::Info) == MessageBoxButton::OK);

            std::vector<std::string> tried;
            for (const auto& call: calls)
                tried.push_back(call.executable);
            REQUIRE(tried ==
                    std::vector<std::string>{"kdialog", "zenity", "xmessage", "kdialog", "zenity", "xmessage"});
        }
    }
}

SCENARIO("open_notification_popup prefers notify-send and falls back", "[unit][core][os]")
{
    std::vector<ProcessCommand> calls;

    WHEN("notify-send succeeds")
    {
        ScopedRunner guard(calls, true);
        const bool shown = portal::os::open_notification_popup("Title", "Body", IconType::Warning);

        THEN("only notify-send runs, with the icon name and summary/body")
        {
            REQUIRE(shown);
            REQUIRE(calls.size() == 1);
            REQUIRE(calls[0].executable == "notify-send");
            REQUIRE(calls[0].arguments == std::vector<std::string>{"--icon", "dialog-warning", "Title", "Body"});
        }
    }

    WHEN("no notification daemon answers")
    {
        ScopedRunner guard(calls, false);
        std::vector<ProcessCommand> box_calls;
        ScopedCapturedRunner box_guard(box_calls, [](const ProcessCommand&) { return std::nullopt; });

        const bool shown = portal::os::open_notification_popup("Title", "Body", IconType::Info);

        THEN("it tries every notifier and then a message box")
        {
            REQUIRE(shown);
            std::vector<std::string> tried;
            for (const auto& call: calls)
                tried.push_back(call.executable);
            REQUIRE(tried == std::vector<std::string>{"notify-send", "kdialog", "zenity"});
            REQUIRE_FALSE(box_calls.empty());
        }
    }
}

#endif

// set() on Windows writes to the user registry rather than the current
// process environment, so the round-trip below only holds on POSIX.
#if !defined(_WIN32)
SCENARIO("Environment set makes a variable readable", "[unit][core][os]")
{
    const auto name = unique_env_name();

    WHEN("a variable is set")
    {
        const auto result = Environment::set(name, "value-1");
        REQUIRE(result.has_value());

        THEN("has, get and get-with-default all observe it")
        {
            REQUIRE(Environment::has(name));
            REQUIRE(Environment::get(name) == "value-1");
            REQUIRE(Environment::get(name, "fallback") == "value-1");
        }

        AND_WHEN("the variable is overwritten")
        {
            REQUIRE(Environment::set(name, "value-2").has_value());

            THEN("the new value is returned")
            {
                REQUIRE(Environment::get(name) == "value-2");
            }
        }

        AND_WHEN("the variable is set to an empty string")
        {
            REQUIRE(Environment::set(name, "").has_value());

            THEN("it still exists and reads back empty")
            {
                REQUIRE(Environment::has(name));
                REQUIRE(Environment::get(name) == "");
            }
        }
    }
}
#endif
