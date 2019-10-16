#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <aramid/library_main.hpp>

namespace aramid {
namespace test {

void initialize(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
}

int get_num_test_suites() {
    auto unit_test = ::testing::UnitTest::GetInstance();
    return unit_test->total_test_suite_count();
}

std::string get_test_suite_name(int test_suite_index) {
    auto unit_test = ::testing::UnitTest::GetInstance();
    auto test_suite = unit_test->GetTestSuite(test_suite_index);
    return test_suite->name();
}

int get_num_test_cases(int test_suite_index) {
    auto unit_test = ::testing::UnitTest::GetInstance();
    auto test_suite = unit_test->GetTestSuite(test_suite_index);
    return test_suite->total_test_count();
}

std::string get_test_case_name(int test_suite_index, int test_case_index) {
    auto unit_test = ::testing::UnitTest::GetInstance();
    auto test_suite = unit_test->GetTestSuite(test_suite_index);
    auto test_info = test_suite->GetTestInfo(test_case_index);
    return test_info->name();
}

int run_all_tests(::testing::TestEventListener &test_event_listener) {
    auto unit_test = ::testing::UnitTest::GetInstance();

    unit_test->listeners().Append(&test_event_listener);

    int result = RUN_ALL_TESTS();

    unit_test->listeners().Release(&test_event_listener);
    return result;
}

} // namespace test
} // namespace aramid
