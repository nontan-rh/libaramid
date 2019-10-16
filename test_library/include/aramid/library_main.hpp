#ifndef ARAMID_TEST_LIBRARY_MAIN_HPP
#define ARAMID_TEST_LIBRARY_MAIN_HPP

#include <string>

#include <gtest/gtest.h>

namespace aramid {
namespace test {

void initialize(int argc, char **argv);
int get_num_test_suites();
std::string get_test_suite_name(int test_suite_index);
int get_num_test_cases(int test_suite_index);
std::string get_test_case_name(int test_suite_index, int test_case_index);
int run_all_tests(::testing::TestEventListener &test_event_listener);

} // namespace test
} // namespace aramid

#endif
