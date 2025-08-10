#include <gtest/gtest.h>
#include <cstdint>

#include "src/mapalloc.h"

static bool IsAligned(void* p, size_t align) {
    return (reinterpret_cast<uintptr_t>(p) % align) == 0;
}

TEST(MapAllocEdgeTest, AlignmentRespectedInMemory) {
    MapAlloc::CacheThreshold(static_cast<size_t>(~0ULL));
    constexpr size_t kAlign = 64;
    void* p = MapAlloc::Alloc(12345, static_cast<int>(kAlign));
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(IsAligned(p, kAlign));
    EXPECT_FALSE(MapAlloc::LastFile());
    MapAlloc::Free(p);
}

TEST(MapAllocEdgeTest, AlignmentRespectedWhenFileBacked) {
    MapAlloc::CacheThreshold(0);
    constexpr size_t kAlign = 64;
    void* p = MapAlloc::Alloc(4096, static_cast<int>(kAlign));
    ASSERT_NE(p, nullptr);
    // mmap returns page-aligned memory; should satisfy 64B alignment
    EXPECT_TRUE(IsAligned(p, kAlign));
    EXPECT_TRUE(MapAlloc::LastFile());
    MapAlloc::Free(p);
}

