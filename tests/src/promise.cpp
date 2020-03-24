#include <stdint.h>
#include <stdio.h>

#include <gtest/gtest.h>

#include <aramid/aramid.h>

namespace {

class PromiseTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_Context *context;

    PromiseTest() {}

    ~PromiseTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        context = armd_context_create(&memory_allocator, 1);
    }

    void TearDown() override { armd_context_destroy(context); }
};

struct CbCtx {
    bool is_called;
};

void cb(ARMD_Handle handle, void *callback_context) {
    (void)handle;

    CbCtx *cbctx = reinterpret_cast<CbCtx *>(callback_context);
    cbctx->is_called = true;
}

TEST_F(PromiseTest, ChainEmptyProcedure) {
    int res;

    ARMD_Procedure *empty_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        empty_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise;
    for (int count = 0; count < 100; count++) {
        promise =
            armd_invoke(context, empty_procedure, nullptr, 1, dependencies);
        ASSERT_NE(promise, 0u);
        dependencies[0] = promise;
    }
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    res = armd_procedure_destroy(empty_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(PromiseTest, CallbackEmptyProcedure) {
    int res;

    ARMD_Procedure *empty_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        empty_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    CbCtx cbctx;
    cbctx.is_called = false;

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, empty_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_add_promise_callback(context, promise, &cbctx, cb);
    ASSERT_EQ(res, 0);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    ASSERT_TRUE(cbctx.is_called);

    res = armd_procedure_destroy(empty_procedure);
    ASSERT_EQ(res, 0);
}

int single_sleep_continuation(ARMD_Job *job, const void *constants,
                              const void *args, void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    usleep(100 * 1000);
    return 0;
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

    CbCtx cbctx;
    cbctx.is_called = false;

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, sleep_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_add_promise_callback(context, promise, &cbctx, cb);
    ASSERT_EQ(res, 0);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    ASSERT_TRUE(cbctx.is_called);

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

    CbCtx cbctx;
    cbctx.is_called = false;

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, sleep_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);
    res = armd_add_promise_callback(context, promise, &cbctx, cb);
    ASSERT_EQ(res, 0);

    ASSERT_TRUE(cbctx.is_called);

    res = armd_procedure_destroy(sleep_procedure);
    ASSERT_EQ(res, 0);
}

} // namespace
