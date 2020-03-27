#include <stdint.h>
#include <stdio.h>

#include <gtest/gtest.h>

#include <aramid/aramid.h>

namespace {

class ExecutionTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_Context *context;

    ExecutionTest() {}

    ~ExecutionTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        context = armd_context_create(&memory_allocator, 1);
    }

    void TearDown() override { armd_context_destroy(context); }
};

int single_normal_continuation(ARMD_Job *job, const void *constants,
                               const void *args, void *frame) {
    (void)constants;
    // For compilation on windows
    fprintf(stderr, "executor: %u, args: %p, frame: %p\n",
            (unsigned int)armd_job_get_executor_id(job), args, frame);
    return 0;
}

ARMD_Size single_sequential_for_count(const void *args, void *frame) {
    (void)args;
    (void)frame;
    return 10;
}

int single_sequential_for_continuation(ARMD_Job *job, const void *constants,
                                       const void *args, void *frame,
                                       ARMD_Size index) {
    (void)constants;
    fprintf(stderr, "executor: %u, args: %p, frame: %p, index: %u\n",
            (unsigned int)armd_job_get_executor_id(job), args, frame,
            (unsigned int)index);
    return 0;
}

typedef struct TAG_NormalSpawnParentContinuationConstants {
    ARMD_Procedure *child_procedure;
} NormalSpawnParentContinuationConstants;

int single_normal_spawn_parent_continuation1(ARMD_Job *job,
                                             const void *constants,
                                             const void *args, void *frame) {
    const NormalSpawnParentContinuationConstants *typed_constants =
        reinterpret_cast<const NormalSpawnParentContinuationConstants *>(
            constants);

    fprintf(stderr, "parent 1.1 - executor: %u, args: %p, frame: %p\n",
            (unsigned int)armd_job_get_executor_id(job), args, frame);
    armd_fork(job, typed_constants->child_procedure, nullptr);
    fprintf(stderr, "parent 1.2 - executor: %u, args: %p, frame: %p\n",
            (unsigned int)armd_job_get_executor_id(job), args, frame);
    return 0;
}

int single_normal_spawn_parent_continuation2(ARMD_Job *job,
                                             const void *constants,
                                             const void *args, void *frame) {
    const NormalSpawnParentContinuationConstants *typed_constants =
        reinterpret_cast<const NormalSpawnParentContinuationConstants *>(
            constants);

    fprintf(stderr, "parent 2.1 - executor: %u, args: %p, frame: %p\n",
            (unsigned int)armd_job_get_executor_id(job), args, frame);
    armd_fork(job, typed_constants->child_procedure, nullptr);
    fprintf(stderr, "parent 2.2 - executor: %u, args: %p, frame: %p\n",
            (unsigned int)armd_job_get_executor_id(job), args, frame);
    return 0;
}

int single_normal_spawn_child_continuation(ARMD_Job *job, const void *constants,
                                           const void *args, void *frame) {
    (void)constants;
    fprintf(stderr, "child - executor: %u, args: %p, frame: %p\n",
            (unsigned int)armd_job_get_executor_id(job), args, frame);
    return 0;
}

typedef struct TAG_FibonacciArgs {
    uint64_t input;
    uint64_t *result;
} FibonacciArgs;

typedef struct TAG_FibonacciFrame {
    FibonacciArgs child_args_1;
    FibonacciArgs child_args_2;
    uint64_t child_result_1;
    uint64_t child_result_2;
} FibonacciFrame;

typedef struct TAG_FibonacciConstants {
    ARMD_Procedure *fibonacci_procedure;
} FibonacciConstants;

int fibonacci_continuation1(ARMD_Job *job, const void *constants,
                            const void *args, void *frame) {
    const FibonacciConstants *typed_constants =
        reinterpret_cast<const FibonacciConstants *>(constants);
    const FibonacciArgs *typed_args =
        reinterpret_cast<const FibonacciArgs *>(args);
    FibonacciFrame *typed_frame = reinterpret_cast<FibonacciFrame *>(frame);

    if (typed_args->input >= 2) {
        typed_frame->child_args_1.input = typed_args->input - 1;
        typed_frame->child_args_1.result = &typed_frame->child_result_1;

        typed_frame->child_args_2.input = typed_args->input - 2;
        typed_frame->child_args_2.result = &typed_frame->child_result_2;

        armd_fork(job, typed_constants->fibonacci_procedure,
                  &typed_frame->child_args_1);
        armd_fork(job, typed_constants->fibonacci_procedure,
                  &typed_frame->child_args_2);
    }

    return 0;
}

int fibonacci_continuation2(ARMD_Job *job, const void *constants,
                            const void *args, void *frame) {
    (void)job;

    const FibonacciConstants *typed_constants =
        reinterpret_cast<const FibonacciConstants *>(constants);
    const FibonacciArgs *typed_args =
        reinterpret_cast<const FibonacciArgs *>(args);
    FibonacciFrame *typed_frame = reinterpret_cast<FibonacciFrame *>(frame);

    (void)typed_constants;

    if (typed_args->input >= 2) {
        *typed_args->result =
            typed_frame->child_result_1 + typed_frame->child_result_2;
    } else {
        *typed_args->result = 1;
    }

    return 0;
}

TEST_F(ExecutionTest, ExecuteEmptyProcedure) {
    int res;

    ARMD_Procedure *empty_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        empty_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle dependencies[1];
    ARMD_Handle promise =
        armd_invoke(context, empty_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    res = armd_procedure_destroy(empty_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ExecutionTest, ExecuteSingleNormalProcedure) {
    int res;

    ARMD_Procedure *single_normal_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        armd_then_single(builder, single_normal_continuation);
        single_normal_procedure =
            armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle dependencies[1];
    ARMD_Handle promise =
        armd_invoke(context, single_normal_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    res = armd_procedure_destroy(single_normal_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ExecutionTest, ExecuteSingleSequentialForProcedure) {
    int res;

    ARMD_Procedure *single_sequential_for_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        armd_then_sequential_for(builder, single_sequential_for_count,
                                 single_sequential_for_continuation);
        single_sequential_for_procedure =
            armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle dependencies[1];
    ARMD_Handle promise = armd_invoke(context, single_sequential_for_procedure,
                                      nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    res = armd_procedure_destroy(single_sequential_for_procedure);
    ASSERT_EQ(res, 0);
}

TEST_F(ExecutionTest, ExecuteSpawnForProcedure) {
    int res;

    ARMD_Procedure *single_normal_spawn_parent_procedure;
    {
        ARMD_ProcedureBuilder *builder = armd_procedure_builder_create(
            &memory_allocator, sizeof(NormalSpawnParentContinuationConstants),
            0);
        armd_then_single(builder, single_normal_spawn_parent_continuation1);
        armd_then_single(builder, single_normal_spawn_parent_continuation2);
        single_normal_spawn_parent_procedure =
            armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Procedure *single_normal_spawn_child_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        armd_then_single(builder, single_normal_spawn_child_continuation);
        single_normal_spawn_child_procedure =
            armd_procedure_builder_build_and_destroy(builder);
    }

    NormalSpawnParentContinuationConstants *parent_constants =
        reinterpret_cast<NormalSpawnParentContinuationConstants *>(
            armd_procedure_get_constants(single_normal_spawn_parent_procedure));
    parent_constants->child_procedure = single_normal_spawn_child_procedure;

    ARMD_Handle dependencies[1];
    ARMD_Handle promise =
        armd_invoke(context, single_normal_spawn_parent_procedure, nullptr, 0,
                    dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    res = armd_procedure_destroy(single_normal_spawn_parent_procedure);
    ASSERT_EQ(res, 0);
    res = armd_procedure_destroy(single_normal_spawn_child_procedure);
    ASSERT_EQ(res, 0);
}

uint64_t fibonacci_sequential(uint64_t input) {
    if (input >= 2) {
        return fibonacci_sequential(input - 1) +
               fibonacci_sequential(input - 2);
    }

    return 1;
}

TEST_F(ExecutionTest, ExecuteFibonacci) {
    int res;

    ARMD_Procedure *fibonacci_procedure;
    {
        ARMD_ProcedureBuilder *builder = armd_procedure_builder_create(
            &memory_allocator, sizeof(FibonacciConstants),
            sizeof(FibonacciFrame));
        armd_then_single(builder, fibonacci_continuation1);
        armd_then_single(builder, fibonacci_continuation2);
        fibonacci_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    FibonacciConstants *fibonacci_constants =
        reinterpret_cast<FibonacciConstants *>(
            armd_procedure_get_constants(fibonacci_procedure));
    fibonacci_constants->fibonacci_procedure = fibonacci_procedure;

    const uint64_t input = 20;

    FibonacciArgs args;
    uint64_t result;

    args.input = input;
    args.result = &result;

    ARMD_Handle dependencies[1];
    ARMD_Handle promise =
        armd_invoke(context, fibonacci_procedure, &args, 0, dependencies);
    ASSERT_NE(promise, 0u);

    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    ASSERT_EQ(result, fibonacci_sequential(input));
    fprintf(stderr, "fib(%u) = %u\n", (unsigned)input, (unsigned)result);

    res = armd_procedure_destroy(fibonacci_procedure);
    ASSERT_EQ(res, 0);
}

typedef struct TAG_UnwindArgs {
    bool *unwind;
} UnwindArgs;

typedef struct TAG_UnwindFrame {
} UnwindFrame;

int unwind_continuation(ARMD_Job *job, const void *constants, const void *args,
                        void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    return 0;
}

void unwind_unwind(ARMD_Job *job, const void *constants, void *args,
                   void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;

    *((UnwindArgs *)args)->unwind = true;
}

TEST_F(ExecutionTest, ExecuteUnwind) {
    int res;

    ARMD_Procedure *unwind_procedure;
    {
        ARMD_ProcedureBuilder *builder = armd_procedure_builder_create(
            &memory_allocator, 0, sizeof(FibonacciFrame));
        armd_then_single(builder, unwind_continuation);
        armd_unwind(builder, unwind_unwind);
        unwind_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    UnwindArgs args;
    bool unwind = false;

    args.unwind = &unwind;

    ARMD_Handle dependencies[1];
    ARMD_Handle promise =
        armd_invoke(context, unwind_procedure, &args, 0, dependencies);
    ASSERT_NE(promise, 0u);

    res = armd_await(context, promise);
    ASSERT_EQ(res, 0);

    ASSERT_TRUE(unwind);

    res = armd_procedure_destroy(unwind_procedure);
    ASSERT_EQ(res, 0);
}

} // namespace
