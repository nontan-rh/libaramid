#include <condition_variable>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

#include <aramid/aramid.h>

namespace {

class LoggerCreationTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_MemoryRegion *memory_region;

    LoggerCreationTest() {}

    ~LoggerCreationTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        memory_region = armd_memory_region_create(&memory_allocator);
    }

    void TearDown() override { armd_memory_region_destroy(memory_region); }
};

TEST_F(LoggerCreationTest, CreateAndDestroyLogger) {
    ARMD_Logger *logger = armd_logger_create(memory_region);
    ASSERT_NE(logger, nullptr);
    ARMD_Bool destroyed = armd_logger_decrement_reference_count(logger);
    ASSERT_TRUE(destroyed);
}

TEST_F(LoggerCreationTest, DestroyNonEmptyLogger) {
    ARMD_Logger *logger = armd_logger_create(memory_region);
    ASSERT_NE(logger, nullptr);
    armd_logger_log(logger, ARMD_LogLevel_Debug,
                    armd_memory_region_strdup(memory_region, "a"));
    ARMD_Bool destroyed = armd_logger_decrement_reference_count(logger);
    ASSERT_TRUE(destroyed);
}

TEST_F(LoggerCreationTest, DestroyWithMultithreadAwaiter) {
    std::mutex mutex;
    std::condition_variable condvar;
    ARMD_Logger *logger = armd_logger_create(memory_region);
    ASSERT_NE(logger, nullptr);

    ARMD_Bool destroyed_in_parent = 0;
    ARMD_Bool destroyed_in_child = 0;

    armd_logger_increment_reference_count(logger);
    std::thread th([logger, &destroyed_in_child] {
        ARMD_LogElement *elem;
        int res = armd_logger_get_log_element(logger, &elem);
        ASSERT_NE(res, 0);
        ASSERT_EQ(elem, nullptr);

        destroyed_in_child = armd_logger_decrement_reference_count(logger);
    });

    destroyed_in_parent = armd_logger_decrement_reference_count(logger);

    th.join();

    ASSERT_TRUE(destroyed_in_parent || destroyed_in_child);
    ASSERT_FALSE(destroyed_in_parent && destroyed_in_child);
}

class LoggerTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_MemoryRegion *memory_region;
    ARMD_Logger *logger;

    LoggerTest() {}

    ~LoggerTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        memory_region = armd_memory_region_create(&memory_allocator);
        logger = armd_logger_create(memory_region);
    }

    void TearDown() override {
        ARMD_Bool destroyed = armd_logger_decrement_reference_count(logger);
        ASSERT_TRUE(destroyed);
        armd_memory_region_destroy(memory_region);
    }
};

TEST_F(LoggerTest, LogAndGetSingle) {
    int res;
    ARMD_LogElement *elem;
    armd_logger_log(logger, ARMD_LogLevel_Debug,
                    armd_memory_region_strdup(memory_region, "a"));
    res = armd_logger_get_log_element(logger, &elem);
    ASSERT_EQ(res, 0);

    ASSERT_EQ(elem->level, ARMD_LogLevel_Debug);
    ASSERT_NE(elem->timespec.seconds, 0);
    ASSERT_STREQ(elem->message, "a");
}

TEST_F(LoggerTest, LogAndGetTwo) {
    int res;
    ARMD_LogElement *elem;
    armd_logger_log(logger, ARMD_LogLevel_Debug,
                    armd_memory_region_strdup(memory_region, "a"));
    armd_logger_log(logger, ARMD_LogLevel_Info,
                    armd_memory_region_strdup(memory_region, "b"));
    res = armd_logger_get_log_element(logger, &elem);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(elem->level, ARMD_LogLevel_Debug);
    ASSERT_NE(elem->timespec.seconds, 0);
    ASSERT_STREQ(elem->message, "a");
    armd_logger_destroy_log_element(logger, elem);
    res = armd_logger_get_log_element(logger, &elem);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(elem->level, ARMD_LogLevel_Info);
    ASSERT_NE(elem->timespec.seconds, 0);
    ASSERT_STREQ(elem->message, "b");
    armd_logger_destroy_log_element(logger, elem);
}

TEST_F(LoggerTest, LogAndGetAlternative) {
    int res;
    ARMD_LogElement *elem;
    armd_logger_log(logger, ARMD_LogLevel_Debug,
                    armd_memory_region_strdup(memory_region, "a"));
    res = armd_logger_get_log_element(logger, &elem);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(elem->level, ARMD_LogLevel_Debug);
    ASSERT_NE(elem->timespec.seconds, 0);
    ASSERT_STREQ(elem->message, "a");
    armd_logger_destroy_log_element(logger, elem);
    armd_logger_log(logger, ARMD_LogLevel_Info,
                    armd_memory_region_strdup(memory_region, "b"));
    res = armd_logger_get_log_element(logger, &elem);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(elem->level, ARMD_LogLevel_Info);
    ASSERT_NE(elem->timespec.seconds, 0);
    ASSERT_STREQ(elem->message, "b");
    armd_logger_destroy_log_element(logger, elem);
}

} // namespace
