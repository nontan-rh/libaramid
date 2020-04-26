cmake_minimum_required(VERSION 3.10.2)
cmake_policy(VERSION 3.10.2...3.10.2)

project(aramid)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)

include(CMakePackageConfigHelpers)
include(cmake/load-external-project.cmake)
include(cmake/build-config.cmake)

if(BUILD_TESTING)
    enable_testing()
    include(GoogleTest)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    aramid_load_external_project(cmake/external_projects/googletest.cmake)
endif()

add_subdirectory(lib)

if(BUILD_TESTING)
    add_subdirectory(tests)

    if((${CMAKE_SYSTEM_NAME} MATCHES "Android") OR (${CMAKE_SYSTEM_NAME} MATCHES "iOS"))
        message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME} matches 'Android' or 'iOS', so it won't build the test excutable but builds a library.")
        add_subdirectory(test_library)
    elseif(TIDY_MODE)
        add_subdirectory(test_library)
        add_subdirectory(test_executable)
    else()
        add_subdirectory(test_executable)
    endif()
endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/aramid/aramid-config-version.cmake"
    VERSION 0.0.1
    COMPATIBILITY AnyNewerVersion
    )

export(
    EXPORT aramid_targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/aramid/aramid-targets.cmake"
    NAMESPACE aramid::
    )
configure_file(cmake/aramid-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/aramid/aramid-config.cmake"
    COPYONLY
    )

set(config_package_location lib/cmake/aramid)
install(EXPORT aramid_targets
    FILE
    aramid-targets.cmake
    NAMESPACE
    aramid::
    DESTINATION
    ${config_package_location}
    )
install(
    FILES
    cmake/aramid-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/aramid/aramid-config-version.cmake"
    DESTINATION
    ${config_package_location}
    )
