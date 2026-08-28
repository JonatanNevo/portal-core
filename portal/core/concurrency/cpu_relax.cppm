//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include "portal/core/core_includes.h"

#if defined(PORTAL_ARCHITECTURE_X86)
#include <immintrin.h>
#endif

export module portal.core.concurrency:cpu_relax;

import std;
import portal.core.util;

export namespace portal
{

PORTAL_FORCE_INLINE void cpu_relax()
{
    if constexpr (PORTAL_COMPILER == Compiler::MSVC)
    {
        if constexpr (PORTAL_ARCHITECTURE == Architecture::x86)
        {
            _mm_pause();
            return;
        }
    }
    else
    {
        if constexpr (PORTAL_ARCHITECTURE == Architecture::x86)
        {
            __builtin_ia32_pause();
            return;
        }
        else if constexpr (PORTAL_ARCHITECTURE == Architecture::Arm)
        {
            __asm__ __volatile__("yield" ::: "memory");
            return;
        }
    }

    // Fallback
    std::this_thread::yield();
}
} // namespace portal
