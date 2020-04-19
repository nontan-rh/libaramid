#ifndef ARAMID__PROMISE_H
#define ARAMID__PROMISE_H

#include <aramid/aramid.h>

#include "hash_table.h"
#include "memory_region.h"

typedef enum TAG_ARMD__PromiseStatus {
    ARMD__PromiseStatus_NotFinished,
    ARMD__PromiseStatus_Success,
    ARMD__PromiseStatus_Error,
} ARMD__PromiseStatus;

typedef struct TAG_ARMD__PromiseCallback {
    ARMD_PromiseCallbackFunc func;
    void *context;
} ARMD__PromiseCallback;

typedef struct TAG_ARMD__Promise {
    ARMD_Bool detached;
    ARMD_Size reference_count;
    ARMD__PromiseStatus status;
    ARMD_MemoryRegion *memory_region;
    ARMD_Size num_all_waiting_promises;
    ARMD_Size num_ended_waiting_promises;
    ARMD_Bool dependency_has_error;
    ARMD_Job *pending_job;
    ARMD_Size num_continuation_promises;
    ARMD_Handle *continuation_promises;
    ARMD_Size num_promise_callbacks;
    ARMD__PromiseCallback *promise_callbacks;
} ARMD__Promise;

ARMD_EXTERN_C ARMD__Promise *
armd__promise_create_no_pending_job(ARMD_MemoryRegion *memory_region);
ARMD_EXTERN_C ARMD__Promise *
armd__promise_create_with_pending_job(ARMD_MemoryRegion *memory_region,
                                      ARMD_Size num_waiting_promises,
                                      ARMD_Job *pending_job);
ARMD_EXTERN_C int armd__promise_destroy(ARMD__Promise *promise);

ARMD_EXTERN_C int
armd__promise_add_continuation_promise(ARMD__Promise *promise,
                                       ARMD_Handle continuation_promise);
ARMD_EXTERN_C ARMD_Size armd__promise_remove_continuation_promise(
    ARMD__Promise *promise, ARMD_Handle continuation_promise);

ARMD_EXTERN_C int armd__promise_add_promise_callback(
    ARMD__Promise *promise, const ARMD__PromiseCallback *promise_callback);

ARMD_EXTERN_C void armd__promise_detach(ARMD__Promise *promise);
ARMD_EXTERN_C void
armd__promise_increment_reference_count(ARMD__Promise *promise);
ARMD_EXTERN_C void armd__promise_add_reference_count(ARMD__Promise *promise,
                                                     ARMD_Size value);
ARMD_EXTERN_C int
armd__promise_decrement_reference_count(ARMD__Promise *promise);

#endif // ARAMID__PROMISE_H
