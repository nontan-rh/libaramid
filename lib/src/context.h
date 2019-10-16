#ifndef ARAMID__CONTEXT_H
#define ARAMID__CONTEXT_H

#include <aramid/aramid.h>

#include "condvar.h"
#include "hash_table.h"
#include "memory_region.h"
#include "mutex.h"
#include "types.h"

struct TAG_ARMD_Context {
    ARMD__Mutex executor_mutex;
    ARMD__Condvar executor_condvar;
    ARMD_Size num_executors;
    ARMD__Executor **executors;
    ARMD_MemoryAllocator memory_allocator;
    ARMD_MemoryRegion *memory_region;
    volatile ARMD_Size free_job_count;
    struct {
        ARMD__Mutex mutex;
        ARMD__Condvar condvar;
        ARMD__HashTable *promises;
        ARMD_Handle handle_counter;
    } promise_manager;
};

ARMD_EXTERN_C int armd__context_complete_promise(ARMD_Context *context,
                                                 ARMD_Handle promise_handle);

#endif // ARAMID__CONTEXT_H
