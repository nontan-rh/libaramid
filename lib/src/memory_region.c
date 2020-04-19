#include <assert.h>
#include <string.h>

#include "memory_allocator.h"
#include "memory_region.h"
#include "spinlock.h"

#ifdef ARAMID_DISABLE_MEMORY_REGION

ARMD_MemoryRegion *
armd_memory_region_create(const ARMD_MemoryAllocator *memory_allocator) {
    ARMD_MemoryRegion *memory_region = armd_memory_allocator_allocate(
        memory_allocator, sizeof(ARMD_MemoryRegion));

    memory_region->memory_allocator = *memory_allocator;

    return memory_region;
}

ARMD_Size armd_memory_region_destroy(ARMD_MemoryRegion *memory_region) {
    int res = 0;
    (void)res;

    if (memory_region == NULL) {
        return 0;
    }

    ARMD_MemoryAllocator memory_allocator = memory_region->memory_allocator;

    armd_memory_allocator_free(&memory_allocator, memory_region);

    return 0;
}

void *armd_memory_region_allocate(ARMD_MemoryRegion *memory_region,
                                  ARMD_Size size) {
    return armd_memory_allocator_allocate(&memory_region->memory_allocator,
                                          size);
}

void armd_memory_region_free(ARMD_MemoryRegion *memory_region, void *buf) {
    armd_memory_allocator_free(&memory_region->memory_allocator, buf);
}

#else

static const ARMD_Size header_alignment = 16;

static ARMD_Size get_header_size() {
    return (sizeof(ARMD__MemoryAllocationHeader) + header_alignment - 1) /
           header_alignment * header_alignment;
}

static int allocate_with_header(const ARMD_MemoryAllocator *memory_allocator,
                                ARMD_Size body_size,
                                ARMD__MemoryAllocationHeader **header,
                                void **body) {
    ARMD_Size header_size = get_header_size();
    ARMD_Size total_size = header_size + body_size;

    unsigned char *buf =
        armd_memory_allocator_allocate(memory_allocator, total_size);
    if (buf == NULL) {
        *header = NULL;
        *body = NULL;
        return -1;
    }

    *header = (ARMD__MemoryAllocationHeader *)buf;
    *body = (void *)(buf + header_size);
    return 0;
}

static ARMD__MemoryAllocationHeader *get_header_by_body(void *body) {
    ARMD_Size header_size = get_header_size();
    return (ARMD__MemoryAllocationHeader *)(((unsigned char *)body) -
                                            header_size);
}

static void free_by_header(const ARMD_MemoryAllocator *memory_allocator,
                           ARMD__MemoryAllocationHeader *header) {
    armd_memory_allocator_free(memory_allocator, header);
}

ARMD_MemoryRegion *
armd_memory_region_create(const ARMD_MemoryAllocator *memory_allocator) {
    ARMD_MemoryRegion *memory_region = armd_memory_allocator_allocate(
        memory_allocator, sizeof(ARMD_MemoryRegion));
    if (memory_region == NULL) {
        return NULL;
    }

    ARMD__MemoryAllocationHeader *sentinel_header;
    void *sentinel_body;
    if (allocate_with_header(memory_allocator, 1, &sentinel_header,
                             &sentinel_body)) {
        armd_memory_allocator_free(memory_allocator, memory_region);
        return NULL;
    }

    sentinel_header->next = sentinel_header;
    sentinel_header->prev = sentinel_header;

    memory_region->memory_allocator = *memory_allocator;
    memory_region->ring = sentinel_header;

    if (armd__spinlock_init(&memory_region->lock)) {
        armd_memory_allocator_free(memory_allocator, memory_region);
        return NULL;
    }

    return memory_region;
}

ARMD_Size armd_memory_region_destroy(ARMD_MemoryRegion *memory_region) {
    int res = 0;
    (void)res;

    if (memory_region == NULL) {
        return 0;
    }

    ARMD_MemoryAllocator memory_allocator = memory_region->memory_allocator;

    res = armd__spinlock_lock(&memory_region->lock);
    assert(res == 0);

    ARMD_Size non_freed_count = 0;

    ARMD__MemoryAllocationHeader *header = memory_region->ring->next;
    while (header != memory_region->ring) {
        ARMD__MemoryAllocationHeader *next = header->next;
        free_by_header(&memory_allocator, header);
        header = next;

        ++non_freed_count;
    }
    free_by_header(&memory_allocator, memory_region->ring);

    res = armd__spinlock_unlock(&memory_region->lock);
    assert(res == 0);

    armd_memory_allocator_free(&memory_allocator, memory_region);

    return non_freed_count;
}

void *armd_memory_region_allocate(ARMD_MemoryRegion *memory_region,
                                  ARMD_Size size) {
    int res = 0;
    (void)res;

    ARMD__MemoryAllocationHeader *header;
    void *body;
    if (allocate_with_header(&memory_region->memory_allocator, size, &header,
                             &body)) {
        return NULL;
    }

    res = armd__spinlock_lock(&memory_region->lock);
    assert(res == 0);

    header->prev = memory_region->ring;
    header->next = memory_region->ring->next;

    memory_region->ring->next = header;
    header->next->prev = header;

    res = armd__spinlock_unlock(&memory_region->lock);
    assert(res == 0);

    return body;
}

void armd_memory_region_free(ARMD_MemoryRegion *memory_region, void *buf) {
    int res = 0;
    (void)res;

    ARMD__MemoryAllocationHeader *header = get_header_by_body(buf);

    res = armd__spinlock_lock(&memory_region->lock);
    assert(res == 0);

    ARMD__MemoryAllocationHeader *next = header->next;
    ARMD__MemoryAllocationHeader *prev = header->prev;

    header->prev->next = next;
    header->next->prev = prev;

    res = armd__spinlock_unlock(&memory_region->lock);
    assert(res == 0);

    header->next = NULL;
    header->prev = NULL;
    free_by_header(&memory_region->memory_allocator, header);
}

#endif

char *armd_memory_region_strdup(ARMD_MemoryRegion *memory_region,
                                const char *str) {
    size_t length = strlen(str);
    char *result = armd_memory_region_allocate(memory_region, length + 1);
    memcpy(result, str, length);
    result[length] = 0;
    return result;
}
