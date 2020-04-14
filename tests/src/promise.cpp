#include <stdint.h>
#include <stdio.h>

#include <gtest/gtest.h>

#include <aramid/aramid.h>

#if defined(ARAMID_USE_PTHREAD)
#include <pthread.h>
#elif defined(ARAMID_USE_WIN32THREAD)
#include <windows.h>
#elif defined(ARAMID_EDITOR)
#else
#error Thread implementation is not specified
#endif

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

    void TearDown() override {
        int res = armd_context_destroy(context);
        ASSERT_EQ(res, 0);
    }
};

struct CallbackContext {
    bool is_called;
};

void cb(ARMD_Handle handle, void *callback_context, int has_error) {
    (void)handle;
    (void)has_error;
    reinterpret_cast<CallbackContext *>(callback_context)->is_called = true;
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
    ARMD_Handle promise = 0;
    for (int count = 0; count < 100; count++) {
        promise =
            armd_invoke(context, empty_procedure, nullptr, 1, dependencies);
        ASSERT_NE(promise, 0u);
        if (dependencies[0] != 0) {
            res = armd_detach(context, dependencies[0]);
            ASSERT_EQ(res, 0);
        }
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

    CallbackContext callback_context;
    callback_context.is_called = false;

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, empty_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_add_promise_callback(context, promise, &callback_context, cb);
    ASSERT_EQ(res, 0);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    ASSERT_TRUE(callback_context.is_called);

    res = armd_procedure_destroy(empty_procedure);
    ASSERT_EQ(res, 0);
}

#if defined(ARAMID_USE_PTHREAD)

static void sleep_microsecond(unsigned long us) { usleep(us); }

#elif defined(ARAMID_USE_WIN32THREAD)

static void sleep_microsecond(unsigned long us) {
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(LONGLONG)(10 * us);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

#elif defined(ARAMID_EDITOR)

static void sleep_microsecond(unsigned long us) { assert(0); }

#else

#error Thread implementation is not specified

#endif

int single_sleep_continuation(ARMD_Job *job, const void *constants, void *args,
                              void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    sleep_microsecond(100 * 1000);
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

TEST_F(PromiseTest, SynchronizeAfterDetach) {
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
    res = armd_detach(context, promise);
    ASSERT_EQ(res, 0);

    res = armd_add_promise_callback(context, promise, &callback_context, cb);
    ASSERT_NE(res, 0); // Error
    dependencies[0] = promise;
    ARMD_Handle error_handle =
        armd_invoke(context, sleep_procedure, nullptr, 1, dependencies);
    ASSERT_EQ(error_handle, 0u); // Error
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // Error

    res = armd_await_all(context);
    ASSERT_EQ(res, 0);

    ASSERT_FALSE(callback_context.is_called);

    res = armd_procedure_destroy(sleep_procedure);
    ASSERT_EQ(res, 0);
}

} // namespace
