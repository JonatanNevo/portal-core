cmake_minimum_required(VERSION 3.38...4.4 FATAL_ERROR)

include(cmake/enable-experimental-import-std.cmake)

# Load .env before project() so the vcpkg toolchain picks up VCPKG_BINARY_SOURCES when it loads.
include(cmake/load-env-file.cmake)
portal_load_env_file(".env")

include(cmake/tests_helpers.cmake)

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
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# vcpkg's glm port ships glm/glm.cppm (g-truc/glm#1208) but glmConfig.cmake only
# exports header-only imported targets, not a compiled module target. Build one
# ourselves from the shipped source file so `import glm;` works.
get_target_property(GLM_INCLUDE_DIR glm::glm-header-only INTERFACE_INCLUDE_DIRECTORIES)
add_library(glm-module STATIC)
target_sources(glm-module
        PUBLIC
        FILE_SET CXX_MODULES
            FILES "${GLM_INCLUDE_DIR}/glm/glm.cppm"
        BASE_DIRS
            "${GLM_INCLUDE_DIR}"
)
target_link_libraries(glm-module PUBLIC glm::glm)
target_compile_features(glm-module PUBLIC cxx_std_23)
target_compile_definitions(glm-module PRIVATE
        GLM_FORCE_SWIZZLE
        GLM_FORCE_RADIANS
        GLM_FORCE_CTOR_INIT
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_CXX20
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)
set_target_properties(glm-module PROPERTIES CXX_MODULE_STD ON)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "portal/core/*.cpp")
file(GLOB_RECURSE CORE_MODULES CONFIGURE_DEPENDS "portal/core/*.cppm")
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS "portal/core/*.h")

file(GLOB_RECURSE HAL_MODULES CONFIGURE_DEPENDS "portal/platform/core/hal/*.cppm")

file(GLOB_RECURSE CORE_WINDOWS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/windows/*.cpp")
file(GLOB_RECURSE CORE_WINDOWS_MODULES CONFIGURE_DEPENDS "portal/platform/core/windows/*.cppm")
file(GLOB_RECURSE CORE_WINDOWS_HEADERS CONFIGURE_DEPENDS "portal/platform/core/windows/*.h")

file(GLOB_RECURSE CORE_MACOS_SOURCES CONFIGURE_DEPENDS "portal/platform/core/macos/*.cpp")
file(GLOB_RECURSE CORE_MACOS_MODULES CONFIGURE_DEPENDS "portal/platform/core/macos/*.cppm")
file(GLOB_RECURSE CORE_MACOS_HEADERS CONFIGURE_DEPENDS "portal/platform/core/macos/*.h")

file(GLOB_RECURSE CORE_LINUX_SOURCES CONFIGURE_DEPENDS "portal/platform/core/linux/*.cpp")
file(GLOB_RECURSE CORE_LINUX_MODULES CONFIGURE_DEPENDS "portal/platform/core/linux/*.cppm")
file(GLOB_RECURSE CORE_LINUX_HEADERS CONFIGURE_DEPENDS "portal/platform/core/linux/*.h")

add_library(portal-core STATIC)
target_sources(portal-core
        PUBLIC
        ${CORE_SOURCES}
        PUBLIC
        FILE_SET CXX_MODULES
            FILES ${CORE_MODULES} ${HAL_MODULES}
        PUBLIC
        FILE_SET HEADERS
            FILES ${CORE_HEADERS}
        BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(portal-core PROPERTIES CXX_MODULE_STD ON)
target_compile_features(portal-core PUBLIC cxx_std_23)
target_link_libraries(portal-core PUBLIC
        spdlog::spdlog
        glm-module
)

if (WIN32)
    if (CORE_WINDOWS_SOURCES OR CORE_WINDOWS_HEADERS OR CORE_WINDOWS_MODULES)
        target_sources(portal-core
                PRIVATE ${CORE_WINDOWS_SOURCES}
                PUBLIC
                FILE_SET CXX_MODULES
                    FILES ${CORE_WINDOWS_MODULES}
                PUBLIC
                FILE_SET HEADERS
                    FILES ${CORE_WINDOWS_HEADERS}
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif ()
elseif (APPLE)
    if (CORE_MACOS_SOURCES OR CORE_MACOS_HEADERS OR CORE_MACOS_MODULES)
        target_sources(portal-core
                PRIVATE ${CORE_MACOS_SOURCES}
                PUBLIC
                FILE_SET CXX_MODULES
                    FILES ${CORE_MACOS_MODULES}
                PUBLIC
                FILE_SET HEADERS
                    FILES ${CORE_MACOS_HEADERS}
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif ()
elseif (UNIX)
    if (CORE_LINUX_SOURCES OR CORE_LINUX_HEADERS OR CORE_LINUX_MODULES)
        target_sources(portal-core
                PRIVATE ${CORE_LINUX_SOURCES}
                PUBLIC
                FILE_SET CXX_MODULES
                    FILES ${CORE_LINUX_MODULES}
                PUBLIC
                FILE_SET HEADERS
                    FILES ${CORE_LINUX_HEADERS}
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif ()
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

if (MSVC)
    target_compile_options(portal-core PRIVATE /W4 /WX)
else ()
    target_compile_options(portal-core PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-designated-field-initializers -Wthread-safety)
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