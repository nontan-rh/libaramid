#ifndef ARAMID_TESTS_CONFIG_HPP
#define ARAMID_TESTS_CONFIG_HPP

#include <cstdlib>
#include <iostream>
#include <locale>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace aramid {
namespace test {

namespace {

inline std::string get_environment_variable(const std::string &key) {
#ifdef _WIN32
    const size_t max_length = 32768;
    char buf[max_length];
    const auto ret = GetEnvironmentVariableA(key.c_str(), buf, max_length);
    if (ret == 0 || ret > max_length) {
        return "";
    } else {
        return std::string(buf);
    }
#else
    const auto buf = getenv(key.c_str());
    if (buf == nullptr) {
        return "";
    } else {
        return std::string(buf);
    }
#endif
}

} // namespace

inline int get_num_executors() {
    try {
        const auto num_executors_str =
            get_environment_variable("ARAMID_TEST_NUM_EXECUTORS");

        std::cout << "ARAMID_TEST_NUM_EXECUTORS: " << num_executors_str
                  << std::endl;

        auto num_executors = std::stoi(num_executors_str);
        if (num_executors < 1) {
            num_executors = 1;
        }

        return num_executors;
    } catch (...) {
        return 1;
    }
}

} // namespace test
} // namespace aramid

#endif // ARAMID_TESTS_CONFIG_HPP
