#include <gtest/gtest.h>

#include <aramid/aramid.h>

#include "deque.h"

namespace {

class DequeCreationTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_MemoryRegion *memory_region;

    DequeCreationTest() {}

    ~DequeCreationTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        memory_region = armd_memory_region_create(&memory_allocator);
    }

    void TearDown() override { armd_memory_region_destroy(memory_region); }
};

TEST_F(DequeCreationTest, CreateAndDestroyDeque) {
    int res;
    ARMD__Deque *deque = armd__deque_create(memory_region, 1);
    ASSERT_NE(deque, nullptr);
    res = armd__deque_destroy(deque);
    ASSERT_EQ(res, 0);
}

TEST_F(DequeCreationTest, CreateZeroSizedDeque) {
    ARMD__Deque *deque = armd__deque_create(memory_region, 0);
    ASSERT_EQ(deque, nullptr);
}

TEST_F(DequeCreationTest, DestroyNonEmptyDeque) {
    int res;

    ARMD__Deque *deque = armd__deque_create(memory_region, 1);
    ASSERT_NE(deque, nullptr);

    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(1));
    ASSERT_EQ(res, 0);

    res = armd__deque_destroy(deque);
    ASSERT_NE(res, 0);
}

class DequeTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_MemoryRegion *memory_region;
    ARMD__Deque *deque;

    DequeTest() {}

    ~DequeTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        memory_region = armd_memory_region_create(&memory_allocator);
        deque = armd__deque_create(memory_region, 4);
    }

    void TearDown() override {
        armd__deque_destroy(deque);
        armd_memory_region_destroy(memory_region);
    }
};

TEST_F(DequeTest, CheckIsEmpty) {
    int res;
    ARMD_Job *job;
    ASSERT_TRUE(armd__deque_is_empty(deque));

    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(1));
    ASSERT_EQ(res, 0);
    ASSERT_FALSE(armd__deque_is_empty(deque));

    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_TRUE(armd__deque_is_empty(deque));
}

TEST_F(DequeTest, CheckIsFull) {
    int res;
    ARMD_Job *job;
    ASSERT_FALSE(armd__deque_is_full(deque));

    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(1));
    ASSERT_EQ(res, 0);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(2));
    ASSERT_EQ(res, 0);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(3));
    ASSERT_EQ(res, 0);

    ASSERT_TRUE(armd__deque_is_full(deque));

    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);

    ASSERT_FALSE(armd__deque_is_full(deque));

    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);

    ASSERT_FALSE(armd__deque_is_full(deque));
}

TEST_F(DequeTest, CheckGetNumEntries) {
    int res;
    ARMD_Job *job;
    ASSERT_EQ(armd__deque_get_num_entries(deque), 0u);

    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(1));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 1u);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(2));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 2u);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(3));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 3u);

    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 2u);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 1u);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 0u);
}

TEST_F(DequeTest, CheckExpand) {
    int res;
    ARMD_Job *job;
    ASSERT_EQ(armd__deque_get_num_entries(deque), 0u);

    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(1));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 1u);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(2));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 2u);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(3));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 3u);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(4));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 4u);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(5));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 5u);

    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 4u);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 3u);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 2u);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 1u);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__deque_get_num_entries(deque), 0u);
}

TEST_F(DequeTest, MixedOperation) {
    int res;
    ARMD_Job *job;

    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(1));
    ASSERT_EQ(res, 0);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(2));
    ASSERT_EQ(res, 0);
    res = armd__deque_enqueue_back(deque, reinterpret_cast<ARMD_Job *>(3));
    ASSERT_EQ(res, 0);
    res = armd__deque_enqueue_forward(deque, reinterpret_cast<ARMD_Job *>(4));
    ASSERT_EQ(res, 0);
    res = armd__deque_enqueue_forward(deque, reinterpret_cast<ARMD_Job *>(5));
    ASSERT_EQ(res, 0);

    ASSERT_EQ(armd__deque_get_num_entries(deque), 5u);
    ASSERT_FALSE(armd__deque_is_empty(deque));

    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(job), 3u);
    res = armd__deque_dequeue_back(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(job), 2u);
    res = armd__deque_dequeue_forward(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(job), 5u);
    res = armd__deque_dequeue_forward(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(job), 4u);
    res = armd__deque_dequeue_forward(deque, &job);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(job), 1u);

    ASSERT_EQ(armd__deque_get_num_entries(deque), 0u);
    ASSERT_TRUE(armd__deque_is_empty(deque));
}

} // namespace
