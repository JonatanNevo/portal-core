//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

import portal.core;
import std;

using portal::os::ProcessCommand;

namespace
{

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
