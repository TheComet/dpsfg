#include "gtest/gtest.h"

extern "C" {
#include "csfg/util/mem.h"
}

#define NAME test_mem

using namespace testing;

TEST(NAME, malloc_free)
{
    void* p = mem_alloc(16);
    ASSERT_TRUE(p != nullptr);
    mem_free(p);
}

TEST(NAME, realloc_free)
{
    void* p = mem_realloc(NULL, 16);
    ASSERT_TRUE(p != nullptr);
    mem_free(p);
}

TEST(NAME, realloc_realloc_free)
{
    void* p = mem_realloc(NULL, 2);
    ASSERT_TRUE(p != nullptr);
    p = mem_realloc(p, 4);
    ASSERT_TRUE(p != nullptr);
    mem_free(p);
}
