//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

export module portal.core.debug;

import std;
import portal.core.log;

export namespace portal::debug
{
template<typename... Args>
bool print_assert_message(detail::log_format_string_t<Args...> format, Args&&... args)
{
    auto message = std::format(format.fmt, std::make_format_args(std::forward<Args...>(args)...));
    return print_assert_message(message, format.loc);
}

bool print_assert_message(std::string_view message, std::source_location loc = std::source_location::current());
} // namespace portal::debug
