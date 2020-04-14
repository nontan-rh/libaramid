#include <assert.h>

#include "memory_region.h"
#include "promise.h"

ARMD__Promise *
armd__promise_create_no_pending_job(ARMD_MemoryRegion *memory_region) {
    assert(memory_region != NULL);

    int promise_initialized = 0;
    int continuation_promises_initialized = 0;
    int promise_callbacks_initialized = 0;

    ARMD__Promise *promise;

    promise = armd_memory_region_allocate(memory_region, sizeof(ARMD__Promise));
    if (promise == NULL) {
        goto error;
    }
    promise_initialized = 1;

    promise->detached = 0;
    promise->reference_count = 1;
    promise->status = ARMD__PromiseStatus_NotFinished;

    promise->memory_region = memory_region;

    promise->num_all_waiting_promises = 0;
    promise->num_ended_waiting_promises = 0;
    promise->error_in_waiting_promises = 0;
    promise->pending_job = NULL;

    promise->num_continuation_promises = 0;
    promise->continuation_promises =
        armd_memory_region_allocate(memory_region, sizeof(ARMD_Handle) * 1);
    if (promise->continuation_promises == NULL) {
        goto error;
    }
    continuation_promises_initialized = 1;

    promise->num_promise_callbacks = 0;
    promise->promise_callbacks = armd_memory_region_allocate(
        memory_region, sizeof(ARMD__PromiseCallback) * 1);
    if (promise->promise_callbacks == NULL) {
        goto error;
    }
    promise_callbacks_initialized =
        1; // NOLINT(clang-analyzer-deadcode.DeadStores)

    return promise;

error:
    if (promise_callbacks_initialized) {
        armd_memory_region_free(memory_region, promise->promise_callbacks);
    }

    if (continuation_promises_initialized) {
        armd_memory_region_free(memory_region, promise->continuation_promises);
    }

    if (promise_initialized) {
        armd_memory_region_free(memory_region, promise);
    }

    return NULL;
}

ARMD__Promise *
armd__promise_create_with_pending_job(ARMD_MemoryRegion *memory_region,
                                      ARMD_Size num_waiting_promises,
                                      ARMD_Job *pending_job) {
    assert(memory_region != NULL);
    assert(num_waiting_promises != 0);
    assert(pending_job != NULL);

    int promise_initialized = 0;
    int continuation_promises_initialized = 0;
    int promise_callbacks_initialized = 0;

    ARMD__Promise *promise;

    promise = armd_memory_region_allocate(memory_region, sizeof(ARMD__Promise));
    if (promise == NULL) {
        goto error;
    }
    promise_initialized = 1;

    promise->detached = 0;
    promise->reference_count = 1;
    promise->status = ARMD__PromiseStatus_NotFinished;

    promise->memory_region = memory_region;

    promise->num_all_waiting_promises = num_waiting_promises;
    promise->num_ended_waiting_promises = 0;
    promise->error_in_waiting_promises = 0;
    promise->pending_job = pending_job;

    promise->num_continuation_promises = 0;
    promise->continuation_promises =
        armd_memory_region_allocate(memory_region, sizeof(ARMD_Handle) * 1);
    if (promise->continuation_promises == NULL) {
        goto error;
    }
    continuation_promises_initialized = 1;

    promise->num_promise_callbacks = 0;
    promise->promise_callbacks = armd_memory_region_allocate(
        memory_region, sizeof(ARMD__PromiseCallback) * 1);
    if (promise->promise_callbacks == NULL) {
        goto error;
    }
    promise_callbacks_initialized =
        1; // NOLINT(clang-analyzer-deadcode.DeadStores)

    return promise;

error:
    if (promise_callbacks_initialized) {
        armd_memory_region_free(memory_region, promise->promise_callbacks);
    }

    if (continuation_promises_initialized) {
        armd_memory_region_free(memory_region, promise->continuation_promises);
    }

    if (promise_initialized) {
        armd_memory_region_free(memory_region, promise);
    }

    return NULL;
}

int armd__promise_destroy(ARMD__Promise *promise) {
    assert(promise != NULL);

    assert(promise->reference_count == 0);

    ARMD_MemoryRegion *memory_region = promise->memory_region;

    armd_memory_region_free(memory_region, promise->continuation_promises);
    armd_memory_region_free(memory_region, promise->promise_callbacks);
    armd_memory_region_free(memory_region, promise);

    return 0;
}

int armd__promise_add_continuation_promise(ARMD__Promise *promise,
                                           ARMD_Handle continuation_promise) {
    assert(promise != NULL);

    assert(!promise->detached);
    assert(promise->reference_count >= 1);

    ARMD_Size new_num_continuation_promises =
        promise->num_continuation_promises + 1;
    ARMD_Handle *new_continuation_promises = armd_memory_region_allocate(
        promise->memory_region,
        sizeof(ARMD_Handle) * new_num_continuation_promises);

    if (new_continuation_promises == NULL) {
        return -1;
    }

    for (ARMD_Size i = 0; i < promise->num_continuation_promises; i++) {
        new_continuation_promises[i] = promise->continuation_promises[i];
    }
    new_continuation_promises[promise->num_continuation_promises] =
        continuation_promise;

    armd_memory_region_free(promise->memory_region,
                            promise->continuation_promises);

    promise->num_continuation_promises = new_num_continuation_promises;
    promise->continuation_promises = new_continuation_promises;

    return 0;
}

ARMD_Size
armd__promise_remove_continuation_promise(ARMD__Promise *promise,
                                          ARMD_Handle continuation_promise) {
    assert(promise != NULL);

    assert(promise->reference_count >= 1);

    int remove_count = 0;
    for (ARMD_Size i = 0; i < promise->num_continuation_promises; i++) {
        if (promise->continuation_promises[i] == continuation_promise) {
            promise->continuation_promises[i] = 0;
            ++remove_count;
        }
    }

    return remove_count;
}

int armd__promise_add_promise_callback(
    ARMD__Promise *promise, const ARMD__PromiseCallback *promise_callback) {
    assert(promise != NULL);
    assert(promise_callback != NULL);

    assert(!promise->detached);
    assert(promise->reference_count >= 1);

    ARMD_Size new_num_promise_callbacks = promise->num_promise_callbacks + 1;
    ARMD__PromiseCallback *new_promise_callbacks = armd_memory_region_allocate(
        promise->memory_region,
        sizeof(ARMD__PromiseCallback) * new_num_promise_callbacks);

    if (new_promise_callbacks == NULL) {
        return -1;
    }

    for (ARMD_Size i = 0; i < promise->num_promise_callbacks; i++) {
        new_promise_callbacks[i] = promise->promise_callbacks[i];
    }
    new_promise_callbacks[promise->num_promise_callbacks] = *promise_callback;

    armd_memory_region_free(promise->memory_region, promise->promise_callbacks);

    promise->num_promise_callbacks = new_num_promise_callbacks;
    promise->promise_callbacks = new_promise_callbacks;

    return 0;
}

void armd__promise_detach(ARMD__Promise *promise) {
    assert(promise->reference_count >= 1);

    promise->detached = 1;
}

void armd__promise_increment_reference_count(ARMD__Promise *promise) {
    assert(promise->reference_count >= 1);

    ++promise->reference_count;
}

int armd__promise_decrement_reference_count(ARMD__Promise *promise) {
    assert(promise->reference_count >= 1);

    --promise->reference_count;
    if (promise->reference_count == 0) {
        return 1;
    } else {
        return 0;
    }
}
