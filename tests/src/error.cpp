#include <stdint.h>
#include <stdio.h>

#include <gtest/gtest.h>

#include <aramid/aramid.h>

namespace {

class ErrorTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_Context *context;

    ErrorTest() {}

    ~ErrorTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        context = armd_context_create(&memory_allocator, 1);
    }

    void TearDown() override { armd_context_destroy(context); }
};

int single_error_continuation(ARMD_Job *job, const void *constants,
                              const void *args, void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    return 1;
}

TEST_F(PromiseTest, CallbackSleepProcedureBeforeAwait) {
    int res;

    ARMD_Procedure *sleep_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        armd_then_single(builder, single_sleep_continuation);
        sleep_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    CallbackContext callback_context;
    callback_context.is_called = false;

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, sleep_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_add_promise_callback(context, promise, &callback_context, cb);
    ASSERT_EQ(res, 0);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    ASSERT_TRUE(callback_context.is_called);

    res = armd_procedure_destroy(sleep_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(PromiseTest, CallbackSleepProcedureAfterAwait) {
    int res;

    ARMD_Procedure *sleep_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        armd_then_single(builder, single_sleep_continuation);
        sleep_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    CallbackContext callback_context;
    callback_context.is_called = false;

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, sleep_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);
    res = armd_add_promise_callback(context, promise, &callback_context, cb);
    ASSERT_EQ(res, -1); // This will fail because there's already no promise

    ASSERT_FALSE(callback_context.is_called);

    res = armd_procedure_destroy(sleep_procedure);
    ASSERT_EQ(res, 0);
}

} // namespace
