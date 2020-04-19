
add_library(aramid_integration_test_object OBJECT
    src/error.cpp
    src/execution.cpp
    src/logger.cpp
    src/promise.cpp
    src/time.cpp
    )
aramid_target_setup_compile_options(aramid_integration_test_object)
# Use $<TARGET_PROPERTY:*,INTERFACE_INCLUDE_DIRECTORIES> instead of
# target_link_libraries because Linking Object Library feature is not
# available before version 3.12.0
target_include_directories(aramid_integration_test_object
    PRIVATE
    $<TARGET_PROPERTY:gtest_main,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:aramid,INTERFACE_INCLUDE_DIRECTORIES>)
