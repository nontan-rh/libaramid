#include <gtest/gtest.h>

#include "random.h"

namespace {

class RandomTest : public ::testing::Test {
protected:
    ARMD__Random random;

    RandomTest() {}

    ~RandomTest() override {}

    void SetUp() override { armd__random_init(&random, 1); }

    void TearDown() override {}
};

TEST_F(RandomTest, GenerateSomeRandomNumbers) {
    auto v0 = armd__random_generate(&random);
    auto v1 = armd__random_generate(&random);
    auto v2 = armd__random_generate(&random);
    auto v3 = armd__random_generate(&random);
    auto v4 = armd__random_generate(&random);
    auto v5 = armd__random_generate(&random);
    auto v6 = armd__random_generate(&random);
    auto v7 = armd__random_generate(&random);
    auto v8 = armd__random_generate(&random);
    auto v9 = armd__random_generate(&random);

    auto all_same = v0 == v1 && v0 == v2 && v0 == v3 && v0 == v4 && v0 == v5 &&
                    v0 == v6 && v0 == v7 && v0 == v8 && v0 == v9;

    ASSERT_FALSE(all_same);
}

} // namespace
