#include <gtest/gtest.h>

#include "src/mapalloc.h"

TEST(MapAllocTest, UsesFileWhenOverThreshold) {
    // Force file-backed allocation by setting threshold to 0
    MapAlloc::CacheThreshold(0);
    void* p = MapAlloc::Alloc(4096);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(MapAlloc::LastFile()) << "Allocation should be file-backed";
    EXPECT_EQ(MapAlloc::GetSize(p), static_cast<size_t>(4096));
    MapAlloc::Free(p);
}

TEST(MapAllocTest, UsesMemoryWhenUnderThreshold) {
    // Large threshold to keep allocations in memory
    MapAlloc::CacheThreshold(static_cast<size_t>(~0ULL));
    void* p = MapAlloc::Alloc(8192);
    ASSERT_NE(p, nullptr);
    EXPECT_FALSE(MapAlloc::LastFile()) << "Allocation should be memory-backed";
    EXPECT_EQ(MapAlloc::GetSize(p), static_cast<size_t>(8192));
    MapAlloc::Free(p);
}

