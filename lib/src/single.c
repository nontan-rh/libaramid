#include <aramid/aramid.h>

#include "single.h"

static ARMD_ContinuationResult
continuation_func(ARMD_Job *job, const void *constants, void *args, void *frame,
                  const void *continuation_constants,
                  void *continuation_frame) {
    (void)continuation_frame;

    ARMD__SingleContinuationConstants *single_continuation_constants =
        (ARMD__SingleContinuationConstants *)continuation_constants;
    int single_continuation_result =
        single_continuation_constants->single_continuation_func(job, constants,
                                                                args, frame);

    if (single_continuation_result == 0) {
        return ARMD_ContinuationResult_Ended;
    } else {
        return ARMD_ContinuationResult_Error;
    }
}

static void *continuation_frame_creator(ARMD_MemoryRegion *memory_region) {
    return armd_memory_region_allocate(memory_region, 1);
}

static void continuation_frame_destroyer(ARMD_MemoryRegion *memory_region,
                                         void *continuation_frame) {
    armd_memory_region_free(memory_region, continuation_frame);
}

int armd_then_single(ARMD_ProcedureBuilder *procedure_builder,
                     ARMD_SingleContinuationFunc single_continuation_func) {
    if (single_continuation_func == NULL) {
        return -1;
    }

    ARMD_MemoryAllocator memory_allocator =
        armd_procedure_builder_get_memory_allocator(procedure_builder);

    ARMD__SingleContinuationConstants *continuation_constants =
        armd_memory_allocator_allocate(
            &memory_allocator, sizeof(ARMD__SingleContinuationConstants));

    continuation_constants->single_continuation_func = single_continuation_func;

    return armd_then(procedure_builder, continuation_func,
                     continuation_constants, NULL, continuation_frame_creator,
                     continuation_frame_destroyer);
}
