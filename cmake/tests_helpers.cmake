function(portal_add_test_subdir TARGET_NAME)
    set(options "")
    set(oneValueArgs GLOB_STRING TEST_TARGET_NAME TEST_TYPE)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_GLOB_STRING)
        set(ARG_GLOB_STRING "*.cpp")
    endif()

    if(NOT ARG_TEST_TARGET_NAME)
        cmake_path(GET CMAKE_CURRENT_SOURCE_DIR FILENAME ARG_TEST_TARGET_NAME)
    endif()

    if(NOT ARG_TEST_TYPE)
        set(ARG_TEST_TYPE "unit")
    endif()

    set(FULL_TARGET_NAME ${TARGET_NAME}-${ARG_TEST_TYPE}-${ARG_TEST_TARGET_NAME}-test)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${ARG_GLOB_STRING})

    add_executable(${FULL_TARGET_NAME})
    target_sources(${FULL_TARGET_NAME} PUBLIC ${TEST_SOURCES})
    target_link_libraries(${FULL_TARGET_NAME} PRIVATE ${TARGET_NAME} Catch2::Catch2WithMain)
    set_target_properties(${FULL_TARGET_NAME} PROPERTIES CXX_MODULE_STD ON)

    enable_testing()

    include(Catch)
    catch_discover_tests(${FULL_TARGET_NAME})
endfunction()
