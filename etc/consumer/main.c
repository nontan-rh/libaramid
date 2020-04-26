#include <aramid/aramid.h>

int main(void) {
    ARMD_MemoryAllocator memory_allocator;
    armd_memory_allocator_init_default(&memory_allocator);
    ARMD_Context *context = armd_context_create(&memory_allocator, 1);
    armd_context_destroy(context);
    return 0;
}
