
if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME} matches 'Android', so it will build the library as shared library.")
    set(TEST_LIBRARY_TYPE SHARED)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME} matches 'iOS', so it will build the library as static library.")
    set(TEST_LIBRARY_TYPE STATIC)
else()
    message(FATAL_ERROR "Unknown CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
endif()

add_library(aramid_test_library ${TEST_LIBRARY_TYPE}
    src/library_main.cpp
    $<TARGET_OBJECTS:aramid_unit_test_object>
    $<TARGET_OBJECTS:aramid_integration_test_object>)

aramid_target_setup_compile_options(aramid_test_library)
target_link_libraries(aramid_test_library PRIVATE gtest_main aramid)
target_include_directories(aramid_test_library PUBLIC include)
