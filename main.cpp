//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

import std;
import portal.core;

int main()
{
    try
    {
        portal::os::open_message_box(
                "Hello World",
                "This is a message box",
                portal::os::MessageBoxType::YesNoCancel,
                portal::os::IconType::Error
        );
        portal::os::open_notification_popup(
                "This is a notification popup",
                "wow so message",
                portal::os::IconType::Info
        );
    }
    catch (...)
    {
        std::cout << "Unhandled exception\n";
    }
}
