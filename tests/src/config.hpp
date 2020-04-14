#ifndef ARAMID_TESTS_CONFIG_HPP
#define ARAMID_TESTS_CONFIG_HPP

#include <cstdlib>
#include <string>
#include <iostream>

namespace aramid {
namespace test {

inline int get_num_executors() {
    try {
        const auto num_executors_str = getenv("ARAMID_TEST_NUM_EXECUTORS");
        if (num_executors_str == nullptr) {
            return 1;
        }

        std::cout << "ARAMID_TEST_NUM_EXECUTORS: " << num_executors_str << std::endl;

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
