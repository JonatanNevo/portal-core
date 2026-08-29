//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


module portal.core.os;

import std;
import native_file_dialog;

namespace portal::os
{

namespace
{

    // Owns the null-terminated strings that the NFD filter list points into. The
    // FileDialogFilterItem members are string_views, which are not guaranteed to be
    // null-terminated, so they must be copied before being handed to NFD.
    struct FilterStorage
    {
        std::vector<std::string> names;
        std::vector<std::string> specs;
        std::vector<nfd::nfdu8filteritem_t> items;
    };

    FilterStorage build_filters(std::initializer_list<FileDialogFilterItem> in_filters)
    {
        FilterStorage storage;
        storage.names.reserve(in_filters.size());
        storage.specs.reserve(in_filters.size());
        storage.items.reserve(in_filters.size());

        for (const auto& [name, spec]: in_filters)
        {
            storage.names.emplace_back(name);
            storage.specs.emplace_back(spec);
        }

        for (std::size_t i = 0; i < in_filters.size(); ++i)
        {
            storage.items.push_back({.name = storage.names[i].c_str(), .spec = storage.specs[i].c_str()});
        }

        return storage;
    }

} // namespace

std::filesystem::path open_file_dialog(std::initializer_list<FileDialogFilterItem> in_filters)
{
    if (nfd::NFD_Init() != nfd::NFD_OKAY)
    {
        return {};
    }

    FilterStorage filters = build_filters(in_filters);

    nfd::nfdu8char_t* out_path = nullptr;
    const nfd::nfdopendialogu8args_t args{
            .filterList = filters.items.empty() ? nullptr : filters.items.data(),
            .filterCount = static_cast<nfd::nfdfiltersize_t>(filters.items.size())
    };

    const nfd::nfdresult_t result = nfd::NFD_OpenDialogU8_With(&out_path, &args);
    if (result != nfd::NFD_OKAY)
    {
        nfd::NFD_Quit();
        return {};
    }

    std::filesystem::path result_path(out_path);
    nfd::NFD_FreePathU8(out_path);
    nfd::NFD_Quit();
    return result_path;
}

std::filesystem::path save_file_dialog(
        std::string_view default_name,
        std::initializer_list<FileDialogFilterItem> in_filters
)
{
    if (nfd::NFD_Init() != nfd::NFD_OKAY)
    {
        return {};
    }

    FilterStorage filters = build_filters(in_filters);
    const std::string default_name_str(default_name);

    nfd::nfdu8char_t* out_path = nullptr;
    const nfd::nfdsavedialogu8args_t args{
            .filterList = filters.items.empty() ? nullptr : filters.items.data(),
            .filterCount = static_cast<nfd::nfdfiltersize_t>(filters.items.size()),
            .defaultName = default_name_str.empty() ? nullptr : default_name_str.c_str()
    };

    const auto result = nfd::NFD_SaveDialogU8_With(&out_path, &args);
    if (result != nfd::NFD_OKAY)
    {
        nfd::NFD_Quit();
        return {};
    }

    std::filesystem::path result_path(out_path);
    nfd::NFD_FreePathU8(out_path);
    nfd::NFD_Quit();
    return result_path;
}

namespace
{
    // NOLINTNEXTLINE (cppcoreguidelines-avoid-non-const-global-variables)
    std::filesystem::path s_binary_path;
} // namespace

void set_binary_path(const std::filesystem::path& path)
{
    s_binary_path = path;
}

std::filesystem::path get_binary_path()
{
    return s_binary_path;
}

} // namespace portal::os
