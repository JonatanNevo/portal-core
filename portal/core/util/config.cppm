//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include "portal/core/core_includes.h"

export module portal.core.util:config;

export namespace portal
{
enum class Platform
{
    Windows,
    MacOS,
    Linux
};

#if defined(PORTAL_PLATFORM_WINDOWS)
constexpr auto PORTAL_PLATFORM = portal::Platform::Windows;
#elif defined(PORTAL_PLATFORM_MACOS)
constexpr auto PORTAL_PLATFORM = portal::Platform::MacOS;
#elif defined(PORTAL_PLATFORM_LINUX)
constexpr auto PORTAL_PLATFORM = portal::Platform::Linux;
#endif

enum class Compiler
{
    GCC,
    Clang,
    MSVC
};

enum class Architecture
{
    Arm,
    x86
};

#if defined(PORTAL_COMPILER_CLANG)
constexpr auto PORTAL_COMPILER = Compiler::Clang;
#elif defined(PORTAL_COMPILER_GCC)
constexpr auto PORTAL_COMPILER = Compiler::GCC;
#elif defined(PORTAL_COMPILER_MSVC)
constexpr auto PORTAL_COMPILER = Compiler::MSVC;
#endif

#if defined(PORTAL_ARCHITECTURE_X86)
constexpr auto PORTAL_ARCHITECTURE = portal::Architecture::x86;
#elif defined(PORTAL_ARCHITECTURE_ARM)
constexpr auto PORTAL_ARCHITECTURE = portal::Architecture::Arm;
#endif


enum class BuildConfiguration
{
    Debug,
    Development,
    Dist
};

/* Build Configuration */
#if defined(PORTAL_DEBUG)
constexpr auto PORTAL_BUILD = portal::BuildConfiguration::Debug;
#elif defined(PORTAL_DEVELOPMENT)
constexpr auto PORTAL_BUILD = portal::BuildConfiguration::Development;
#elif defined(PORTAL_DIST)
constexpr auto PORTAL_BUILD = portal::BuildConfiguration::Dist;
#else
#error Undefined configuration
#endif
} // namespace portal
