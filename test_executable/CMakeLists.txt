
add_executable(aramid_test_executable
    src/executable_main.cpp
    $<TARGET_OBJECTS:aramid_unit_test_object>
    $<TARGET_OBJECTS:aramid_integration_test_object>)
aramid_target_setup_compile_options(aramid_test_executable)
target_link_libraries(aramid_test_executable PRIVATE gtest_main aramid)

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set_property(TARGET aramid_test_executable PROPERTY OUTPUT_NAME "index")
    set_property(TARGET aramid_test_executable PROPERTY SUFFIX ".html")
else()
    gtest_discover_tests(aramid_test_executable)
endif()
