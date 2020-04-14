#include <aramid/aramid.h>

#include "deque.h"
#include "memory_region.h"

ARMD__Deque *armd__deque_create(ARMD_MemoryRegion *memory_region,
                                ARMD_Size initial_size) {
    if (initial_size == 0) {
        return NULL;
    }

    ARMD__Deque *deque =
        armd_memory_region_allocate(memory_region, sizeof(ARMD__Deque));
    if (deque == NULL) {
        return NULL;
    }

    deque->memory_region = memory_region;
    deque->buffer_size = initial_size;
    deque->head_index = 0;
    deque->tail_index = 0;
    deque->buffer = armd_memory_region_allocate(
        memory_region, initial_size * sizeof(ARMD_Job *));
    if (deque->buffer == NULL) {
        armd_memory_region_free(memory_region, deque);
        return NULL;
    }

    return deque;
}

int armd__deque_destroy(ARMD__Deque *deque) {
    int status = 0;

    if (deque == NULL) {
        return -1;
    }

    if (deque->head_index != deque->tail_index) {
        status = -1;
    }

    armd_memory_region_free(deque->memory_region, deque->buffer);
    deque->buffer = NULL;

    armd_memory_region_free(deque->memory_region, deque);

    return status;
}

static ARMD_Size prev_index(ARMD_Size i, ARMD_Size m) {
    return (i + m - 1) % m;
}

static ARMD_Size next_index(ARMD_Size i, ARMD_Size m) { return (i + 1) % m; }

ARMD_Bool armd__deque_is_empty(const ARMD__Deque *deque) {
    return deque->head_index == deque->tail_index;
}

ARMD_Bool armd__deque_is_full(const ARMD__Deque *deque) {
    return next_index(deque->tail_index, deque->buffer_size) ==
           deque->head_index;
}

ARMD_Size armd__deque_get_num_entries(const ARMD__Deque *deque) {
    return (deque->buffer_size + deque->tail_index - deque->head_index) %
           deque->buffer_size;
}

int armd__deque_expand(ARMD__Deque *deque) {
    ARMD_Size new_buffer_size = deque->buffer_size * 2;
    ARMD_Job **new_buffer = armd_memory_region_allocate(
        deque->memory_region, new_buffer_size * sizeof(ARMD_Job *));
    if (new_buffer == NULL) {
        return -1;
    }

    ARMD_Size num_entries = armd__deque_get_num_entries(deque);
    for (ARMD_Size i = 0; i < num_entries; i++) {
        new_buffer[i] =
            deque->buffer[(deque->head_index + i) % deque->buffer_size];
    }

    armd_memory_region_free(deque->memory_region, deque->buffer);

    deque->buffer_size = new_buffer_size;
    deque->buffer = new_buffer;
    deque->head_index = 0;
    deque->tail_index = num_entries;

    return 0;
}

int armd__deque_enqueue_forward(ARMD__Deque *deque, ARMD_Job *job) {
    if (armd__deque_is_full(deque)) {
        int expand_result = armd__deque_expand(deque);
        if (expand_result == -1) {
            return -1;
        }
    }

    deque->head_index = prev_index(deque->head_index, deque->buffer_size);

    deque->buffer[deque->head_index] = job;

    return 0;
}

int armd__deque_dequeue_forward(ARMD__Deque *deque, ARMD_Job **result) {
    if (armd__deque_is_empty(deque)) {
        *result = NULL;
        return -1;
    }

    *result = deque->buffer[deque->head_index];
    deque->buffer[deque->head_index] = NULL;

    deque->head_index = next_index(deque->head_index, deque->buffer_size);

    return 0;
}

int armd__deque_enqueue_back(ARMD__Deque *deque, ARMD_Job *job) {
    if (armd__deque_is_full(deque)) {
        int expand_result = armd__deque_expand(deque);
        if (expand_result == -1) {
            return -1;
        }
    }

    deque->buffer[deque->tail_index] = job;

    deque->tail_index = next_index(deque->tail_index, deque->buffer_size);

    return 0;
}

int armd__deque_dequeue_back(ARMD__Deque *deque, ARMD_Job **result) {
    if (armd__deque_is_empty(deque)) {
        *result = NULL;
        return -1;
    }

    deque->tail_index = prev_index(deque->tail_index, deque->buffer_size);

    *result = deque->buffer[deque->tail_index];
    deque->buffer[deque->tail_index] = NULL;

    return 0;
}
