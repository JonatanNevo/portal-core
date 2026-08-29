//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

export module portal.core.os;

export import portal.core.os.platform;

import std;

export namespace portal::os
{

struct FileDialogFilterItem
{
    std::string_view name;
    std::string_view spec;
};

std::filesystem::path open_file_dialog(std::initializer_list<FileDialogFilterItem> in_filters = {});
std::filesystem::path save_file_dialog(
        std::string_view default_name,
        std::initializer_list<FileDialogFilterItem> in_filters = {}
);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Program Data Paths ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void set_binary_path(const std::filesystem::path& path);
std::filesystem::path get_binary_path();

} // namespace portal::os
