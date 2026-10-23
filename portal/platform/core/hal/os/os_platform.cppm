//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
export module portal.core.os.platform;

import std.compat;
import portal.core.concurrency;

export namespace portal::os
{

/**
 * A resolved command line: an executable to locate on PATH, plus its arguments
 * (argv[1..]). Built by the platform "open ..." functions and handed to
 * run_process.
 */
struct ProcessCommand
{
    std::string executable;
    std::vector<std::string> arguments;

    bool operator==(const ProcessCommand&) const = default;
};

/**
 * The callable that actually launches a process. Returns true when the process
 * ran to completion with exit code 0.
 *
 * Install one with set_process_runner to intercept the commands that
 * open_externally / open_directory_in_explorer / show_file_in_explorer would
 * run, without spawning anything (see the os_platform unit tests).
 */
using ProcessRunner = std::function<bool(const ProcessCommand& command)>;

/**
 * Launches a command and waits for it to exit, returning true only on a clean
 * exit with status 0. Each platform provides its own implementation.
 *
 * When a runner has been installed via set_process_runner it is used instead of
 * the platform default; this is how the "open ..." functions below are unit
 * tested without spawning anything.
 */
bool run_process(const ProcessCommand& command);

/**
 * Overrides the launcher used by run_process. Pass a null runner to restore the
 * platform default. Not thread-safe: intended for tests and one-off setup.
 */
void set_process_runner(ProcessRunner runner);

/**
 * The result of a finished process: its exit code together with whatever it
 * wrote to stdout (trailing newlines trimmed). Produced by run_process_captured.
 */
struct ProcessOutput
{
    int exit_code = -1;
    std::string output;

    bool operator==(const ProcessOutput&) const = default;
};

/**
 * A process launcher that also reports the child's stdout. Returns std::nullopt
 * when the executable could not be spawned (for example, it is not installed).
 * Mirrors ProcessRunner; install one with set_captured_process_runner.
 */
using CapturedProcessRunner = std::function<std::optional<ProcessOutput>(const ProcessCommand& command)>;

/**
 * Runs a command to completion and returns its exit code plus captured stdout,
 * or std::nullopt when it could not be launched. open_message_box uses this to
 * read back which button the user pressed. Each platform provides its own
 * implementation; a runner installed via set_captured_process_runner overrides
 * it.
 */
std::optional<ProcessOutput> run_process_captured(const ProcessCommand& command);

/**
 * Overrides the launcher used by run_process_captured. Pass a null runner to
 * restore the platform default. Not thread-safe: intended for tests.
 */
void set_captured_process_runner(CapturedProcessRunner runner);

/**
 * Opens `path` with the desktop's default handler (xdg-open / `open` / the shell
 * association). Returns false for an empty or unresolvable path.
 */
bool open_externally(const std::filesystem::path& path);

/**
 * Opens `path` (which must be a directory) in the system file manager.
 */
bool open_directory_in_explorer(const std::filesystem::path& path);

/**
 * Reveals `path` in the system file manager, selecting the item inside its
 * parent folder. Directories are opened directly.
 */
bool show_file_in_explorer(const std::filesystem::path& path);

/// This class provides access to environment variables
/// and some general system information.
class Environment
{
public:
    /**
     * Returns the value of the environment variable `name`, or an empty string
     * if it is not defined.
     */
    [[nodiscard]] static std::optional<std::string> get(const std::string& name);

    /**
     * Returns the value of the environment variable `name` or the default_value if it does not exist
     */
    [[nodiscard]] static std::string get(const std::string& name, const std::string& default_value);

    /**
     * returns true iff an environment variable with the given name exists
     */
    [[nodiscard]] static bool has(const std::string& name);

    /**
     * Sets the environment variable with the given name to the given value.
    */
    static std::expected<void, std::error_code> set(const std::string& name, const std::string& value);

private:
    static SpinLock lock;
};

enum class MessageBoxType : uint8_t
{
    OK,
    OKCancel,
    YesNo,
    YesNoCancel,
    TryAgainContinueCancel
};

enum class IconType : uint8_t
{
    Info,
    Warning,
    Error,
    Question
};

/**
 * The button the user pressed in a message box. A box that offers fewer buttons
 * can only ever return that subset (an OK box returns OK; a YesNo box returns
 * Yes or No). Cancel is also reported when the box is dismissed by the window
 * manager or no backend was available.
 */
enum class MessageBoxButton : uint8_t
{
    OK,
    Cancel,
    Yes,
    No,
    TryAgain,
    Continue
};

/**
 * Shows a transient desktop notification ("toast") with the given title, message
 * and icon. When no notification service is reachable it falls back to a modal
 * message box. Returns true when a backend accepted the request.
 */
bool open_notification_popup(std::string_view title, std::string_view message, IconType icon);

/**
 * Shows a modal message box and blocks until the user answers, returning the
 * button that was pressed. A dismissed box, or one shown when no backend is
 * available, reports OK for an OK box and Cancel for every other type.
 */
MessageBoxButton open_message_box(std::string_view title, std::string_view message, MessageBoxType type, IconType icon);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Program Data Paths ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path get_root_path();

/**
 * Retrieves the base folder for storing data files.
 *
 * On Windows this defaults to %APPDATA% (Roaming profile)
 * On Linux this defaults to ~/.local/share but can be configured by the user
 *
 * @return The base folder for storing program data.
 */
std::filesystem::path get_data_home(std::filesystem::path app_name = "");


/**
 * Retrieves the base folder for storing config files.
 *
 * On Windows this defaults to %APPDATA% (Roaming profile)
 * On Linux this defaults to ~/.config but can be configured by the user
 *
 * @return The base folder for storing config data.
 */
std::filesystem::path get_config_home(std::filesystem::path app_name = "");

/**
 * Retrieves the base folder for storing cache files.
 *
 * On Windows this defaults to %APPDATALOCAL%
 * On Linux this defaults to ~/.cache but can be configured by the user
 *
 * Note that it is recommended to append "cache" after the program name to prevent conflicting with "StateDir" under
 * Windows
 * @return The base folder for storing data that do not need to be backed up and might be deleted.
 */
std::filesystem::path get_cache_dir(std::filesystem::path app_name = "");

/**
 * Retrieves the base folder used for state files.
 *
 * On Windows this defaults to %APPDATALOCAL%
 * On Linux this defaults to ~/.local/state but can be configured by the user
 * On OS X this is the same as getDataHome()
 *
 * @return The base folder for storing data that do not need to be backed up but should not be regularly deleted either.
 */
std::filesystem::path get_state_dir(std::filesystem::path app_name = "");


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Global Platform Paths /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The folder that represents the desktop.
 * Normally you should try not to use this folder.
 *
 * @return Absolute path to the user's desktop
 */
std::filesystem::path get_desktop_folder();

/**
 * The folder to store user documents to
 *
 * @return Absolute path to the "Documents" folder
 */
std::filesystem::path get_documents_folder();

/**
 * The folder where files are downloaded.
 *
 * @return Absolute path to the folder where files are downloaded to.
 */
std::filesystem::path get_download_folder();

/**
 * The folder for storing the user's pictures.
 *
 * @return Absolute path to the "Picture" folder
 */
std::filesystem::path get_pictures_folder();

/**
 * This returns the folder that can be used for sharing files with other users on the same system.
 *
 * @return Absolute path to the "Public" folder
 */
std::filesystem::path get_public_folder();

/**
 * The folder where music is stored
 *
 * @return Absolute path to the music folder
 */
std::filesystem::path get_music_folder();

/**
 * The folder where video is stored
 *
 * @return Absolute path to the video folder
 */
std::filesystem::path get_video_folder();

/**
 * Checks whether a debugger is currently attached to this process.
 *
 * @return true if a debugger is attached, false otherwise (or if it cannot be determined)
 */
bool is_debugger_connected();
} // namespace portal::os
