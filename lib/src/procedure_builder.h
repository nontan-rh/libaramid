#ifndef ARAMID__PROCEDURE_BUILDER_H
#define ARAMID__PROCEDURE_BUILDER_H

#include <aramid/aramid.h>

#include "memory_allocator.h"
#include "procedure.h"

struct TAG_ARMD_ProcedureBuilder {
    ARMD_MemoryAllocator memory_allocator;
    // frame
    ARMD_Size frame_size;
    // constants
    void *constants;
    // continuations
    ARMD_Size num_continuations;
    ARMD_Size continuation_buffer_size;
    ARMD__Continuation *continuation_buffer;
};

#endif
