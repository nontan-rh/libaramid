#include <stdio.h>

#include <gtest/gtest.h>

#include <aramid/aramid.h>

namespace {

TEST(TimeTest, ChainEmptyProcedure) {
    int res;

    ARMD_MemoryAllocator memory_allocator;
    armd_memory_allocator_init_default(&memory_allocator);
    ARMD_MemoryRegion *memory_region =
        armd_memory_region_create(&memory_allocator);

    ARMD_Timespec timespec;
    res = armd_get_time(&timespec);
    ASSERT_EQ(res, 0);

    char *isotime = armd_format_time_iso8601(memory_region, &timespec);
    fprintf(stderr, "%s\n", isotime);
}

} // namespace
