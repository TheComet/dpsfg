#include <gtest/gtest.h>

extern "C" {
#include "csfg/util/bm.h"
}

#define NAME test_bm

using namespace ::testing;

struct NAME : Test
{
    struct bm* bm = nullptr;
};

TEST_F(NAME, count_is_correct)
{
    bm = bm_create(1);
    ASSERT_EQ(bm->count, 1);
    bm_deinit(bm);

    bm = bm_create(64);
    ASSERT_EQ(bm->count, 1);
    bm_deinit(bm);

    bm = bm_create(64);
    ASSERT_EQ(bm->count, 1);
    bm_deinit(bm);

    bm = bm_create(65);
    ASSERT_EQ(bm->count, 2);
    bm_deinit(bm);

    bm = bm_create(128);
    ASSERT_EQ(bm->count, 2);
    bm_deinit(bm);

    bm = bm_create(192);
    ASSERT_EQ(bm->count, 3);
    bm_deinit(bm);

    bm = bm_create(193);
    ASSERT_EQ(bm->count, 4);
    bm_deinit(bm);
}

TEST_F(NAME, set_and_reset)
{
    bm = bm_create(128);
    ASSERT_EQ(bm_set_and_test(bm, 0), 0);
    ASSERT_EQ(bm_set_and_test(bm, 0), 1);
    ASSERT_EQ(bm_set_and_test(bm, 0), 1);
    ASSERT_EQ(bm_set_and_test(bm, 63), 0);
    ASSERT_EQ(bm_set_and_test(bm, 63), 1);
    ASSERT_EQ(bm_set_and_test(bm, 63), 1);
    ASSERT_EQ(bm_set_and_test(bm, 64), 0);
    ASSERT_EQ(bm_set_and_test(bm, 64), 1);
    ASSERT_EQ(bm_set_and_test(bm, 64), 1);
    ASSERT_EQ(bm->data[0], 0x8000000000000001ULL);
    ASSERT_EQ(bm->data[1], 0x0000000000000001ULL);
    bm_reset_all(bm);
    ASSERT_EQ(bm->data[0], 0ULL);
    ASSERT_EQ(bm->data[1], 0ULL);
    ASSERT_EQ(bm_set_and_test(bm, 0), 0);
    ASSERT_EQ(bm_set_and_test(bm, 0), 1);
    ASSERT_EQ(bm_set_and_test(bm, 0), 1);
    ASSERT_EQ(bm_set_and_test(bm, 63), 0);
    ASSERT_EQ(bm_set_and_test(bm, 63), 1);
    ASSERT_EQ(bm_set_and_test(bm, 63), 1);
    ASSERT_EQ(bm_set_and_test(bm, 64), 0);
    ASSERT_EQ(bm_set_and_test(bm, 64), 1);
    ASSERT_EQ(bm_set_and_test(bm, 64), 1);
    bm_deinit(bm);
}
