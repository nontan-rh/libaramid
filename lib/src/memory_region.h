#ifndef ARAMID__MEMORY_REGION_H
#define ARAMID__MEMORY_REGION_H

#include <aramid/aramid.h>

#include "memory_allocator.h"
#include "spinlock.h"

typedef struct TAG_ARMD__MemoryAllocationHeader {
    struct TAG_ARMD__MemoryAllocationHeader *prev;
    struct TAG_ARMD__MemoryAllocationHeader *next;
} ARMD__MemoryAllocationHeader;

struct TAG_ARMD_MemoryRegion {
    ARMD_MemoryAllocator memory_allocator;

    ARMD__Spinlock lock;
    ARMD__MemoryAllocationHeader *ring;
};

#endif // ARAMID__MEMORY_REGION_H
