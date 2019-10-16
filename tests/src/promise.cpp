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

// int single_normal_continuation(ARMD_Job *job, const void *constants,
//                                const void *args, void *frame) {
//     (void)constants;

//     // For compilation on windows
//     fprintf(stderr, "executor: %u, args: %p, frame: %p\n",
//             (unsigned int)armd_job_get_executor_id(job), args, frame);
//     return 0;
// }

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

// TEST_F(PromiseTest, ChainSingleNormalProcedure) {
//     int res;

//     ARMD_Procedure *single_normal_procedure;
//     {
//         ARMD_ProcedureBuilder *builder =
//             armd_procedure_builder_create(&memory_allocator, 0, 0);
//         armd_then_single(builder, single_normal_continuation);
//         single_normal_procedure =
//             armd_procedure_builder_build_and_destroy(builder);
//     }

//     ARMD_Handle dependencies[1];
//     ARMD_Handle promise =
//         armd_invoke(context, single_normal_procedure, nullptr, 0,
//         dependencies);
//     ASSERT_NE(promise, 0);
//     res = armd_await(context, promise);
//     ASSERT_EQ(res, 0);

//     res = armd_procedure_destroy(single_normal_procedure);
//     ASSERT_EQ(res, 0);
// }

} // namespace
