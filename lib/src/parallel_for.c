#include <assert.h>

#include <aramid/aramid.h>

#include "parallel_for.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

static ARMD_ContinuationResult
process(ARMD_Job *job,
        ARMD_ParallelForContinuationFunc parallel_for_continuation_func,
        ARMD__ParallelForChildProcedureArgs *child_args) {
    ARMD_Size index;

#if defined(__GNUC__) || defined(__clang__)
    index = __atomic_fetch_add(&child_args->parent_continuation_frame->index, 1,
                               __ATOMIC_RELAXED);
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
    if (sizeof(ARMD_Size) == 4) {
        index = InterlockedIncrement(
                    (LONG *)&child_args->parent_continuation_frame->index) -
                1;
    } else if (sizeof(ARMD_Size) == 8) {
        index = InterlockedIncrement64(
                    (LONG64 *)&child_args->parent_continuation_frame->index) -
                1;
    } else {
        assert(0);
    }
#pragma warning(pop)
#else
#error Spinlock implementation is not specified
#endif

    if (index >= child_args->parent_continuation_frame->count) {
        return ARMD_ContinuationResult_Ended;
    }

    int continuation_error = parallel_for_continuation_func(
        job, child_args->parent_constants, child_args->parent_args,
        child_args->parent_frame, index);
    if (continuation_error) {
        return ARMD_ContinuationResult_Error;
    }

    return ARMD_ContinuationResult_Repeat;
}

static ARMD_ContinuationResult
parent_continuation_func(ARMD_Job *job, const void *constants, void *args,
                         void *frame, const void *continuation_constants,
                         void *continuation_frame) {
    ARMD__ParallelForContinuationConstants
        *parallel_for_continuation_constants =
            (ARMD__ParallelForContinuationConstants *)continuation_constants;
    ARMD__ParallelForContinuationFrame *parallel_for_continuation_frame =
        (ARMD__ParallelForContinuationFrame *)continuation_frame;
    ARMD__ParallelForChildProcedureArgs *child_args =
        &parallel_for_continuation_frame->child_args;

    if (parallel_for_continuation_frame->is_first_time) {
        parallel_for_continuation_frame->count =
            parallel_for_continuation_constants->parallel_for_count_func(args,
                                                                         frame);

        child_args->parent_continuation_frame = parallel_for_continuation_frame;
        child_args->parent_constants = constants;
        child_args->parent_args = args;
        child_args->parent_frame = frame;

        ARMD_Size num_executors = armd_job_get_num_executors(job);
        for (ARMD_Size executor_id = 0; executor_id < num_executors;
             executor_id++) {
            armd_fork_with_id(
                executor_id, job,
                parallel_for_continuation_constants->child_procedure,
                child_args);
        }

        parallel_for_continuation_frame->is_first_time = 0;
    }

    return ARMD_ContinuationResult_Ended;
}

static ARMD_ContinuationResult
child_continuation_func(ARMD_Job *job, const void *constants, void *args,
                        void *frame, const void *continuation_constants,
                        void *continuation_frame) {
    (void)frame;
    (void)continuation_constants;
    (void)continuation_frame;

    return process(job,
                   ((ARMD__ParallelForChildProcedureConstants *)constants)
                       ->parallel_for_continuation_func,
                   (ARMD__ParallelForChildProcedureArgs *)args);
}

static void
parent_continuation_constants_destroyer(ARMD_MemoryAllocator *memory_region,
                                        void *continuation_constants) {
    armd_procedure_destroy(
        ((ARMD__ParallelForContinuationConstants *)continuation_constants)
            ->child_procedure);
    armd_memory_allocator_free(memory_region, continuation_constants);
}

static void *
parent_continuation_frame_creator(ARMD_MemoryRegion *memory_region) {
    ARMD__ParallelForContinuationFrame *continuation_frame =
        armd_memory_region_allocate(memory_region,
                                    sizeof(ARMD__ParallelForContinuationFrame));

    continuation_frame->is_first_time = 1;
    continuation_frame->count = 0;
    continuation_frame->index = 0;

    return continuation_frame;
}

static void
parent_continuation_frame_destroyer(ARMD_MemoryRegion *memory_region,
                                    void *continuation_frame) {
    armd_memory_region_free(memory_region, continuation_frame);
}

static void
child_continuation_constants_destroyer(ARMD_MemoryAllocator *memory_region,
                                       void *continuation_constants) {
    armd_memory_allocator_free(memory_region, continuation_constants);
}

static void *
child_continuation_frame_creator(ARMD_MemoryRegion *memory_region) {
    return armd_memory_region_allocate(memory_region, 1);
}

static void child_continuation_frame_destroyer(ARMD_MemoryRegion *memory_region,
                                               void *continuation_frame) {
    armd_memory_region_free(memory_region, continuation_frame);
}

static ARMD_Procedure *build_child_procedure(
    ARMD_MemoryAllocator *memory_allocator,
    ARMD_ParallelForContinuationFunc parallel_for_continuation_func) {
    ARMD_ProcedureBuilder *child_builder = armd_procedure_builder_create(
        memory_allocator, sizeof(ARMD__ParallelForChildProcedureConstants), 1);
    ARMD__ParallelForContinuationConstants *continuation_constants =
        armd_memory_allocator_allocate(
            memory_allocator, sizeof(ARMD__ParallelForContinuationConstants));
    armd_then(child_builder, child_continuation_func, continuation_constants,
              child_continuation_constants_destroyer, NULL,
              child_continuation_frame_creator,
              child_continuation_frame_destroyer);

    ((ARMD__ParallelForChildProcedureConstants *)
         armd_procedure_builder_get_constants(child_builder))
        ->parallel_for_continuation_func = parallel_for_continuation_func;

    return armd_procedure_builder_build_and_destroy(child_builder);
}

int armd_then_parallel_for(
    ARMD_ProcedureBuilder *procedure_builder,
    ARMD_ParallelForCountFunc parallel_for_count_func,
    ARMD_ParallelForContinuationFunc parallel_for_continuation_func) {
    if (parallel_for_count_func == NULL) {
        return -1;
    }

    if (parallel_for_continuation_func == NULL) {
        return -1;
    }

    ARMD_MemoryAllocator memory_allocator =
        armd_procedure_builder_get_memory_allocator(procedure_builder);

    ARMD__ParallelForContinuationConstants *continuation_constants =
        armd_memory_allocator_allocate(
            &memory_allocator, sizeof(ARMD__ParallelForContinuationConstants));

    continuation_constants->parallel_for_count_func = parallel_for_count_func;
    continuation_constants->parallel_for_continuation_func =
        parallel_for_continuation_func;
    continuation_constants->child_procedure = build_child_procedure(
        &memory_allocator, parallel_for_continuation_func);

    return armd_then(
        procedure_builder, parent_continuation_func, continuation_constants,
        parent_continuation_constants_destroyer, NULL,
        parent_continuation_frame_creator, parent_continuation_frame_destroyer);
}
