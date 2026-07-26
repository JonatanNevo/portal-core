//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

export module portal.core:config;

export namespace portal
{
enum class Platform
{
    Windows,
    MacOS,
    Linux
};

constexpr auto PORTAL_PLATFORM = _PORTAL_PLATFORM;

enum class Compiler
{
    GCC,
    Clang,
    MSVC
};

#if defined(__GNUC__)
#if defined(__clang__)
constexpr auto PORTAL_COMPILER = Compiler::Clang;
#else
constexpr auto PORTAL_COMPILER = Compiler::GCC;
#endif
#elif defined(_MSC_VER)
constexpr auto PORTAL_COMPILER = Compiler::MSVC;
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
#elif defined(PORTAL_DIST
constexpr auto PORTAL_BUILD = portal::BuildConfiguration::Dist;
#else
#error Undefined configuration
#endif


}
