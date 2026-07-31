//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#if defined(__GNUC__)
#if defined(__clang__)
#define PORTAL_COMPILER_CLANG
#else
#define PORTAL_COMPILER_GCC
#endif
#elif defined(_MSC_VER)
#define PORTAL_COMPILER_MSVC
#endif

#ifdef PORTAL_COMPILER_MSVC
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define PORTAL_FORCE_INLINE [[msvc::forceinline]] inline
#define PORTAL_EXPLICIT_STATIC static
#elif defined(__GNUC__)
#define PORTAL_FORCE_INLINE [[gnu::always_inline]] inline
#define PORTAL_EXPLICIT_STATIC static
#else
#define PORTAL_FORCE_INLINE inline
#define PORTAL_EXPLICIT_STATIC
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#define PORTAL_ARCHITECTURE_X86
#elif defined(__ARM_ARCH) || defined(_M_ARM) || defined(_M_ARM64)
#define PORTAL_ARCHITECTURE_ARM
#else
#error "Unsupported architecture"
#endif

#if defined(_WIN32)
#define PORTAL_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PORTAL_PLATFORM_MACOS
#elif defined(__linux__)
#define PORTAL_PLATFORM_LINUX
#else
#error "Unsupported platform"
#endif