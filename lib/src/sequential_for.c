#include <aramid/aramid.h>

#include "sequential_for.h"

static ARMD_Bool continuation_func(ARMD_Job *job, const void *constants,
                                   const void *args, void *frame,
                                   const void *continuation_constants,
                                   void *continuation_frame) {
    ARMD__SequentialForContinuationConstants
        *sequential_for_continuation_constants =
            (ARMD__SequentialForContinuationConstants *)continuation_constants;
    ARMD__SequentialForContinuationFrame *sequential_for_continuation_frame =
        (ARMD__SequentialForContinuationFrame *)continuation_frame;

    if (sequential_for_continuation_frame->is_first_time) {
        sequential_for_continuation_frame->count =
            sequential_for_continuation_constants->sequential_for_count_func(
                args, frame);
        sequential_for_continuation_frame->is_first_time = 0;
    }

    ARMD_Size index = sequential_for_continuation_frame->index++;
    if (index >= sequential_for_continuation_frame->count) {
        return 0;
    }

    sequential_for_continuation_constants->sequential_for_continuation_func(
        job, constants, args, frame, index);

    if (index == sequential_for_continuation_frame->count - 1) {
        return 0;
    }

    return 1;
}

static void *continuation_frame_creator(ARMD_MemoryRegion *memory_region) {
    ARMD__SequentialForContinuationFrame *continuation_frame =
        armd_memory_region_allocate(
            memory_region, sizeof(ARMD__SequentialForContinuationFrame));

    continuation_frame->is_first_time = 1;
    continuation_frame->count = 0;
    continuation_frame->index = 0;

    return continuation_frame;
}

static void continuation_frame_destroyer(ARMD_MemoryRegion *memory_region,
                                         void *continuation_frame) {
    armd_memory_region_free(memory_region, continuation_frame);
}

int armd_then_sequential_for(
    ARMD_ProcedureBuilder *procedure_builder,
    ARMD_SequentialForCountFunc sequential_for_count_func,
    ARMD_SequentialForContinuationFunc sequential_for_continuation_func) {
    if (sequential_for_count_func == NULL) {
        return -1;
    }

    if (sequential_for_continuation_func == NULL) {
        return -1;
    }

    ARMD_MemoryAllocator memory_allocator =
        armd_procedure_builder_get_memory_allocator(procedure_builder);

    ARMD__SequentialForContinuationConstants *continuation_constants =
        armd_memory_allocator_allocate(
            &memory_allocator,
            sizeof(ARMD__SequentialForContinuationConstants));

    continuation_constants->sequential_for_count_func =
        sequential_for_count_func;
    continuation_constants->sequential_for_continuation_func =
        sequential_for_continuation_func;

    return armd_then(procedure_builder, continuation_func,
                     continuation_constants, continuation_frame_creator,
                     continuation_frame_destroyer);
}
