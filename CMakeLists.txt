cmake_minimum_required(VERSION 3.10.2)
cmake_policy(VERSION 3.10.2...3.10.2)

project(aramid)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)

include(cmake/load_external_project.cmake)
include(cmake/build_config.cmake)

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
