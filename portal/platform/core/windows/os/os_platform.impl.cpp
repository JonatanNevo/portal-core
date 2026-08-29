//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <Shlobj.h>
#include <Windows.h>
#include <process.h>
#include <stringapiset.h>

#include "portal/core/log_macros.h"

module portal.core.os.platform;

import std;
import portal.core.log;

namespace
{
std::string utf16_to_utf8(const wchar_t* w_str)
{
    std::string res;

    // If the 6th parameter is 0 then WideCharToMultiByte returns the number of bytes needed to store the result.
    int size = WideCharToMultiByte(CP_UTF8, 0, w_str, -1, nullptr, 0, nullptr, nullptr);
    if (size > 0)
    {
        // If the converted UTF-8 string could not be in the initial buffer. Allocate one that can hold it.
        std::vector<char> buffer(size);
        size = WideCharToMultiByte(
                CP_UTF8,
                0,
                w_str,
                -1,
                &buffer[0],
                static_cast<int>(buffer.size()),
                nullptr,
                nullptr
        );
        res = buffer.data();
    }
    if (size == 0)
    {
        // WideCharToMultiByte return 0 for errors.
        throw std::runtime_error("UTF16 to UTF8 failed with error code: " + std::to_string(GetLastError()));
    }
    return res;
}

class FreeCoTaskMemory
{
    LPWSTR pointer = nullptr;

public:
    explicit FreeCoTaskMemory(const LPWSTR pointer) : pointer(pointer) {};

    ~FreeCoTaskMemory() { CoTaskMemFree(pointer); }
};

std::filesystem::path get_known_windows_folder(REFKNOWNFOLDERID folder_id, const char* error_msg)
{
    LPWSTR windows_path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(folder_id, KF_FLAG_CREATE, nullptr, &windows_path);
    FreeCoTaskMemory scopeBoundMemory(windows_path);

    if (!SUCCEEDED(result))
    {
        throw std::runtime_error(error_msg);
    }

    return utf16_to_utf8(windows_path);
}

[[maybe_unused]] std::filesystem::path get_appdata()
{
    return get_known_windows_folder(FOLDERID_RoamingAppData, "RoamingAppData could not be found");
}

[[maybe_unused]] std::filesystem::path get_appdata_local()
{
    return get_known_windows_folder(FOLDERID_LocalAppData, "LocalAppData could not be found");
}
} // namespace

namespace portal::os
{
static auto& logger =
        log::get_logger("portal.os.windows"); // NOLINT (cppcoreguidelines-avoid-non-const-global-variables)

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

        // _P_WAIT: block until the child exits. Note that some shell helpers (notably
        // explorer.exe) return non-zero even on success, so callers that only care
        // about "best effort" should not rely on the return value.
        const std::intptr_t result = _spawnvp(_P_WAIT, command.executable.c_str(), argv.data());
        return result == 0;
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

bool show_file_in_explorer(const std::filesystem::path& path)
{
    const auto resolved = resolve_existing(path);
    if (!resolved)
    {
        return false;
    }

    // explorer.exe /select,<path> opens the containing folder with the item selected.
    // explorer's exit code is unreliable, so this is treated as fire-and-forget.
    run_process({.executable = "explorer.exe", .arguments = {"/select," + resolved->string()}});
    return true;
}

bool open_directory_in_explorer(const std::filesystem::path& path)
{
    return show_file_in_explorer(path);
}

bool open_externally(const std::filesystem::path& path)
{
    const auto resolved = resolve_existing(path);
    if (!resolved)
    {
        return false;
    }

    run_process({.executable = "explorer.exe", .arguments = {resolved->string()}});
    return true;
}

SpinLock Environment::lock;

std::optional<std::string> Environment::get(const std::string& name)
{
    const std::unique_lock guard(lock);
    char* value = nullptr;
    size_t len = 0;

    if (_dupenv_s(&value, &len, name.c_str()) == 0 && value != nullptr)
    {
        std::string result(value);
        free(value); // Free the allocated memory
        return result;
    }
    return std::nullopt;
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

    HKEY hKey;
    LSTATUS lOpenStatus = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_ALL_ACCESS, &hKey);

    if (lOpenStatus == ERROR_SUCCESS)
    {
        lOpenStatus = RegQueryValueExA(hKey, name.c_str(), 0, NULL, NULL, NULL);
        RegCloseKey(hKey);
    }

    return lOpenStatus == ERROR_SUCCESS;
}

std::expected<void, std::error_code> Environment::set(const std::string& name, const std::string& value)
{
    const std::unique_lock guard(lock);

    HKEY hKey;
    const auto key_path = "Environment";
    DWORD create_new_key;
    const LSTATUS lOpenStatus = RegCreateKeyExA(
            HKEY_CURRENT_USER,
            key_path,
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            nullptr,
            &hKey,
            &create_new_key
    );
    if (lOpenStatus == ERROR_SUCCESS)
    {
        const LSTATUS lSetStatus = RegSetValueExA(
                hKey,
                name.c_str(),
                0,
                REG_SZ,
                (LPBYTE) value.c_str(),
                static_cast<DWORD>(value.length() + 1)
        );
        RegCloseKey(hKey);

        if (lSetStatus == ERROR_SUCCESS)
        {
            SendMessageTimeoutA(
                    HWND_BROADCAST,
                    WM_SETTINGCHANGE,
                    0,
                    reinterpret_cast<LPARAM>("Environment"),
                    SMTO_BLOCK,
                    100,
                    nullptr
            );
            return true;
        }
    }

    return std::unexpected{std::error_code{lOpenStatus, std::generic_category()}};
}

} // namespace portal::os
