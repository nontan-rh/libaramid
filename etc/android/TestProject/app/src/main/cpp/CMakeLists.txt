cmake_minimum_required(VERSION 3.10.2)
cmake_policy(VERSION 3.10.2...3.10.2)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../.. ${CMAKE_CURRENT_BINARY_DIR}/libaramid)

project(aramid_test_jni)

add_library(aramid_test_jni SHARED aramid_test_jni.cpp)
target_link_libraries(aramid_test_jni PRIVATE gtest_main aramid_test_library)
target_compile_options(aramid_test_jni PRIVATE -Wall -Wextra -pedantic -Werror)
