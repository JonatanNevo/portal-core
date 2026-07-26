cmake_minimum_required(VERSION 3.30...4.4)

include(cmake/enable-experimental-import-std.cmake)

# Load .env before project() so the vcpkg toolchain picks up VCPKG_BINARY_SOURCES when it loads.
include(cmake/load-env-file.cmake)
portal_load_env_file(".env")

file(READ "version" PROJECT_VERSION)
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
        PORTAL_TEMPLATE_BUILD_TESTS
        "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
        ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
        PORTAL_TEMPLATE_BUILD_EXAMPLES
        "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
        ${PROJECT_IS_TOP_LEVEL}
)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

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

add_executable(main main.cpp)
target_link_libraries(main portal-core)