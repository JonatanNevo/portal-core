cmake_minimum_required(VERSION 3.30...4.4)

include(cmake/enable-experimental-import-std.cmake)

# Load .env before project() so the vcpkg toolchain picks up VCPKG_BINARY_SOURCES when it loads.
include(cmake/load-env-file.cmake)
portal_load_env_file(".env")

file(READ "VERSION.txt" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(
        portal-core
        DESCRIPTION "A template for creating portal libraries"
        LANGUAGES CXX
        VERSION ${PROJECT_VERSION}
)

message(STATUS "Configuring ${PROJECT_NAME} - version: ${PROJECT_VERSION}")

# [CMAKE.SKIP_TESTS]
option(
        PORTAL_CORE_BUILD_TESTS
        "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
        ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
        PORTAL_CORE_BUILD_EXAMPLES
        "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
        ${PROJECT_IS_TOP_LEVEL}
)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(Catch2 CONFIG REQUIRED)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB_RECURSE CORE_MODULES CONFIGURE_DEPENDS "portal/core/*.cppm")
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS "portal/core/*.h")

add_library(portal-core STATIC)
target_sources(portal-core
        PUBLIC
        ${CORE_SOURCES}
        PUBLIC
        FILE_SET CXX_MODULES
            FILES ${CORE_MODULES}
        PUBLIC
        FILE_SET HEADERS
            FILES ${CORE_HEADERS}
        BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(portal-core PROPERTIES CXX_MODULE_STD ON)
target_compile_features(portal-core PUBLIC cxx_std_23)

if (WIN32)
    target_compile_definitions(portal-core PRIVATE _PORTAL_PLATFORM=portal::Platform::Windows)
elseif (APPLE)
    target_compile_definitions(portal-core PRIVATE _PORTAL_PLATFORM=portal::Platform::MacOS)
elseif (UNIX)
    target_compile_definitions(portal-core PRIVATE _PORTAL_PLATFORM=portal::Platform::Linux)
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

target_compile_definitions(portal-core
        PUBLIC
        $<BUILD_INTERFACE:$<$<CONFIG:Debug>:PORTAL_DEBUG>>
        $<BUILD_INTERFACE:$<$<CONFIG:RelWithDebInfo>:PORTAL_DEVELOPMENT>>
        $<BUILD_INTERFACE:$<$<CONFIG:Release>:PORTAL_DIST>>
        INTERFACE
        $<INSTALL_INTERFACE:$<$<CONFIG:Debug>:PORTAL_DEBUG>>
        $<INSTALL_INTERFACE:$<$<CONFIG:RelWithDebInfo>:PORTAL_DEVELOPMENT>>
        $<INSTALL_INTERFACE:$<$<CONFIG:Release>:PORTAL_DIST>>
)

if (PORTAL_CORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

add_executable(main main.cpp)
target_link_libraries(main portal-core)
set_target_properties(main PROPERTIES CXX_MODULE_STD ON)