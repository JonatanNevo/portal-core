# This toolchain file is not meant to be used directly,
# but to be invoked by CMake preset and GitHub CI.
#
# This toolchain file configures for MSVC family of compiler.
#
# PORTAL_SANITIZER:
# This optional CMake parameter is not meant for public use and is subject to
# change.
# Possible values:
# - MaxSan: configures cl to use all available non-conflicting sanitizers.
#
# Note that in other toolchain files, TSan is also a possible value for
# PORTAL_SANITIZER, however, MSVC does not support thread sanitizer,
# thus this value is omitted.

include_guard(GLOBAL)

# sccache with MSVC requires embedded debug info (no separate PDB). Kept here
# so the setting applies even though the launcher is set in the root project.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
cmake_policy(SET CMP0141 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)

if(PORTAL_SANITIZER STREQUAL "MaxSan")
    # /Zi flag (add debug symbol) is needed when using address sanitizer
    # See C5072: https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-c5072
    set(SANITIZER_FLAGS "/fsanitize=address /Zi")
endif()

set(CMAKE_CXX_FLAGS_DEBUG_INIT "/EHsc /permissive- ${SANITIZER_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG_INIT "/EHsc /permissive- ${SANITIZER_FLAGS}")

set(RELEASE_FLAGS "/EHsc /permissive- /O2 ${SANITIZER_FLAGS}")

set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "${RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${RELEASE_FLAGS}")

set(CMAKE_C_FLAGS_RELEASE_INIT "${RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "${RELEASE_FLAGS}")
