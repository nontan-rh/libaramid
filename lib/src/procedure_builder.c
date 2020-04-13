#include <assert.h>
#include <string.h>

#include <aramid/aramid.h>

#include "memory_allocator.h"
#include "procedure_builder.h"

ARMD_ProcedureBuilder *
armd_procedure_builder_create(const ARMD_MemoryAllocator *memory_allocator,
                              ARMD_Size constant_size, ARMD_Size frame_size) {
    const ARMD_Size initial_size = 8;

    int builder_initialized = 0;
    int constants_initialized = 0;
    int continuation_buffer_initialized = 0;

    ARMD_ProcedureBuilder *builder = NULL;

    builder = armd_memory_allocator_allocate(memory_allocator,
                                             sizeof(ARMD_ProcedureBuilder));
    if (builder == NULL) {
        goto error;
    }
    builder_initialized = 1;

    builder->memory_allocator = *memory_allocator;
    builder->frame_size = frame_size;
    builder->constants = armd_memory_allocator_allocate(
        memory_allocator, constant_size == 0 ? 1 : constant_size);
    if (builder->constants == NULL) {
        goto error;
    }
    constants_initialized = 1;

    builder->num_continuations = 0;
    builder->continuation_buffer_size = initial_size;
    builder->continuation_buffer = armd_memory_allocator_allocate(
        memory_allocator, initial_size * sizeof(ARMD__Continuation));
    if (builder->continuation_buffer == NULL) {
        goto error;
    }
    continuation_buffer_initialized =
        1; // NOLINT(clang-analyzer-deadcode.DeadStores)

    builder->unwind_func = NULL;

    return builder;

error:
    if (constants_initialized) {
        armd_memory_allocator_free(memory_allocator, builder->constants);
    }

    if (continuation_buffer_initialized) {
        armd_memory_allocator_free(memory_allocator,
                                   builder->continuation_buffer);
    }

    if (builder_initialized) {
        armd_memory_allocator_free(memory_allocator, builder);
    }

    return NULL;
}

int armd_procedure_builder_destroy(ARMD_ProcedureBuilder *builder) {
    assert(builder != NULL);

    ARMD_MemoryAllocator memory_allocator = builder->memory_allocator;

    armd_memory_allocator_free(&memory_allocator, builder->constants);
    builder->constants = NULL;

    armd_memory_allocator_free(&memory_allocator, builder->continuation_buffer);
    builder->continuation_buffer = NULL;

    armd_memory_allocator_free(&memory_allocator, builder);

    return 0;
}

void *armd_procedure_builder_get_constants(ARMD_ProcedureBuilder *builder) {
    assert(builder != NULL);
    return builder->constants;
}

ARMD_MemoryAllocator
armd_procedure_builder_get_memory_allocator(ARMD_ProcedureBuilder *builder) {
    assert(builder != NULL);
    return builder->memory_allocator;
}

static int expand_buffer(ARMD_ProcedureBuilder *builder) {
    assert(builder != NULL);

    ARMD_Size new_buffer_size = builder->continuation_buffer_size * 2;
    ARMD__Continuation *new_buffer = armd_memory_allocator_allocate(
        &builder->memory_allocator,
        new_buffer_size * sizeof(ARMD__Continuation));
    if (new_buffer == NULL) {
        return -1;
    }

    memcpy(new_buffer, builder->continuation_buffer,
           sizeof(ARMD__Continuation) * builder->num_continuations);
    armd_memory_allocator_free(&builder->memory_allocator,
                               builder->continuation_buffer);

    builder->continuation_buffer = new_buffer;
    builder->continuation_buffer_size = new_buffer_size;

    return 0;
}

static ARMD_Bool is_buffer_full(const ARMD_ProcedureBuilder *builder) {
    assert(builder != NULL);

    return builder->num_continuations >= builder->continuation_buffer_size;
}

static int ensure_buffer_space(ARMD_ProcedureBuilder *builder) {
    assert(builder != NULL);

    if (is_buffer_full(builder)) {
        int res = expand_buffer(builder);
        if (res) {
            return -1;
        }
    }

    return 0;
}

int armd_then(ARMD_ProcedureBuilder *builder,
              ARMD_ContinuationFunc continuation_func,
              void *continuation_constants, ARMD_ErrorTrapFunc error_trap_func,
              ARMD_ContinuationFrameCreator continuation_frame_creator,
              ARMD_ContinuationFrameDestroyer continuation_frame_destroyer) {
    assert(builder != NULL);

    if (continuation_func == NULL) {
        return -1;
    }

    if (continuation_constants == NULL) {
        return -1;
    }

    if (continuation_frame_creator == NULL) {
        return -1;
    }

    if (continuation_frame_destroyer == NULL) {
        return -1;
    }

    if (ensure_buffer_space(builder)) {
        return -1;
    }

    ARMD__Continuation continuation;
    continuation.continuation_func = continuation_func;
    continuation.error_trap_func = error_trap_func;
    continuation.continuation_constants = continuation_constants;
    continuation.continuation_frame_creator = continuation_frame_creator;
    continuation.continuation_frame_destroyer = continuation_frame_destroyer;

    builder->continuation_buffer[builder->num_continuations] = continuation;
    ++builder->num_continuations;

    return 0;
}

int armd_unwind(ARMD_ProcedureBuilder *builder, ARMD_UnwindFunc unwind_func) {
    assert(builder != NULL);

    if (unwind_func == NULL) {
        return -1;
    }

    if (builder->unwind_func != NULL) {
        return -1;
    }

    builder->unwind_func = unwind_func;

    return 0;
}

ARMD_Procedure *
armd_procedure_builder_build_and_destroy(ARMD_ProcedureBuilder *builder) {
    assert(builder != NULL);

    ARMD_Procedure *procedure = armd_memory_allocator_allocate(
        &builder->memory_allocator, sizeof(ARMD_Procedure));
    if (procedure == NULL) {
        armd_procedure_builder_destroy(builder);
        return NULL;
    }

    procedure->memory_allocator = builder->memory_allocator;
    procedure->continuations = builder->continuation_buffer;
    procedure->frame_size = builder->frame_size;
    procedure->constants = builder->constants;
    procedure->num_continuations = builder->num_continuations;
    procedure->unwind_func = builder->unwind_func;

    armd_memory_allocator_free(&procedure->memory_allocator, builder);

    return procedure;
}
