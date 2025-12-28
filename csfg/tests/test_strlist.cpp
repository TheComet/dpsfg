#include "csfg/tests/LogHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/util/strlist.h"
}

#define NAME test_strlist

using namespace ::testing;

struct NAME : Test, LogHelper
{
public:
    void            SetUp() override { strlist_init(&strlist); }
    void            TearDown() override { strlist_deinit(strlist); }
    struct strlist* strlist;
};

TEST_F(NAME, append_strings)
{
    ASSERT_EQ(strlist_add_cstr(&strlist, "Hello"), 0);
    ASSERT_EQ(strlist_add_cstr(&strlist, "World"), 0);
    ASSERT_EQ(strlist_add_cstr(&strlist, "!"), 0);
    ASSERT_EQ(strlist_count(strlist), 3);
    ASSERT_TRUE(strview_eq_cstr(strlist_view(strlist, 0), "Hello"));
    ASSERT_TRUE(strview_eq_cstr(strlist_view(strlist, 1), "World"));
    ASSERT_TRUE(strview_eq_cstr(strlist_view(strlist, 2), "!"));
}

TEST_F(NAME, many_strings)
{
    for (int i = 0; i < 1000; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "String %d", i);
        ASSERT_EQ(strlist_add_cstr(&strlist, buf), 0);
    }
    ASSERT_EQ(strlist_count(strlist), 1000);

    for (int i = 0; i < 1000; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "String %d", i);
        ASSERT_TRUE(strview_eq_cstr(strlist_view(strlist, i), buf));
    }
}
