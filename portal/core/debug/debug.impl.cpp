//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include "portal/core/log_macros.h"

module portal.core.debug;
import portal.core.log;
import portal.core.os.platform;

DECLARE_LOGGER("ASSERTION FAIL");

namespace portal::debug
{

bool print_assert_message(std::string_view message, const std::source_location loc)
{
    logger.error(message, loc);

    static std::unordered_set<std::string> ignored_asserts;
    auto key = std::format("{}:{}:{}", loc.file_name(), loc.line(), loc.column());
    if (ignored_asserts.contains(key))
        return false;

    if (os::is_debugger_connected())
    {
        const auto clicked = os::open_message_box(
                "Assertion",
                std::format(
                        "Assert failed at: {}({})\n{}()\n\"{}\"\nTry again to debug, Cancel to ignore this assert in "
                        "the future",
                        loc.file_name(),
                        loc.line(),
                        loc.function_name(),
                        message
                ),
                os::MessageBoxType::TryAgainContinueCancel,
                os::IconType::Error
        );

        if (clicked == os::MessageBoxButton::Cancel)
        {
            ignored_asserts.insert(std::move(key));
        }
        else if (clicked == os::MessageBoxButton::TryAgain)
        {
            return true;
        }
    }
    return false;
}

} // namespace portal::debug
