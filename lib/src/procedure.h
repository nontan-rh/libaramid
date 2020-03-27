#ifndef ARAMID__PROCEDURE_H
#define ARAMID__PROCEDURE_H

#include <aramid/aramid.h>

#include "memory_allocator.h"

typedef struct TAG_ARMD__Continuation {
    ARMD_ContinuationFunc continuation_func;
    void *continuation_constants;
    ARMD_ContinuationFrameCreator continuation_frame_creator;
    ARMD_ContinuationFrameDestroyer continuation_frame_destroyer;
} ARMD__Continuation;

struct TAG_ARMD_Procedure {
    ARMD_MemoryAllocator memory_allocator;
    // frame
    ARMD_Size frame_size;
    // constants
    void *constants;
    // continuations
    ARMD_Size num_continuations;
    ARMD__Continuation *continuations;
    // unwind
    ARMD_UnwindFunc unwind_func;
};

#endif // ARAMID__PROCEDURE_H
