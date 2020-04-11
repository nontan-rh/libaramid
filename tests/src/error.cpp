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

void *error_continuation_frame_creator(ARMD_MemoryRegion *memory_region) {
    return armd_memory_region_allocate(memory_region, 1);
}

void error_continuation_frame_destroyer(ARMD_MemoryRegion *memory_region,
                                        void *continuation_frame) {
    armd_memory_region_free(memory_region, continuation_frame);
}

ARMD_ContinuationResult error_continuation(ARMD_Job *job, const void *constants,
                                           const void *args, void *frame,
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

TEST_F(ErrorTest, Error) {
    int res;

    ARMD_Procedure *error_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);

        void *continuation_constants =
            armd_memory_allocator_allocate(&memory_allocator, 1);
        armd_then(builder, error_continuation, continuation_constants, NULL,
                  error_continuation_frame_creator,
                  error_continuation_frame_destroyer);

        error_procedure = armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, error_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error

    res = armd_procedure_destroy(error_procedure);
    ASSERT_EQ(res, 0);
}

int single_error_continuation(ARMD_Job *job, const void *constants,
                              const void *args, void *frame) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    return 1;
}

TEST_F(ErrorTest, SingleError) {
    int res;

    ARMD_Procedure *single_error_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        armd_then_single(builder, single_error_continuation);
        single_error_procedure =
            armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle dependencies[1] = {0};
    ARMD_Handle promise =
        armd_invoke(context, single_error_procedure, nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // error

    res = armd_procedure_destroy(single_error_procedure);
    ASSERT_EQ(res, 0);
}

ARMD_Size sequential_for_error_count(const void *args, void *frame) {
    (void)args;
    (void)frame;
    return 10;
}

int sequential_for_error_continuation(ARMD_Job *job, const void *constants,
                                      const void *args, void *frame,
                                      ARMD_Size index) {
    (void)job;
    (void)constants;
    (void)args;
    (void)frame;
    (void)index;
    return 1;
}

TEST_F(ErrorTest, SequentialForError) {
    int res;

    ARMD_Procedure *sequential_for_error_procedure;
    {
        ARMD_ProcedureBuilder *builder =
            armd_procedure_builder_create(&memory_allocator, 0, 0);
        armd_then_sequential_for(builder, sequential_for_error_count,
                                 sequential_for_error_continuation);
        sequential_for_error_procedure =
            armd_procedure_builder_build_and_destroy(builder);
    }

    ARMD_Handle dependencies[1];
    ARMD_Handle promise = armd_invoke(context, sequential_for_error_procedure,
                                      nullptr, 0, dependencies);
    ASSERT_NE(promise, 0u);
    res = armd_await(context, promise);
    ASSERT_NE(res, 0); // ERROR

    res = armd_procedure_destroy(sequential_for_error_procedure);
    ASSERT_EQ(res, 0);
}

} // namespace
