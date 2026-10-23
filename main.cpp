//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

import std;
import portal.core;
#include "portal/core/debug_macros.h"

int main()
{
    try
    {
        PORTAL_ASSERT(false, "ho shit");
    }
    catch (...)
    {
        std::cout << "Unhandled exception\n";
    }
}
