//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#ifdef PORTAL_PROFILE
#include <algorithm>
#include <cstddef>

#include <fmt/format.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#include "portal/core/defines/preprocessor.h"

#define PORTAL_TRACE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define PORTAL_TRACE_FREE(ptr) TracyFree(ptr)
#define PORTAL_TRACE_REALLOC(old_ptr, new_ptr, size)                                                                   \
    TracyAlloc(new_ptr, size);                                                                                         \
    TracyFree(old_ptr)

#define PORTAL_PROF_ZONE_FUNC(...) ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#define PORTAL_PROF_ZONE(...) PORTAL_PROF_ZONE_FUNC(__VA_ARGS__)

#define PORTAL_PROF_ZONE_DYNAMIC_NAME(txt, size) ZoneName(txt, size)

// Formats a dynamic zone name into a call-site-local static buffer and applies it to the
// current zone. Must be invoked directly in the enclosing block (not inside its own {}) so
// the ZoneScoped it opens lives for the rest of that block. Fully compiles out, including
// the fmt::format_to_n call itself, when PORTAL_PROFILE is off.
#define PORTAL_PROF_ZONE_FMT(...) PORTAL_PROF_ZONE_FMT_IMPL(PORTAL_JOIN(_portal_prof_zone_buf_, __LINE__), __VA_ARGS__)
#define PORTAL_PROF_ZONE_FMT_IMPL(buf_name, ...)                                                                       \
    PORTAL_PROF_ZONE();                                                                                                \
    static thread_local char buf_name[1024];                                                                           \
    PORTAL_PROF_ZONE_DYNAMIC_NAME(                                                                                     \
            buf_name,                                                                                                  \
            std::min<size_t>(fmt::format_to_n(buf_name, sizeof(buf_name), __VA_ARGS__).size, sizeof(buf_name))         \
    )

// Non-scoped zone: name is an unquoted identifier, doubling as the ctx variable and (stringized) zone label.
#define PORTAL_PROF_ZONE_BEGIN(name) TracyCZoneN(name, #name, true)
#define PORTAL_PROF_ZONE_END(name) TracyCZoneEnd(name)

#define PORTAL_NAME_THREAD(name) tracy::SetThreadName(name)

#define PORTAL_PROF_LOCK(type, varname) TracyLockable(type, varname)

#define PORTAL_FRAME_MARK(...) FrameMark
#define PORTAL_FRAME_MARK_START(name) FrameMarkStart(name)
#define PORTAL_FRAME_MARK_END(name) FrameMarkEnd(name)

#else

#define PORTAL_TRACE_ALLOC(...)
#define PORTAL_TRACE_FREE(...)
#define PORTAL_TRACE_REALLOC(...)

#define PORTAL_PROF_ZONE(...)

#define PORTAL_PROF_ZONE_DYNAMIC_NAME(txt, size)

#define PORTAL_PROF_ZONE_FMT(...)

#define PORTAL_PROF_ZONE_BEGIN(name)
#define PORTAL_PROF_ZONE_END(name)

#define PORTAL_NAME_THREAD(...)

#define PORTAL_PROF_LOCK(type, varname) type varname

#define PORTAL_FRAME_MARK(...)
#define PORTAL_FRAME_MARK_START(name)
#define PORTAL_FRAME_MARK_END(name)

#endif
