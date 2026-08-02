//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

export module portal.core.os;

import std;

export namespace portal::os
{

struct FileDialogFilterItem
{
    std::string_view name;
    std::string_view spec;
};

std::filesystem::path open_file_dialog(std::initializer_list<FileDialogFilterItem> in_filters = {});
std::filesystem::path open_folder_dialog(std::string_view initial_folder = "");
std::filesystem::path save_file_dialog(std::initializer_list<FileDialogFilterItem> in_filters = {});

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Platform Specific Functions ////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool show_file_in_explorer(const std::filesystem::path& path);
bool open_directory_in_explorer(const std::filesystem::path& path);
bool open_externally(const std::filesystem::path& path);

bool has_environment_variable(const std::string& name);
bool set_environment_variable(const std::string& name, const std::string& value);
std::string get_environment_variable(const std::string& name);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Program Data Paths /////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void set_binary_path(const std::filesystem::path& path);
std::filesystem::path get_binary_path();
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
 * Note that it is recommended to append "cache" after the program name to prevent conflicting with "StateDir" under Windows
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Global Platform Paths //////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

}