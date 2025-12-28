#include "gtest/gtest.h"

extern "C" {
#include "csfg/util/hset.h"
}

#define NAME         test_hset
#define MIN_CAPACITY 128

using namespace testing;

namespace {
HSET_DECLARE(obj_hset, int16_t, 16)
HSET_DEFINE(obj_hset, int16_t, 16)
} // namespace

struct NAME : Test
{
    void SetUp() override { obj_hset_init(&obj_hset); }
    void TearDown() override { obj_hset_deinit(obj_hset); }

    struct obj_hset* obj_hset;
};

TEST_F(NAME, null_hset_is_set)
{
    ASSERT_EQ(hset_capacity(obj_hset), 0u);
    ASSERT_EQ(hset_count(obj_hset), 0u);
}

TEST_F(NAME, deinit_null_hset_works)
{
    obj_hset_deinit(obj_hset);
}

TEST_F(NAME, insertion_forwards)
{
    ASSERT_EQ(obj_hset_insert(&obj_hset, 0), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 1u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 1), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 2u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 2), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 3u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 3), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 4u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 4), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 5u);

    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 1));
    ASSERT_TRUE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    ASSERT_TRUE(obj_hset_find(obj_hset, 4));
    EXPECT_FALSE(obj_hset_find(obj_hset, 5));
}

TEST_F(NAME, insertion_backwards)
{
    ASSERT_EQ(obj_hset_insert(&obj_hset, 4), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 1u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 3), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 2u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 2), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 3u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 1), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 4u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 0), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 5u);

    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 1));
    ASSERT_TRUE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    ASSERT_TRUE(obj_hset_find(obj_hset, 4));
    EXPECT_FALSE(obj_hset_find(obj_hset, 5));
}

TEST_F(NAME, insertion_random)
{
    ASSERT_EQ(obj_hset_insert(&obj_hset, 26), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 1u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 44), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 2u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 82), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 3u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 41), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 4u);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 70), HSET_NEW);
    ASSERT_EQ(hset_count(obj_hset), 5u);

    ASSERT_TRUE(obj_hset_find(obj_hset, 26));
    ASSERT_TRUE(obj_hset_find(obj_hset, 41));
    ASSERT_TRUE(obj_hset_find(obj_hset, 44));
    ASSERT_TRUE(obj_hset_find(obj_hset, 70));
    ASSERT_TRUE(obj_hset_find(obj_hset, 82));
}

TEST_F(NAME, clear_keeps_underlying_buffer)
{
    obj_hset_insert(&obj_hset, 0);
    obj_hset_insert(&obj_hset, 1);
    obj_hset_insert(&obj_hset, 2);

    // this should delete all entries but keep the underlying buffer
    obj_hset_clear(obj_hset);

    ASSERT_EQ(hset_count(obj_hset), 0u);
    EXPECT_NE(hset_capacity(obj_hset), 0u);
}

// TODO:
// TEST_F(NAME, compact_when_no_buffer_is_allocated_doesnt_crash)
//{
//     obj_hset_compact(&obj_hset);
// }
//
// TEST_F(NAME, compact_reduces_capacity_and_keeps_elements_in_tact)
//{
//    for (int i = 0; i != MIN_CAPACITY * 3; ++i)
//        ASSERT_EQ(obj_hset_insert(&obj_hset, i), HSET_NEW);
//    for (int i = 0; i != MIN_CAPACITY; ++i)
//        obj_hset_erase(obj_hset, i);
//
//    int16_t old_capacity = hset_capacity(obj_hset);
//    obj_hset_compact(&obj_hset);
//    EXPECT_THAT(hset_capacity(obj_hset), Lt(old_capacity));
//    ASSERT_EQ(hset_count(obj_hset), MIN_CAPACITY * 2);
//    ASSERT_EQ(hset_capacity(obj_hset), MIN_CAPACITY * 2);
//}
//
// TEST_F(NAME, clear_and_compact_deletes_underlying_buffer)
//{
//    obj_hset_insert(&obj_hset, 0);
//    obj_hset_insert(&obj_hset, 1);
//    obj_hset_insert(&obj_hset, 2);
//
//    // this should delete all entries + free the underlying buffer
//    obj_hset_clear(obj_hset);
//    obj_hset_compact(&obj_hset);
//
//    ASSERT_EQ(hset_count(obj_hset), 0u);
//    ASSERT_EQ(hset_capacity(obj_hset), 0u);
//}

TEST_F(NAME, insert_key_twice_returns_exists)
{
    ASSERT_EQ(obj_hset_insert(&obj_hset, 0), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 0), HSET_EXISTS);
    ASSERT_EQ(hset_count(obj_hset), 1);
    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
}

TEST_F(NAME, find_on_empty_hset_doesnt_crash)
{
    EXPECT_FALSE(obj_hset_find(obj_hset, 3));
}

TEST_F(NAME, find_returns_false_if_key_doesnt_exist)
{
    obj_hset_insert(&obj_hset, 3);
    obj_hset_insert(&obj_hset, 8);
    obj_hset_insert(&obj_hset, 2);

    EXPECT_FALSE(obj_hset_find(obj_hset, 4));
}

TEST_F(NAME, find_returns_true_if_key_exists)
{
    obj_hset_insert(&obj_hset, 3);
    obj_hset_insert(&obj_hset, 8);
    obj_hset_insert(&obj_hset, 2);

    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
}

TEST_F(NAME, erase)
{
    obj_hset_insert(&obj_hset, 0);
    obj_hset_insert(&obj_hset, 1);
    obj_hset_insert(&obj_hset, 2);
    obj_hset_insert(&obj_hset, 3);
    obj_hset_insert(&obj_hset, 4);

    ASSERT_EQ(obj_hset_erase(obj_hset, 2), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 2), 0);

    // 4
    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 1));
    EXPECT_FALSE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    ASSERT_TRUE(obj_hset_find(obj_hset, 4));

    ASSERT_EQ(obj_hset_erase(obj_hset, 4), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 4), 0);

    // 3
    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 1));
    EXPECT_FALSE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    EXPECT_FALSE(obj_hset_find(obj_hset, 4));

    ASSERT_EQ(obj_hset_erase(obj_hset, 0), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 0), 0);

    // 2
    ASSERT_FALSE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 1));
    ASSERT_FALSE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    ASSERT_FALSE(obj_hset_find(obj_hset, 4));

    ASSERT_EQ(obj_hset_erase(obj_hset, 1), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 1), 0);

    // 1
    ASSERT_FALSE(obj_hset_find(obj_hset, 0));
    ASSERT_FALSE(obj_hset_find(obj_hset, 1));
    ASSERT_FALSE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    ASSERT_FALSE(obj_hset_find(obj_hset, 4));

    ASSERT_EQ(obj_hset_erase(obj_hset, 3), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 3), 0);

    // 0
    ASSERT_FALSE(obj_hset_find(obj_hset, 0));
    ASSERT_FALSE(obj_hset_find(obj_hset, 1));
    ASSERT_FALSE(obj_hset_find(obj_hset, 2));
    ASSERT_FALSE(obj_hset_find(obj_hset, 3));
    ASSERT_FALSE(obj_hset_find(obj_hset, 4));
}

TEST_F(NAME, reinsertion_forwards)
{
    ASSERT_EQ(obj_hset_insert(&obj_hset, 0), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 1), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 2), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 3), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 4), HSET_NEW);

    ASSERT_EQ(obj_hset_erase(obj_hset, 4), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 3), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 2), 1);

    ASSERT_EQ(obj_hset_insert(&obj_hset, 2), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 3), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 4), HSET_NEW);

    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 1));
    ASSERT_TRUE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    ASSERT_TRUE(obj_hset_find(obj_hset, 4));
}

TEST_F(NAME, reinsertion_backwards)
{
    ASSERT_EQ(obj_hset_insert(&obj_hset, 0), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 1), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 2), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 3), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 4), HSET_NEW);

    ASSERT_EQ(obj_hset_erase(obj_hset, 0), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 1), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 2), 1);

    ASSERT_EQ(obj_hset_insert(&obj_hset, 2), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 1), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 0), HSET_NEW);

    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 1));
    ASSERT_TRUE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 3));
    ASSERT_TRUE(obj_hset_find(obj_hset, 4));
}

TEST_F(NAME, reinsertion_random)
{
    ASSERT_EQ(obj_hset_insert(&obj_hset, 26), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 44), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 82), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 41), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 70), HSET_NEW);

    ASSERT_EQ(obj_hset_erase(obj_hset, 44), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 70), 1);
    ASSERT_EQ(obj_hset_erase(obj_hset, 26), 1);

    ASSERT_EQ(obj_hset_insert(&obj_hset, 26), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 70), HSET_NEW);
    ASSERT_EQ(obj_hset_insert(&obj_hset, 44), HSET_NEW);

    ASSERT_TRUE(obj_hset_find(obj_hset, 26));
    ASSERT_TRUE(obj_hset_find(obj_hset, 44));
    ASSERT_TRUE(obj_hset_find(obj_hset, 82));
    ASSERT_TRUE(obj_hset_find(obj_hset, 41));
    ASSERT_TRUE(obj_hset_find(obj_hset, 70));
}

TEST_F(NAME, iterate_with_no_items)
{
    int16_t idx, key;
    int     counter = 0;
    hset_for_each (obj_hset, idx, key)
    {
        (void)idx;
        (void)key;
        ++counter;
    }
    ASSERT_EQ(counter, 0);
}

TEST_F(NAME, iterate_5_random_items)
{
    obj_hset_insert(&obj_hset, 243);
    obj_hset_insert(&obj_hset, 256);
    obj_hset_insert(&obj_hset, 456);
    obj_hset_insert(&obj_hset, 468);
    obj_hset_insert(&obj_hset, 969);

    int16_t idx, key;
    int     counter = 0;
    hset_for_each (obj_hset, idx, key)
    {
        switch (counter)
        {
            case 0: ASSERT_EQ(key, 243u); break;
            case 4: ASSERT_EQ(key, 456u); break;
            case 3: ASSERT_EQ(key, 468u); break;
            case 1: ASSERT_EQ(key, 969u); break;
            case 2: ASSERT_EQ(key, 256u); break;
            default: ASSERT_FALSE(true); break;
        }
        ++counter;
    }
    ASSERT_EQ(counter, 5);
}

TEST_F(NAME, retain_empty)
{
    int counter = 0;
    EXPECT_EQ(
        obj_hset_retain(
            obj_hset,
            [](int16_t, void* counter)
            {
                ++*(int*)counter;
                return HSET_RETAIN;
            },
            &counter),
        0);
    ASSERT_EQ(hset_count(obj_hset), 0);
    ASSERT_EQ(counter, 0);
}

TEST_F(NAME, retain_all)
{
    for (int16_t i = 0; i != 8; ++i)
        obj_hset_insert(&obj_hset, i);

    int counter = 0;
    ASSERT_EQ(hset_count(obj_hset), 8);
    EXPECT_EQ(
        obj_hset_retain(
            obj_hset,
            [](int16_t, void* counter)
            {
                ++*(int*)counter;
                return HSET_RETAIN;
            },
            &counter),
        0);
    ASSERT_EQ(hset_count(obj_hset), 8);
    ASSERT_EQ(counter, 8);
}

TEST_F(NAME, retain_half)
{
    for (int16_t i = 0; i != 8; ++i)
        obj_hset_insert(&obj_hset, i);

    ASSERT_EQ(hset_count(obj_hset), 8);
    ASSERT_EQ(
        obj_hset_retain(
            obj_hset, [](int16_t key, void*) { return key % 2; }, NULL),
        0);
    ASSERT_EQ(hset_count(obj_hset), 4);
    ASSERT_TRUE(obj_hset_find(obj_hset, 0));
    ASSERT_TRUE(obj_hset_find(obj_hset, 2));
    ASSERT_TRUE(obj_hset_find(obj_hset, 4));
    ASSERT_TRUE(obj_hset_find(obj_hset, 6));
}

TEST_F(NAME, retain_returning_error)
{
    for (int16_t i = 0; i != 8; ++i)
        ASSERT_EQ(obj_hset_insert(&obj_hset, i), HSET_NEW);

    ASSERT_EQ(hset_count(obj_hset), 8);
    EXPECT_EQ(
        obj_hset_retain(obj_hset, [](int16_t, void*) { return -5; }, NULL), -5);
    ASSERT_EQ(hset_count(obj_hset), 8);
}
