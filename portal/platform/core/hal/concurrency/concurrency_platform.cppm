//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include "portal/core/core_includes.h"

export module portal.core.concurrency.platform;

#ifdef PORTAL_PLATFORM_WINDOWS
export import :windows;
#elifdef PORTAL_PLATFORM_MACOS
export import :macos;
#elifdef PORTAL_PLATFORM_LINUX
export import :linux;
#else
#error "Invalid Platform"
#endif


export namespace portal
{

#ifdef PORTAL_PLATFORM_WINDOWS
using Thread = WindowsThread;
#elifdef PORTAL_PLATFORM_MACOS
using Thread = MacOSThread;
#elifdef PORTAL_PLATFORM_LINUX
using Thread = LinuxThread;
#else
#error "Invalid Platform"
#endif

}

