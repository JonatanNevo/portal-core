//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <cerrno>

#include "portal/core/log_macros.h"

module portal.core.filesystem;

import std;
import portal.core.log;

namespace portal::filesystem
{
DECLARE_LOGGER("portal.filesystem")

std::filesystem::path get_working_directory()
{
    return std::filesystem::current_path();
}

void set_working_directory(const std::filesystem::path& path)
{
    std::filesystem::current_path(path);
}

std::expected<void, std::error_code> create_directory(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto abs_path = std::filesystem::absolute(path);
    std::filesystem::create_directories(abs_path, ec);
    if (ec)
    {
        LOGGER_ERROR("{}: Failed to create directory", abs_path.string(), ec.message());
        return std::unexpected{ec};
    }
    return {};
}

std::expected<void, std::error_code> create_directory(const std::string& file_path)
{
    return portal::filesystem::create_directory(std::filesystem::path{file_path});
}

bool exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path);
}

bool exists(const std::string& file_path)
{
    return std::filesystem::exists(file_path);
}

[[nodiscard]] std::expected<void, std::error_code> remove(const std::filesystem::path& path)
{
    if (!portal::filesystem::exists(path))
        return std::unexpected{std::make_error_code(std::errc::no_such_file_or_directory)};

    std::error_code ec;
    if (portal::filesystem::is_directory(path))
        std::filesystem::remove_all(path, ec);
    else
        std::filesystem::remove(path, ec);

    if (ec)
    {
        LOGGER_ERROR("{}: Failed to remove file: {}", path.string(), ec.message());
        return std::unexpected{ec};
    }
    return {};
}

[[nodiscard]] std::expected<void, std::error_code> move(
        const std::filesystem::path& from,
        const std::filesystem::path& to
)
{
    if (portal::filesystem::exists(to))
        return std::unexpected{std::make_error_code(std::errc::file_exists)};

    std::error_code ec;
    std::filesystem::rename(from, to, ec);

    if (ec)
    {
        LOGGER_ERROR("{}: Failed to move file to {}: {}", from.string(), to.string(), ec.message());
        return std::unexpected{ec};
    }
    return {};
}

[[nodiscard]] std::expected<void, std::error_code> copy(
        const std::filesystem::path& from,
        const std::filesystem::path& to
)
{
    std::expected<void, std::error_code> res;
    if (portal::filesystem::is_directory(to) && !portal::filesystem::exists(to))
    {
        res = portal::filesystem::create_directory(to);
    }
    else if (!portal::filesystem::is_directory(to) && !portal::filesystem::exists(to.parent_path()))
    {
        res = portal::filesystem::create_directory(to.parent_path());
    }

    if (!res.has_value())
        return res;

    std::error_code ec;
    std::filesystem::copy(from, to, std::filesystem::copy_options::update_existing, ec);

    if (ec)
    {
        LOGGER_ERROR("{}: Failed to copy file to {}: {}", from.string(), to.string(), ec.message());
        return std::unexpected{ec};
    }
    return {};
}

[[nodiscard]] std::expected<void, std::error_code> rename(
        const std::filesystem::path& from,
        const std::filesystem::path& to
)
{
    return move(from, to);
}

[[nodiscard]] std::expected<void, std::error_code> rename_filename(
        const std::filesystem::path& path,
        const std::string& new_name
)
{
    const std::filesystem::path new_path =
            path.parent_path() / std::filesystem::path(new_name + path.extension().string());
    return portal::filesystem::rename(path, new_path);
}

[[nodiscard]] std::expected<FileStat, std::error_code> stat_file(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto fs_stat = std::filesystem::status(path, ec);
    if (ec)
    {
        LOGGER_WARN("{}: Failed to stat file: {}", path.string(), ec.message());
        return std::unexpected{ec};
    }

    auto size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        LOGGER_WARN("{}: Failed to get fuke file: {}", path.string(), ec.message());
        size = 0;
    }

    return FileStat(
            std::filesystem::is_regular_file(fs_stat),
            std::filesystem::is_directory(fs_stat),
            std::filesystem::last_write_time(path).time_since_epoch().count(),
            size
    );
}

bool is_file(const std::filesystem::path& path)
{
    return std::filesystem::is_regular_file(path);
}

bool is_directory(const std::filesystem::path& path)
{
    return std::filesystem::is_directory(path);
}

bool is_newer(const std::filesystem::path& path_a, const std::filesystem::path& path_b)
{
    return std::filesystem::last_write_time(path_a) > std::filesystem::last_write_time(path_b);
}

uint64_t get_last_write_time(const std::filesystem::path& path)
{
    if (path.filename().empty() || !portal::filesystem::exists(path))
        return 0;

    const auto stat = stat_file(path);
    return stat.transform([](auto stat_value) { return stat_value.last_write_time; }).value_or(0);
}

std::filesystem::path get_unique_file_name(const std::filesystem::path& path)
{
    if (!portal::filesystem::exists(path))
        return path;

    int counter = 0;
    while (true)
    {
        ++counter;
        std::string new_file_name = std::format("{} ({:02})", path.stem().string(), counter);

        if (path.has_extension())
            new_file_name = std::format("{}{}", new_file_name, path.extension().string());

        if (!portal::filesystem::exists(path.parent_path() / new_file_name))
            return path.parent_path() / new_file_name;
    }
}

std::expected<void, std::error_code> write_file(
        const std::filesystem::path& path,
        const std::span<const std::byte>& buffer,
        size_t offset
)
{
    const auto abs_path = std::filesystem::absolute(path);

    // create directory if it doesn't exist
    const auto parent = abs_path.parent_path();
    if (!std::filesystem::exists(parent))
        if (auto res = portal::filesystem::create_directory(parent); !res.has_value())
            return std::unexpected{res.error()};


    const auto mode = offset == 0 || !std::filesystem::exists(abs_path)
                              ? std::ios::binary | std::ios::trunc
                              : std::ios::binary | std::ios::in | std::ios::out;
    std::ofstream file(abs_path, mode);

    if (!file.is_open())
    {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        LOGGER_ERROR("{}: Failed to open file for writing: {}", abs_path.string(), ec.message());
        return std::unexpected{ec};
    }

    file.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    file.write(
            reinterpret_cast<const std::ostream::char_type*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
    );
    file.close();

    return {};
}

std::expected<void, std::error_code> write_file(
        const std::filesystem::path& path,
        const std::string& data,
        size_t offset
)
{
    return write_file(path, std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()}, offset);
}

std::expected<std::vector<std::byte>, std::error_code> read_chunk(
        const std::filesystem::path& path,
        size_t offset,
        size_t count
)
{
    const auto abs_path = std::filesystem::absolute(path);

    std::ifstream file(abs_path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        LOGGER_ERROR("{}: Failed to open file for reading: {}", abs_path.string(), ec.message());
        return std::unexpected{ec};
    }

    auto stat = stat_file(path);
    if (!stat.has_value())
    {
        return std::unexpected{stat.error()};
    }

    auto size = stat->size;

    if (offset + count > size)
    {
        LOGGER_ERROR(
                "{}: Requested read chunk ({} + {}) is bigger than size: ({})",
                path.string(),
                offset,
                count,
                size
        );
        return std::unexpected{std::make_error_code(std::errc::result_out_of_range)};
    }

    std::vector<std::byte> buffer(count);
    file.seekg(static_cast<std::streamsize>(offset), std::ios::beg);
    file.read(reinterpret_cast<std::istream::char_type*>(buffer.data()), static_cast<std::streamsize>(count));
    return buffer;
}

std::expected<std::vector<std::byte>, std::error_code> read_file_binary(const std::filesystem::path& path)
{
    const auto stat = stat_file(path);
    return stat.and_then([&](auto& stat_result) { return read_chunk(path, 0, stat_result.size); });
}

std::expected<std::string, std::error_code> read_file_string(const std::filesystem::path& path)
{
    return read_file_binary(path).transform(
            [](auto&& binary)
            {
                return binary | std::views::transform([](auto& byte) { return static_cast<const char>(byte); }) |
                       std::ranges::to<std::string>();
            }
    );
}
} // namespace portal::filesystem
