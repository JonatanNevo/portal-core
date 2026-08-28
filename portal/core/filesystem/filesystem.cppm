//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

export module portal.core.filesystem;

import std.compat;

export namespace portal::filesystem
{
struct FileStat
{
    bool is_file{};
    bool is_directory{};
    uint64_t last_write_time{};
    size_t size{};
};

std::filesystem::path get_working_directory();

void set_working_directory(const std::filesystem::path& path);

[[nodiscard]] std::expected<void, std::error_code> create_directory(const std::filesystem::path& path);
[[nodiscard]] std::expected<void, std::error_code> create_directory(const std::string& file_path);

bool exists(const std::filesystem::path& path);
bool exists(const std::string& file_path);

[[nodiscard]] std::expected<void, std::error_code> remove(const std::filesystem::path& path);

[[nodiscard]] std::expected<void, std::error_code> move(
        const std::filesystem::path& from,
        const std::filesystem::path& to
);
[[nodiscard]] std::expected<void, std::error_code> copy(
        const std::filesystem::path& from,
        const std::filesystem::path& to
);
[[nodiscard]] std::expected<void, std::error_code> rename(
        const std::filesystem::path& from,
        const std::filesystem::path& to
);
[[nodiscard]] std::expected<void, std::error_code> rename_filename(
        const std::filesystem::path& path,
        const std::string& new_name
);

[[nodiscard]] std::expected<FileStat, std::error_code> stat_file(const std::filesystem::path& path);
bool is_file(const std::filesystem::path& path);
bool is_directory(const std::filesystem::path& path);

bool is_newer(const std::filesystem::path& path_a, const std::filesystem::path& path_b);
uint64_t get_last_write_time(const std::filesystem::path& path);

std::filesystem::path get_unique_file_name(const std::filesystem::path& path);

[[nodiscard]] std::expected<void, std::error_code> write_file(
        const std::filesystem::path& path,
        const std::span<const std::byte>& buffer,
        size_t offset = 0
);
[[nodiscard]] std::expected<void, std::error_code> write_file(
        const std::filesystem::path& path,
        const std::string& data,
        size_t offset = 0
);

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code> read_chunk(
        const std::filesystem::path& path,
        size_t offset,
        size_t count
);
[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code> read_file_binary(
        const std::filesystem::path& path
);
[[nodiscard]] std::expected<std::string, std::error_code> read_file_string(const std::filesystem::path& path);
} // namespace portal::filesystem
