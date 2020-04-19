#include <cstdint>
#include <cstdio>

#include <atomic>

#include <gtest/gtest.h>

#include <aramid/aramid.h>

#include "config.hpp"

namespace {

class ErrorTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_Context *context;

    ErrorTest() {}

    ~ErrorTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        context = armd_context_create(&memory_allocator,
                                      aramid::test::get_num_executors());
    }

    void TearDown() override {
        int res = armd_context_destroy(context);
        ASSERT_EQ(res, 0);
    }
};

typedef struct TAG_CommonArgs {
    std::atomic<int> unwind;
} CommonArgs;

typedef struct TAG_CommonFrame {
} CommonFrame;

void unwind_func(ARMD_Job *job, const void *constants, void *args,
                 void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;

    ++(reinterpret_cast<CommonArgs *>(args))->unwind;
}

void *error_continuation_frame_creator(ARMD_MemoryRegion *memory_region) {
    return armd_memory_region_allocate(memory_region, 1);
}

void error_continuation_frame_destroyer(ARMD_MemoryRegion *memory_region,
                                        void *continuation_frame) {
    armd_memory_region_free(memory_region, continuation_frame);
}

ARMD_ContinuationResult error_continuation(ARMD_Job *job, const void *constants,
                                           void *args, void *frame,
                                           const void *continuation_constants,
                                           void *continuation_frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    (void)continuation_constants;
    (void)continuation_frame;
    return ARMD_ContinuationResult_Error;
}

ARMD_ContinuationResult error_trap_recover(ARMD_Job *job, const void *constants,
                                           void *args, void *frame,
                                           const void *continuation_constants,
                                           void *continuation_frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    (void)continuation_constants;
    (void)continuation_frame;
    return ARMD_ContinuationResult_Ended;
}

ARMD_ContinuationResult error_trap_rethrow(ARMD_Job *job, const void *constants,
                                           void *args, void *frame,
                                           const void *continuation_constants,
                                           void *continuation_frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    (void)continuation_constants;
    (void)continuation_frame;
    return ARMD_ContinuationResult_Error;
}

ARMD_Procedure *
build_error_procedure(const ARMD_MemoryAllocator *memory_allocator,
                      bool unwind) {
    ARMD_ProcedureBuilder *builder =
        armd_procedure_builder_create(memory_allocator, 0, 0);

    void *continuation_constants =
        armd_memory_allocator_allocate(memory_allocator, 1);
    armd_then(builder, error_continuation, continuation_constants, nullptr,
              error_continuation_frame_creator,
              error_continuation_frame_destroyer);
    if (unwind) {
        armd_unwind(builder, unwind_func);
    }

    return armd_procedure_builder_build_and_destroy(builder);
}

int single_success_continuation(ARMD_Job *job, const void *constants,
                                void *args, void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    return 0;
}

ARMD_Procedure *
build_single_success_procedure(const ARMD_MemoryAllocator *memory_allocator,
                               bool unwind) {
    ARMD_ProcedureBuilder *builder =
        armd_procedure_builder_create(memory_allocator, 0, 0);
    armd_then_single(builder, single_success_continuation);
    if (unwind) {
        armd_unwind(builder, unwind_func);
    }
    return armd_procedure_builder_build_and_destroy(builder);
}

int single_error_continuation(ARMD_Job *job, const void *constants, void *args,
                              void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    return 1;
}

ARMD_Procedure *
build_single_error_procedure(const ARMD_MemoryAllocator *memory_allocator,
                             bool unwind) {
    ARMD_ProcedureBuilder *builder =
        armd_procedure_builder_create(memory_allocator, 0, 0);
    armd_then_single(builder, single_error_continuation);
    if (unwind) {
        armd_unwind(builder, unwind_func);
    }
    return armd_procedure_builder_build_and_destroy(builder);
}

ARMD_Size sequential_for_error_count(void *args, void *frame) {
    (void)args;
    (void)frame;
    return 10;
}

int sequential_for_error_continuation(ARMD_Job *job, const void *constants,
                                      void *args, void *frame,
                                      ARMD_Size index) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    (void)index;
    return 1;
}

ARMD_Procedure *build_sequential_for_error_procedure(
    const ARMD_MemoryAllocator *memory_allocator) {
    ARMD_ProcedureBuilder *builder =
        armd_procedure_builder_create(memory_allocator, 0, 0);
    armd_then_sequential_for(builder, sequential_for_error_count,
                             sequential_for_error_continuation);
    return armd_procedure_builder_build_and_destroy(builder);
}

typedef struct TAG_CallerConstants {
    ARMD_Procedure *procedure;
} CallerConstants;

int single_caller_continuation(ARMD_Job *job, const void *constants, void *args,
                               void *frame) {
    (void)constants;
    (void)args;
    (void)frame;
    armd_fork(job,
              reinterpret_cast<const CallerConstants *>(constants)->procedure,
              args);
    return 0;
}

ARMD_Procedure *
build_caller_procedure(const ARMD_MemoryAllocator *memory_allocator,
                       ARMD_Procedure *procedure, bool unwind) {
    ARMD_ProcedureBuilder *builder = armd_procedure_builder_create(
        memory_allocator, sizeof(CallerConstants), 0);
    armd_then_single(builder, single_caller_continuation);
    if (unwind) {
        armd_unwind(builder, unwind_func);
    }
    auto constants = reinterpret_cast<CallerConstants *>(
        armd_procedure_builder_get_constants(builder));
    constants->procedure = procedure;
    return armd_procedure_builder_build_and_destroy(builder);
}

TEST_F(ErrorTest, Error) {
    int res;

    ARMD_Procedure *error_procedure =
        build_error_procedure(&memory_allocator, false);

    ARMD_Handle promise =
        armd_invoke(context, error_procedure, nullptr, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error

    res = armd_procedure_destroy(error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, SingleError) {
    int res;

    ARMD_Procedure *single_error_procedure =
        build_single_error_procedure(&memory_allocator, false);

    ARMD_Handle promise =
        armd_invoke(context, single_error_procedure, nullptr, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error

    res = armd_procedure_destroy(single_error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, SequentialForError) {
    int res;

    ARMD_Procedure *sequential_for_error_procedure =
        build_sequential_for_error_procedure(&memory_allocator);

    ARMD_Handle promise = armd_invoke(context, sequential_for_error_procedure,
                                      nullptr, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // ERROR

    res = armd_procedure_destroy(sequential_for_error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, SingleErrorChain2) {
    int res;

    ARMD_Procedure *single_error_procedure =
        build_single_error_procedure(&memory_allocator, false);
    ARMD_Procedure *single_call_error_procedure = build_caller_procedure(
        &memory_allocator, single_error_procedure, false);

    ARMD_Handle promise =
        armd_invoke(context, single_call_error_procedure, nullptr, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error

    res = armd_procedure_destroy(single_call_error_procedure);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(single_error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, SingleErrorChain3) {
    int res;

    ARMD_Procedure *single_error_procedure =
        build_single_error_procedure(&memory_allocator, false);
    ARMD_Procedure *single_call_error_procedure = build_caller_procedure(
        &memory_allocator, single_error_procedure, false);
    ARMD_Procedure *single_call_error_procedure2 = build_caller_procedure(
        &memory_allocator, single_call_error_procedure, false);

    ARMD_Handle promise =
        armd_invoke(context, single_call_error_procedure2, nullptr, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error

    res = armd_procedure_destroy(single_call_error_procedure2);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(single_call_error_procedure);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(single_error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, SingleErrorUnwind) {
    int res;

    ARMD_Procedure *single_error_procedure =
        build_single_error_procedure(&memory_allocator, true);

    CommonArgs args;
    args.unwind = 0;
    ARMD_Handle promise =
        armd_invoke(context, single_error_procedure, &args, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error
    ASSERT_EQ(args.unwind, 1);

    res = armd_procedure_destroy(single_error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, SingleErrorChain2Unwind) {
    int res;

    ARMD_Procedure *single_error_procedure =
        build_single_error_procedure(&memory_allocator, true);
    ARMD_Procedure *single_call_error_procedure =
        build_caller_procedure(&memory_allocator, single_error_procedure, true);

    CommonArgs args;
    args.unwind = 0;
    ARMD_Handle promise =
        armd_invoke(context, single_call_error_procedure, &args, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error
    ASSERT_EQ(args.unwind, 2);

    res = armd_procedure_destroy(single_call_error_procedure);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(single_error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, SingleErrorChain3Unwind) {
    int res;

    CommonArgs args;
    args.unwind = 0;
    ARMD_Procedure *single_error_procedure =
        build_single_error_procedure(&memory_allocator, true);
    ARMD_Procedure *single_call_error_procedure =
        build_caller_procedure(&memory_allocator, single_error_procedure, true);
    ARMD_Procedure *single_call_error_procedure2 = build_caller_procedure(
        &memory_allocator, single_call_error_procedure, true);

    ARMD_Handle promise =
        armd_invoke(context, single_call_error_procedure2, &args, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error
    ASSERT_EQ(args.unwind, 3);

    res = armd_procedure_destroy(single_call_error_procedure2);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(single_call_error_procedure);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(single_error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, ErrorTrapRecover) {
    int res;

    ARMD_Procedure *error_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);

        void *continuation_constants =
            armd_memory_allocator_allocate(&memory_allocator, 1);
        armd_then(builder, error_continuation, continuation_constants,
                  error_trap_recover, error_continuation_frame_creator,
                  error_continuation_frame_destroyer);

        error_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle promise =
        armd_invoke(context, error_procedure, nullptr, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    res = armd_procedure_destroy(error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, ErrorTrapRethrow) {
    int res;

    ARMD_Procedure *error_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);

        void *continuation_constants =
            armd_memory_allocator_allocate(&memory_allocator, 1);
        armd_then(builder, error_continuation, continuation_constants,
                  error_trap_rethrow, error_continuation_frame_creator,
                  error_continuation_frame_destroyer);

        error_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle promise =
        armd_invoke(context, error_procedure, nullptr, 0, nullptr);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error

    res = armd_procedure_destroy(error_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ErrorTest, ErrorPromise) {
    int res;

    CommonArgs args;
    args.unwind = 0;

    ARMD_Procedure *error_procedure =
        build_single_error_procedure(&memory_allocator, true);
    ARMD_Procedure *success_procedure =
        build_single_success_procedure(&memory_allocator, true);

    ARMD_Handle success_promise =
        armd_invoke(context, success_procedure, &args, 0, nullptr);
    ARMD_Handle error_promise =
        armd_invoke(context, error_procedure, &args, 0, nullptr);
    ASSERT_NE(success_promise, 0u);
    ASSERT_NE(error_promise, 0u);

    ARMD_Handle dependencies[2] = {success_promise, error_promise};
    ARMD_Handle continue_promise =
        armd_invoke(context, success_procedure, &args, 2, dependencies);
    ASSERT_NE(continue_promise, 0u);

    res = armd_detach(context, success_promise);
    ASSERT_EQ(res, 0);
    res = armd_detach(context, error_promise);
    ASSERT_EQ(res, 0);

    res = armd_await(context, continue_promise);
    ASSERT_EQ(res, -2); // error

    ASSERT_EQ(args.unwind, 3);

    res = armd_procedure_destroy(success_procedure);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(error_procedure);
    ASSERT_EQ(res, 0);
}

} // namespace
