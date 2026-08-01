//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>

export module portal.core.log:formatters;

import std;
import glm;

export namespace portal
{
class source_location_flag_formatter final : public spdlog::custom_flag_formatter
{
private:
    size_t width_;

public:
    explicit source_location_flag_formatter(const size_t width = 30) : width_(width) {}

    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
    {
        if (msg.source.filename && msg.source.line > 0)
        {
            // Extract filename from a full path manually
            const std::string full_path = msg.source.filename;
            std::string filename = full_path;

            // Find last occurrence of path separator
            const auto last_slash = full_path.find_last_of("/\\");
            if (last_slash != std::string::npos)
            {
                filename = full_path.substr(last_slash + 1);
            }

            std::string location = fmt::format("{}:{}", filename, msg.source.line);

            // Pad to specified width, left-aligned
            if (location.length() < width_)
            {
                location += std::string(width_ - location.length(), ' ');
            }

            dest.append(location.data(), location.data() + location.size());
        }
        else
        {
            // Fallback: pad with spaces
            const std::string empty_location(width_, ' ');
            dest.append(empty_location.data(), empty_location.data() + empty_location.size());
        }
    }

    [[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<source_location_flag_formatter>(width_);
    }
};
}

export namespace fmt
{
template <>
struct formatter<glm::vec2>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::vec2& vec, FormatContext& ctx) const
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(), "({:.3f}, {:.3f})", vec.x, vec.y)
                   : fmt::format_to(ctx.out(), "({:.3e}, {:.3e})", vec.x, vec.y);
    }
};

template <>
struct formatter<glm::vec3>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::vec3& vec, FormatContext& ctx) const
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z)
                   : fmt::format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z);
    }
};

template <>
struct formatter<glm::vec4>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::vec4& vec, FormatContext& ctx) const
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z, vec.w)
                   : fmt::format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z, vec.w);
    }
};

template <>
struct formatter<glm::mat2>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::mat2& mat, FormatContext& ctx) const
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(),
                         "[({:.3f}, {:.3f}), ({:.3f}, {:.3f})]",
                         mat[0][0], mat[0][1],
                         mat[1][0], mat[1][1])
                   : fmt::format_to(ctx.out(),
                         "[({:.3e}, {:.3e}), ({:.3e}, {:.3e})]",
                         mat[0][0], mat[0][1],
                         mat[1][0], mat[1][1]);
    }
};

template <>
struct formatter<glm::mat3>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::mat3& mat, FormatContext& ctx) const
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(),
                         "[({:.3f}, {:.3f}, {:.3f}), ({:.3f}, {:.3f}, {:.3f}), ({:.3f}, {:.3f}, {:.3f})]",
                         mat[0][0], mat[0][1], mat[0][2],
                         mat[1][0], mat[1][1], mat[1][2],
                         mat[2][0], mat[2][1], mat[2][2])
                   : fmt::format_to(ctx.out(),
                         "[({:.3e}, {:.3e}, {:.3e}), ({:.3e}, {:.3e}, {:.3e}), ({:.3e}, {:.3e}, {:.3e})]",
                         mat[0][0], mat[0][1], mat[0][2],
                         mat[1][0], mat[1][1], mat[1][2],
                         mat[2][0], mat[2][1], mat[2][2]);
    }
};

template <>
struct formatter<glm::mat4>
{
    char presentation = 'f';

    constexpr auto parse(const format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const glm::mat4& mat, FormatContext& ctx) const
    {
        return presentation == 'f'
                   ? fmt::format_to(ctx.out(),
                         "[({:.3f}, {:.3f}, {:.3f}, {:.3f}), ({:.3f}, {:.3f}, {:.3f}, {:.3f}), ({:.3f}, {:.3f}, {:.3f}, {:.3f}), ({:.3f}, {:.3f}, {:.3f}, {:.3f})]",
                         mat[0][0], mat[0][1], mat[0][2], mat[0][3],
                         mat[1][0], mat[1][1], mat[1][2], mat[1][3],
                         mat[2][0], mat[2][1], mat[2][2], mat[2][3],
                         mat[3][0], mat[3][1], mat[3][2], mat[3][3])
                   : fmt::format_to(ctx.out(),
                         "[({:.3e}, {:.3e}, {:.3e}, {:.3e}), ({:.3e}, {:.3e}, {:.3e}, {:.3e}), ({:.3e}, {:.3e}, {:.3e}, {:.3e}), ({:.3e}, {:.3e}, {:.3e}, {:.3e})]",
                         mat[0][0], mat[0][1], mat[0][2], mat[0][3],
                         mat[1][0], mat[1][1], mat[1][2], mat[1][3],
                         mat[2][0], mat[2][1], mat[2][2], mat[2][3],
                         mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
    }
};

}