//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include "portal/core/core_includes.h"

export module portal.core.util:config;

import std.compat;

export namespace portal
{
enum class Platform : uint8_t
{
    Windows,
    MacOS,
    Linux
};

#ifdef PORTAL_PLATFORM_WINDOWS
constexpr auto PORTAL_PLATFORM = portal::Platform::Windows;
#elifdef PORTAL_PLATFORM_MACOS
constexpr auto PORTAL_PLATFORM = portal::Platform::MacOS;
#elifdef PORTAL_PLATFORM_LINUX
constexpr auto PORTAL_PLATFORM = portal::Platform::Linux;
#endif

enum class Compiler : uint8_t
{
    GCC,
    Clang,
    MSVC
};

enum class Architecture : uint8_t
{
    Arm,
    x86
};

#ifdef PORTAL_COMPILER_CLANG
constexpr auto PORTAL_COMPILER = Compiler::Clang;
#elifdef PORTAL_COMPILER_GCC
constexpr auto PORTAL_COMPILER = Compiler::GCC;
#elifdef PORTAL_COMPILER_MSVC
constexpr auto PORTAL_COMPILER = Compiler::MSVC;
#endif

#ifdef PORTAL_ARCHITECTURE_X86
constexpr auto PORTAL_ARCHITECTURE = portal::Architecture::x86;
#elifdef PORTAL_ARCHITECTURE_ARM
constexpr auto PORTAL_ARCHITECTURE = portal::Architecture::Arm;
#endif


enum class BuildConfiguration : uint8_t
{
    Debug,
    Development,
    Dist
};

/* Build Configuration */
#ifdef PORTAL_DEBUG
constexpr auto PORTAL_BUILD = portal::BuildConfiguration::Debug;
#elifdef PORTAL_DEVELOPMENT
constexpr auto PORTAL_BUILD = portal::BuildConfiguration::Development;
#elif defined(PORTAL_DIST)
constexpr auto PORTAL_BUILD = portal::BuildConfiguration::Dist;
#else
#error Undefined configuration
#endif
} // namespace portal
