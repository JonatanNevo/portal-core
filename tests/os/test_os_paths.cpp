//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

#if defined(__linux__)
#include <stdlib.h>
#endif

import portal.core;
import std;

using portal::os::Environment;

namespace
{

#if defined(__linux__)
// Sets an environment variable for the lifetime of the object and restores the
// previous state (value or absence) afterwards, so path tests that depend on
// XDG_* variables never leak state into later tests.
class ScopedEnv
{
public:
    ScopedEnv(std::string name, const std::string& value) :
        m_name(std::move(name)),
        m_previous(Environment::get(m_name))
    {
        REQUIRE(Environment::set(m_name, value).has_value());
    }

    ~ScopedEnv()
    {
        if (m_previous)
            Environment::set(m_name, *m_previous);
        else
            ::unsetenv(m_name.c_str());
    }

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

private:
    std::string m_name;
    std::optional<std::string> m_previous;
};

// A distinct absolute path per call. Nothing is created on disk; the path
// functions are pure string composition on top of the configured base.
std::filesystem::path unique_abs_base()
{
    static const std::uint64_t salt = std::random_device{}();
    static std::atomic<std::uint64_t> counter{0};
    return std::filesystem::path("/portal-os-test") / std::format("{:x}_{}", salt, counter.fetch_add(1));
}
#endif

} // namespace

SCENARIO("get_binary_path round-trips whatever set_binary_path stored", "[unit][core][os]")
{
    const std::filesystem::path original = portal::os::get_binary_path();

    WHEN("a binary path is set")
    {
        const std::filesystem::path path = "/opt/portal/bin/portal-app";
        portal::os::set_binary_path(path);

        THEN("get_binary_path returns it unchanged")
        {
            REQUIRE(portal::os::get_binary_path() == path);
        }

        AND_WHEN("it is overwritten")
        {
            const std::filesystem::path other = "/usr/local/bin/other";
            portal::os::set_binary_path(other);

            THEN("the newest value is returned")
            {
                REQUIRE(portal::os::get_binary_path() == other);
            }
        }
    }

    portal::os::set_binary_path(original);
}

#if defined(__linux__)

SCENARIO("get_root_path reports the current working directory", "[unit][core][os]")
{
    REQUIRE(portal::os::get_root_path() == std::filesystem::current_path());
}

SCENARIO("XDG base directory helpers honour an absolute override", "[unit][core][os]")
{
    struct Case
    {
        const char* env_name;
        std::filesystem::path (*fn)(std::filesystem::path);
    };

    const auto cases = std::to_array<Case>({
            {"XDG_DATA_HOME", &portal::os::get_data_home},
            {"XDG_CONFIG_HOME", &portal::os::get_config_home},
            {"XDG_CACHE_HOME", &portal::os::get_cache_dir},
            {"XDG_STATE_HOME", &portal::os::get_state_dir},
    });

    for (const auto& [env_name, fn]: cases)
    {
        GIVEN(std::string(env_name) + " points at an absolute path")
        {
            const auto base = unique_abs_base();
            ScopedEnv guard(env_name, base.string());

            THEN("the app folder is <base>/portal/<app_name>")
            {
                REQUIRE(fn("my-app") == base / "portal" / "my-app");
            }

            THEN("the app folder sits directly under <base>/portal")
            {
                REQUIRE(fn("my-app").parent_path() == base / "portal");
            }

            THEN("different app names produce sibling folders")
            {
                REQUIRE(fn("a") != fn("b"));
                REQUIRE(fn("a").parent_path() == fn("b").parent_path());
            }
        }
    }
}

SCENARIO("XDG base directory helpers reject a relative override", "[unit][core][os]")
{
    GIVEN("XDG_CONFIG_HOME set to a relative path")
    {
        ScopedEnv guard("XDG_CONFIG_HOME", "relative/config");

        THEN("the relative value is ignored and an absolute fallback is used")
        {
            const auto result = portal::os::get_config_home("app");
            REQUIRE(result.is_absolute());
            REQUIRE(result.string().find("relative/config") == std::string::npos);
            REQUIRE(result.filename() == "app");
            REQUIRE(result.parent_path().filename() == "portal");
        }
    }
}

SCENARIO("user media folders are absolute and distinct", "[unit][core][os]")
{
    const auto desktop = portal::os::get_desktop_folder();
    const auto documents = portal::os::get_documents_folder();
    const auto download = portal::os::get_download_folder();
    const auto pictures = portal::os::get_pictures_folder();
    const auto public_folder = portal::os::get_public_folder();
    const auto music = portal::os::get_music_folder();
    const auto video = portal::os::get_video_folder();

    // When the home directory cannot be resolved the lookup table is empty and
    // every folder comes back empty; there is nothing else to assert then.
    if (desktop.empty())
    {
        SUCCEED("home directory could not be resolved in this environment");
        return;
    }

    for (const auto* folder: {&desktop, &documents, &download, &pictures, &public_folder, &music, &video})
    {
        REQUIRE_FALSE(folder->empty());
        REQUIRE(folder->is_absolute());
    }

    REQUIRE(desktop != documents);
    REQUIRE(music != video);
    REQUIRE(download != pictures);
}

#endif // defined(__linux__)
