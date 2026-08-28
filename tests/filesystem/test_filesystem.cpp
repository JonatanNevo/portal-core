//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

import portal.core.filesystem;
import std;

namespace
{
std::filesystem::path make_temp_dir()
{
    std::random_device rd;
    const auto suffix = std::format("{:016x}", (static_cast<uint64_t>(rd()) << 32) | static_cast<uint64_t>(rd()));
    auto dir = std::filesystem::temp_directory_path() / std::format("portal_fs_test_{}", suffix);
    std::filesystem::create_directories(dir);
    return dir;
}

std::span<const std::byte> as_bytes(const std::string& data)
{
    return {reinterpret_cast<const std::byte*>(data.data()), data.size()};
}

struct TempDirGuard
{
    std::filesystem::path path{make_temp_dir()};

    TempDirGuard() = default;

    ~TempDirGuard()
    {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
} // namespace

SCENARIO("Working directory can be read and changed", "[unit][core][filesystem]")
{
    GIVEN("the current working directory")
    {
        const auto original = portal::filesystem::get_working_directory();
        TempDirGuard temp;

        THEN("set_working_directory changes it and get_working_directory reflects that")
        {
            portal::filesystem::set_working_directory(temp.path);
            REQUIRE(std::filesystem::equivalent(portal::filesystem::get_working_directory(), temp.path));

            portal::filesystem::set_working_directory(original);
            REQUIRE(std::filesystem::equivalent(portal::filesystem::get_working_directory(), original));
        }
    }
}

SCENARIO("create_directory creates nested directories", "[unit][core][filesystem]")
{
    GIVEN("a temp directory")
    {
        TempDirGuard temp;
        const auto nested = temp.path / "a" / "b" / "c";

        WHEN("create_directory is called with a path")
        {
            const auto result = portal::filesystem::create_directory(nested);

            THEN("it succeeds and the directory tree exists")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::filesystem::is_directory(nested));
            }
        }

        WHEN("create_directory is called with a string")
        {
            const auto result = portal::filesystem::create_directory(nested.string());

            THEN("it succeeds and the directory tree exists")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::filesystem::is_directory(nested));
            }
        }

        WHEN("create_directory is called on an already-existing directory")
        {
            REQUIRE(portal::filesystem::create_directory(nested).has_value());
            const auto result = portal::filesystem::create_directory(nested);

            THEN("it still succeeds")
            {
                REQUIRE(result.has_value());
            }
        }
    }
}

SCENARIO("exists reports presence of files and directories", "[unit][core][filesystem]")
{
    GIVEN("a temp directory with a file in it")
    {
        TempDirGuard temp;
        const auto file = temp.path / "file.txt";
        REQUIRE(portal::filesystem::write_file(file, std::string{"hi"}).has_value());

        THEN("exists returns true for the file and directory, path or string overload")
        {
            REQUIRE(portal::filesystem::exists(file));
            REQUIRE(portal::filesystem::exists(file.string()));
            REQUIRE(portal::filesystem::exists(temp.path));
        }

        THEN("exists returns false for a missing path")
        {
            const auto missing = temp.path / "missing.txt";
            REQUIRE_FALSE(portal::filesystem::exists(missing));
            REQUIRE_FALSE(portal::filesystem::exists(missing.string()));
        }
    }
}

SCENARIO("remove deletes files and directories", "[unit][core][filesystem]")
{
    GIVEN("a temp directory")
    {
        TempDirGuard temp;

        WHEN("removing an existing file")
        {
            const auto file = temp.path / "file.txt";
            REQUIRE(portal::filesystem::write_file(file, std::string{"hi"}).has_value());

            const auto result = portal::filesystem::remove(file);

            THEN("it succeeds and the file is gone")
            {
                REQUIRE(result.has_value());
                REQUIRE_FALSE(std::filesystem::exists(file));
            }
        }

        WHEN("removing a directory with contents")
        {
            const auto dir = temp.path / "subdir";
            REQUIRE(portal::filesystem::create_directory(dir).has_value());
            REQUIRE(portal::filesystem::write_file(dir / "inner.txt", std::string{"hi"}).has_value());

            const auto result = portal::filesystem::remove(dir);

            THEN("it recursively removes the directory")
            {
                REQUIRE(result.has_value());
                REQUIRE_FALSE(std::filesystem::exists(dir));
            }
        }

        WHEN("removing a path that does not exist")
        {
            const auto result = portal::filesystem::remove(temp.path / "missing.txt");

            THEN("it fails with no_such_file_or_directory")
            {
                REQUIRE_FALSE(result.has_value());
                REQUIRE(result.error() == std::make_error_code(std::errc::no_such_file_or_directory));
            }
        }
    }
}

SCENARIO("move relocates a file", "[unit][core][filesystem]")
{
    GIVEN("a temp directory with a source file")
    {
        TempDirGuard temp;
        const auto from = temp.path / "from.txt";
        REQUIRE(portal::filesystem::write_file(from, std::string{"payload"}).has_value());

        WHEN("moving to a destination that does not exist")
        {
            const auto to = temp.path / "to.txt";
            const auto result = portal::filesystem::move(from, to);

            THEN("the file is relocated")
            {
                REQUIRE(result.has_value());
                REQUIRE_FALSE(std::filesystem::exists(from));
                REQUIRE(std::filesystem::exists(to));
                REQUIRE(portal::filesystem::read_file_string(to).value() == "payload");
            }
        }

        WHEN("moving to a destination that already exists")
        {
            const auto to = temp.path / "to.txt";
            REQUIRE(portal::filesystem::write_file(to, std::string{"other"}).has_value());

            const auto result = portal::filesystem::move(from, to);

            THEN("it fails with file_exists and leaves both files untouched")
            {
                REQUIRE_FALSE(result.has_value());
                REQUIRE(result.error() == std::make_error_code(std::errc::file_exists));
                REQUIRE(std::filesystem::exists(from));
            }
        }
    }
}

SCENARIO("copy duplicates a file", "[unit][core][filesystem]")
{
    GIVEN("a temp directory with a source file")
    {
        TempDirGuard temp;
        const auto from = temp.path / "from.txt";
        REQUIRE(portal::filesystem::write_file(from, std::string{"payload"}).has_value());

        WHEN("copying to a new path in the same directory")
        {
            const auto to = temp.path / "copy.txt";
            const auto result = portal::filesystem::copy(from, to);

            THEN("both files exist with the same content")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::filesystem::exists(from));
                REQUIRE(std::filesystem::exists(to));
                REQUIRE(portal::filesystem::read_file_string(to).value() == "payload");
            }
        }

        WHEN("copying to a destination whose parent directory does not exist yet")
        {
            const auto to = temp.path / "nested" / "sub" / "copy.txt";
            const auto result = portal::filesystem::copy(from, to);

            THEN("the parent directories are created and the file is copied")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::filesystem::exists(to));
                REQUIRE(portal::filesystem::read_file_string(to).value() == "payload");
            }
        }

        WHEN("copying a source that does not exist")
        {
            const auto missing_from = temp.path / "missing.txt";
            const auto to = temp.path / "copy.txt";
            const auto result = portal::filesystem::copy(missing_from, to);

            THEN("it fails")
            {
                REQUIRE_FALSE(result.has_value());
                REQUIRE_FALSE(std::filesystem::exists(to));
            }
        }
    }
}

SCENARIO("rename behaves like move", "[unit][core][filesystem]")
{
    GIVEN("a temp directory with a source file")
    {
        TempDirGuard temp;
        const auto from = temp.path / "from.txt";
        REQUIRE(portal::filesystem::write_file(from, std::string{"payload"}).has_value());
        const auto to = temp.path / "to.txt";

        THEN("the file is relocated to the new path")
        {
            const auto result = portal::filesystem::rename(from, to);
            REQUIRE(result.has_value());
            REQUIRE_FALSE(std::filesystem::exists(from));
            REQUIRE(std::filesystem::exists(to));
        }
    }
}

SCENARIO("rename_filename renames a file while preserving its extension", "[unit][core][filesystem]")
{
    GIVEN("a temp directory with a source file")
    {
        TempDirGuard temp;
        const auto from = temp.path / "original.txt";
        REQUIRE(portal::filesystem::write_file(from, std::string{"payload"}).has_value());

        WHEN("renaming to a new base name")
        {
            const auto result = portal::filesystem::rename_filename(from, "renamed");

            THEN("the file exists under the new name with the same extension")
            {
                REQUIRE(result.has_value());
                REQUIRE_FALSE(std::filesystem::exists(from));
                REQUIRE(std::filesystem::exists(temp.path / "renamed.txt"));
            }
        }

        WHEN("the target name is already taken")
        {
            const auto taken = temp.path / "taken.txt";
            REQUIRE(portal::filesystem::write_file(taken, std::string{"other"}).has_value());

            const auto result = portal::filesystem::rename_filename(from, "taken");

            THEN("it fails and the original file is left untouched")
            {
                REQUIRE_FALSE(result.has_value());
                REQUIRE(std::filesystem::exists(from));
            }
        }
    }
}

SCENARIO("stat_file reports metadata about a file", "[unit][core][filesystem]")
{
    GIVEN("a temp directory with a file of known content")
    {
        TempDirGuard temp;
        const auto file = temp.path / "file.txt";
        const std::string content = "hello world";
        REQUIRE(portal::filesystem::write_file(file, content).has_value());

        WHEN("stat_file is called on the file")
        {
            const auto result = portal::filesystem::stat_file(file);

            THEN("it reports it as a file with the correct size")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->is_file);
                REQUIRE_FALSE(result->is_directory);
                REQUIRE(result->size == content.size());
            }
        }

        WHEN("stat_file is called on the directory")
        {
            const auto result = portal::filesystem::stat_file(temp.path);

            THEN("it reports it as a directory")
            {
                REQUIRE(result.has_value());
                REQUIRE_FALSE(result->is_file);
                REQUIRE(result->is_directory);
            }
        }

        WHEN("stat_file is called on a path that does not exist")
        {
            const auto result = portal::filesystem::stat_file(temp.path / "missing.txt");

            THEN("it fails")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("is_file and is_directory classify paths correctly", "[unit][core][filesystem]")
{
    GIVEN("a temp directory with a file inside it")
    {
        TempDirGuard temp;
        const auto file = temp.path / "file.txt";
        REQUIRE(portal::filesystem::write_file(file, std::string{"hi"}).has_value());

        THEN("the file is classified as a file, not a directory")
        {
            REQUIRE(portal::filesystem::is_file(file));
            REQUIRE_FALSE(portal::filesystem::is_directory(file));
        }

        THEN("the directory is classified as a directory, not a file")
        {
            REQUIRE(portal::filesystem::is_directory(temp.path));
            REQUIRE_FALSE(portal::filesystem::is_file(temp.path));
        }

        THEN("a missing path is classified as neither")
        {
            const auto missing = temp.path / "missing.txt";
            REQUIRE_FALSE(portal::filesystem::is_file(missing));
            REQUIRE_FALSE(portal::filesystem::is_directory(missing));
        }
    }
}

SCENARIO("is_newer compares last-write times of two files", "[unit][core][filesystem]")
{
    GIVEN("two files with distinct write times")
    {
        TempDirGuard temp;
        const auto older = temp.path / "older.txt";
        const auto newer = temp.path / "newer.txt";
        REQUIRE(portal::filesystem::write_file(older, std::string{"a"}).has_value());
        REQUIRE(portal::filesystem::write_file(newer, std::string{"b"}).has_value());

        const auto now = std::filesystem::file_time_type::clock::now();
        std::filesystem::last_write_time(older, now - std::chrono::hours(1));
        std::filesystem::last_write_time(newer, now);

        THEN("is_newer reports the more recently written file as newer")
        {
            REQUIRE(portal::filesystem::is_newer(newer, older));
            REQUIRE_FALSE(portal::filesystem::is_newer(older, newer));
        }
    }
}

SCENARIO("get_last_write_time reports a file's write time or zero", "[unit][core][filesystem]")
{
    GIVEN("a temp directory")
    {
        TempDirGuard temp;

        WHEN("the file exists")
        {
            const auto file = temp.path / "file.txt";
            REQUIRE(portal::filesystem::write_file(file, std::string{"hi"}).has_value());

            THEN("it matches stat_file's last_write_time")
            {
                const auto stat = portal::filesystem::stat_file(file);
                REQUIRE(stat.has_value());
                REQUIRE(portal::filesystem::get_last_write_time(file) == stat->last_write_time);
                REQUIRE(portal::filesystem::get_last_write_time(file) != 0);
            }
        }

        WHEN("the file does not exist")
        {
            THEN("it returns zero")
            {
                REQUIRE(portal::filesystem::get_last_write_time(temp.path / "missing.txt") == 0);
            }
        }

        WHEN("the path has an empty filename")
        {
            THEN("it returns zero")
            {
                REQUIRE(portal::filesystem::get_last_write_time(temp.path / "") == 0);
            }
        }
    }
}

SCENARIO("get_unique_file_name avoids collisions", "[unit][core][filesystem]")
{
    GIVEN("a temp directory")
    {
        TempDirGuard temp;

        WHEN("the requested path does not exist")
        {
            const auto path = temp.path / "file.txt";

            THEN("the same path is returned unchanged")
            {
                REQUIRE(portal::filesystem::get_unique_file_name(path) == path);
            }
        }

        WHEN("the requested path already exists")
        {
            const auto path = temp.path / "file.txt";
            REQUIRE(portal::filesystem::write_file(path, std::string{"hi"}).has_value());

            THEN("a suffixed variant is returned")
            {
                const auto unique = portal::filesystem::get_unique_file_name(path);
                REQUIRE(unique != path);
                REQUIRE(unique == temp.path / "file (01).txt");
            }

            AND_WHEN("the first suffixed variant also exists")
            {
                REQUIRE(portal::filesystem::write_file(temp.path / "file (01).txt", std::string{"hi"}).has_value());

                THEN("the next suffix is returned")
                {
                    REQUIRE(portal::filesystem::get_unique_file_name(path) == temp.path / "file (02).txt");
                }
            }
        }

        WHEN("the requested path has no extension")
        {
            const auto path = temp.path / "file";
            REQUIRE(portal::filesystem::write_file(path, std::string{"hi"}).has_value());

            THEN("a suffixed variant without an extension is returned")
            {
                REQUIRE(portal::filesystem::get_unique_file_name(path) == temp.path / "file (01)");
            }
        }
    }
}

SCENARIO("write_file writes binary and string data", "[unit][core][filesystem]")
{
    GIVEN("a temp directory")
    {
        TempDirGuard temp;

        WHEN("writing string data to a new file")
        {
            const auto file = temp.path / "file.txt";
            const auto result = portal::filesystem::write_file(file, std::string{"hello"});

            THEN("the file contains the written data")
            {
                REQUIRE(result.has_value());
                REQUIRE(portal::filesystem::read_file_string(file).value() == "hello");
            }
        }

        WHEN("writing binary data via the span overload")
        {
            const auto file = temp.path / "file.bin";
            const std::string data = "binary-payload";
            const auto result = portal::filesystem::write_file(file, as_bytes(data));

            THEN("the file contains the written bytes")
            {
                REQUIRE(result.has_value());
                REQUIRE(portal::filesystem::read_file_string(file).value() == data);
            }
        }

        WHEN("writing to a path whose parent directory does not exist")
        {
            const auto file = temp.path / "nested" / "sub" / "file.txt";
            const auto result = portal::filesystem::write_file(file, std::string{"hello"});

            THEN("the parent directories are created and the write succeeds")
            {
                REQUIRE(result.has_value());
                REQUIRE(std::filesystem::exists(file));
            }
        }

        WHEN("writing at a non-zero offset into an existing file")
        {
            const auto file = temp.path / "file.txt";
            REQUIRE(portal::filesystem::write_file(file, std::string{"0123456789"}).has_value());

            const auto result = portal::filesystem::write_file(file, std::string{"XY"}, 3);

            THEN("only the targeted bytes are overwritten")
            {
                REQUIRE(result.has_value());
                REQUIRE(portal::filesystem::read_file_string(file).value() == "012XY56789");
            }
        }
    }
}

SCENARIO("read_chunk reads a byte range from a file", "[unit][core][filesystem]")
{
    GIVEN("a file with known content")
    {
        TempDirGuard temp;
        const auto file = temp.path / "file.txt";
        const std::string content = "0123456789";
        REQUIRE(portal::filesystem::write_file(file, content).has_value());

        WHEN("reading a valid range")
        {
            const auto result = portal::filesystem::read_chunk(file, 2, 4);

            THEN("the correct bytes are returned")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->size() == 4);
                const std::string text(reinterpret_cast<const char*>(result->data()), result->size());
                REQUIRE(text == "2345");
            }
        }

        WHEN("reading zero bytes")
        {
            const auto result = portal::filesystem::read_chunk(file, 3, 0);

            THEN("an empty buffer is returned")
            {
                REQUIRE(result.has_value());
                REQUIRE(result->empty());
            }
        }

        WHEN("reading a range that extends past the end of the file")
        {
            const auto result = portal::filesystem::read_chunk(file, 5, 100);

            THEN("it fails with result_out_of_range")
            {
                REQUIRE_FALSE(result.has_value());
                REQUIRE(result.error() == std::make_error_code(std::errc::result_out_of_range));
            }
        }

        WHEN("reading from a file that does not exist")
        {
            const auto result = portal::filesystem::read_chunk(temp.path / "missing.txt", 0, 1);

            THEN("it fails")
            {
                REQUIRE_FALSE(result.has_value());
            }
        }
    }
}

SCENARIO("read_file_binary and read_file_string read a whole file", "[unit][core][filesystem]")
{
    GIVEN("a file with known text content")
    {
        TempDirGuard temp;
        const auto file = temp.path / "file.txt";
        const std::string content = "the quick brown fox";
        REQUIRE(portal::filesystem::write_file(file, content).has_value());

        THEN("read_file_binary returns the exact bytes")
        {
            const auto result = portal::filesystem::read_file_binary(file);
            REQUIRE(result.has_value());
            REQUIRE(result->size() == content.size());
            const std::string text(reinterpret_cast<const char*>(result->data()), result->size());
            REQUIRE(text == content);
        }

        THEN("read_file_string returns the same content as a string")
        {
            const auto result = portal::filesystem::read_file_string(file);
            REQUIRE(result.has_value());
            REQUIRE(result.value() == content);
        }

        THEN("reading a missing file fails")
        {
            const auto result = portal::filesystem::read_file_string(temp.path / "missing.txt");
            REQUIRE_FALSE(result.has_value());
        }
    }
}
