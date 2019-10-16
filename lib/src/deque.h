#ifndef ARAMID__DEQUE_H
#define ARAMID__DEQUE_H

#include <aramid/aramid.h>

#include "memory_region.h"

typedef struct TAG_ARMD__Deque {
    ARMD_MemoryRegion *memory_region;
    ARMD_Size buffer_size;
    ARMD_Size head_index;
    ARMD_Size tail_index;
    ARMD_Job **buffer;
} ARMD__Deque;

ARMD_EXTERN_C ARMD__Deque *armd__deque_create(ARMD_MemoryRegion *memory_region,
                                              ARMD_Size initial_size);
ARMD_EXTERN_C int armd__deque_destroy(ARMD__Deque *deque);

ARMD_EXTERN_C ARMD_Bool armd__deque_is_empty(const ARMD__Deque *deque);
ARMD_EXTERN_C ARMD_Bool armd__deque_is_full(const ARMD__Deque *deque);

ARMD_EXTERN_C ARMD_Size armd__deque_get_num_entries(const ARMD__Deque *deque);

ARMD_EXTERN_C int armd__deque_enqueue_forward(ARMD__Deque *deque,
                                              ARMD_Job *job);
ARMD_EXTERN_C int armd__deque_dequeue_forward(ARMD__Deque *deque,
                                              ARMD_Job **result);

ARMD_EXTERN_C int armd__deque_enqueue_back(ARMD__Deque *deque, ARMD_Job *job);
ARMD_EXTERN_C int armd__deque_dequeue_back(ARMD__Deque *deque,
                                           ARMD_Job **result);

#endif // ARAMID__DEQUE_H
