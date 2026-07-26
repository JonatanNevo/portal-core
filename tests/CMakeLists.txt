file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(portal_core_tests)
target_sources(portal_core_tests PUBLIC
        ${TEST_SOURCES}
)
target_link_libraries(portal_core_tests PRIVATE portal-core Catch2::Catch2WithMain)
set_target_properties(portal_core_tests PROPERTIES CXX_MODULE_STD ON)

enable_testing()

include(Catch)
catch_discover_tests(portal_core_tests)
