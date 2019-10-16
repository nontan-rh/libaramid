#include <aramid/aramid.h>
#include <stdlib.h>

static void *default_allocate(void *context, ARMD_Size size) {
    (void)context;
    return calloc(1, size);
}

static void default_free(void *context, void *buf) {
    (void)context;
    free(buf);
}

void armd_memory_allocator_init_default(
    ARMD_MemoryAllocator *memory_allocator) {
    memory_allocator->allocate = default_allocate;
    memory_allocator->free = default_free;
    memory_allocator->context = NULL;
}

void *armd_memory_allocator_allocate(const ARMD_MemoryAllocator *allocator,
                                     ARMD_Size size) {
    return allocator->allocate(allocator->context, size);
}

void armd_memory_allocator_free(const ARMD_MemoryAllocator *allocator,
                                void *buf) {
    allocator->free(allocator->context, buf);
}
