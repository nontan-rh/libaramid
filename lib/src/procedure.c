#include <assert.h>

#include <aramid/aramid.h>

#include "memory_allocator.h"
#include "procedure.h"

int armd_procedure_destroy(ARMD_Procedure *procedure) {
    assert(procedure != NULL);

    ARMD_MemoryAllocator memory_allocator = procedure->memory_allocator;

    armd_memory_allocator_free(&memory_allocator, procedure->constants);
    procedure->constants = NULL;

    for (ARMD_Size i = 0; i < procedure->num_continuations; i++) {
        ARMD__Continuation continuation = procedure->continuations[i];
        continuation.continuation_constants_destroyer(
            &memory_allocator, continuation.continuation_constants);
        continuation.continuation_constants = NULL;
    }

    armd_memory_allocator_free(&memory_allocator, procedure->continuations);
    procedure->continuations = NULL;

    armd_memory_allocator_free(&memory_allocator, procedure);

    return 0;
}

void *armd_procedure_get_constants(ARMD_Procedure *procedure) {
    return procedure->constants;
}
