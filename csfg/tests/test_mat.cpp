#include "gtest/gtest.h"

extern "C" {
#include "csfg/numeric/mat.h"
}

#define NAME test_mat

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_mat_init(&M);
        csfg_mat_reorder_init(&reorder);
    }
    void TearDown() override
    {
        csfg_mat_reorder_deinit(reorder);
        csfg_mat_deinit(M);
    }

    struct csfg_mat*         M;
    struct csfg_mat_reorder* reorder;
};

TEST_F(NAME, set_identity_square_matrix)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 5, 5), 0);
    csfg_mat_identity(M);

    ASSERT_EQ(M->rows, 5);
    ASSERT_EQ(M->cols, 5);
    for (int r = 0; r != M->rows; ++r)
        for (int c = 0; c != M->cols; ++c)
        {
            if (r == c)
                ASSERT_EQ(csfg_mat_get(M, r, c)->real, 1.0);
            else
                ASSERT_EQ(csfg_mat_get(M, r, c)->real, 0.0);
            ASSERT_EQ(csfg_mat_get(M, r, c)->imag, 0.0);
        }
}

TEST_F(NAME, set_identity_more_rows)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 8, 5), 0);
    csfg_mat_identity(M);

    ASSERT_EQ(M->rows, 8);
    ASSERT_EQ(M->cols, 5);
    for (int r = 0; r != M->rows; ++r)
        for (int c = 0; c != M->cols; ++c)
        {
            if (r == c)
                ASSERT_EQ(csfg_mat_get(M, r, c)->real, 1.0);
            else
                ASSERT_EQ(csfg_mat_get(M, r, c)->real, 0.0);
            ASSERT_EQ(csfg_mat_get(M, r, c)->imag, 0.0);
        }
}

TEST_F(NAME, set_identity_more_cols)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 5, 8), 0);
    csfg_mat_identity(M);

    ASSERT_EQ(M->rows, 5);
    ASSERT_EQ(M->cols, 8);
    for (int r = 0; r != M->rows; ++r)
        for (int c = 0; c != M->cols; ++c)
        {
            if (r == c)
                ASSERT_EQ(csfg_mat_get(M, r, c)->real, 1.0);
            else
                ASSERT_EQ(csfg_mat_get(M, r, c)->real, 0.0);
            ASSERT_EQ(csfg_mat_get(M, r, c)->imag, 0.0);
        }
}

TEST_F(NAME, rotate_all_rows)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 4, 5), 0);
    ASSERT_EQ(csfg_mat_reorder_identity(&reorder, M), 0);
    /* clang-format off */
    csfg_mat_set_real(M,
         0.0,  1.0,  2.0,  3.0,  4.0,
         5.0,  6.0,  7.0,  8.0,  9.0,
        10.0, 11.0, 12.0, 13.0, 14.0,
        15.0, 16.0, 17.0, 18.0, 19.0);
    /* clang-format on */

    csfg_mat_rotate_rows_in_range(M, reorder, 0, 4);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 15);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 0);

    ASSERT_EQ(*vec_get(reorder, 0), 1);
    ASSERT_EQ(*vec_get(reorder, 1), 2);
    ASSERT_EQ(*vec_get(reorder, 2), 3);
    ASSERT_EQ(*vec_get(reorder, 3), 0);

    csfg_mat_apply_reorder(M, reorder);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 15);
}

TEST_F(NAME, rotate_last_rows)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 4, 5), 0);
    ASSERT_EQ(csfg_mat_reorder_identity(&reorder, M), 0);
    /* clang-format off */
    csfg_mat_set_real(M,
         0.0,  1.0,  2.0,  3.0,  4.0,
         5.0,  6.0,  7.0,  8.0,  9.0,
        10.0, 11.0, 12.0, 13.0, 14.0,
        15.0, 16.0, 17.0, 18.0, 19.0);
    /* clang-format on */

    csfg_mat_rotate_rows_in_range(M, reorder, 2, 4);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 15);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 10);

    ASSERT_EQ(*vec_get(reorder, 0), 0);
    ASSERT_EQ(*vec_get(reorder, 1), 1);
    ASSERT_EQ(*vec_get(reorder, 2), 3);
    ASSERT_EQ(*vec_get(reorder, 3), 2);

    csfg_mat_apply_reorder(M, reorder);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 15);
}

TEST_F(NAME, rotate_first_rows)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 4, 5), 0);
    ASSERT_EQ(csfg_mat_reorder_identity(&reorder, M), 0);
    /* clang-format off */
    csfg_mat_set_real(M,
         0.0,  1.0,  2.0,  3.0,  4.0,
         5.0,  6.0,  7.0,  8.0,  9.0,
        10.0, 11.0, 12.0, 13.0, 14.0,
        15.0, 16.0, 17.0, 18.0, 19.0);
    /* clang-format on */

    csfg_mat_rotate_rows_in_range(M, reorder, 0, 2);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 15);

    ASSERT_EQ(*vec_get(reorder, 0), 1);
    ASSERT_EQ(*vec_get(reorder, 1), 0);
    ASSERT_EQ(*vec_get(reorder, 2), 2);
    ASSERT_EQ(*vec_get(reorder, 3), 3);

    csfg_mat_apply_reorder(M, reorder);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 15);
}

TEST_F(NAME, rotate_no_rows)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 4, 5), 0);
    ASSERT_EQ(csfg_mat_reorder_identity(&reorder, M), 0);
    /* clang-format off */
    csfg_mat_set_real(M,
         0.0,  1.0,  2.0,  3.0,  4.0,
         5.0,  6.0,  7.0,  8.0,  9.0,
        10.0, 11.0, 12.0, 13.0, 14.0,
        15.0, 16.0, 17.0, 18.0, 19.0);
    /* clang-format on */

    csfg_mat_rotate_rows_in_range(M, reorder, 1, 1);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 15);

    ASSERT_EQ(*vec_get(reorder, 0), 0);
    ASSERT_EQ(*vec_get(reorder, 1), 1);
    ASSERT_EQ(*vec_get(reorder, 2), 2);
    ASSERT_EQ(*vec_get(reorder, 3), 3);

    csfg_mat_apply_reorder(M, reorder);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 5);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 15);
}

TEST_F(NAME, rotate_multiple_isolated_rows)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 8, 2), 0);
    ASSERT_EQ(csfg_mat_reorder_identity(&reorder, M), 0);
    /* clang-format off */
    csfg_mat_set_real(M,
         0.0,  1.0,
         2.0,  3.0,
         4.0,  5.0,
         6.0,  7.0,
         8.0,  9.0,
        10.0, 11.0,
        12.0, 13.0,
        14.0, 14.0);
    /* clang-format on */

    csfg_mat_rotate_rows_in_range(M, reorder, 1, 4);
    csfg_mat_rotate_rows_in_range(M, reorder, 5, 8);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 4);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 6);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 2);
    ASSERT_EQ(csfg_mat_get(M, 4, 0)->real, 8);
    ASSERT_EQ(csfg_mat_get(M, 5, 0)->real, 12);
    ASSERT_EQ(csfg_mat_get(M, 6, 0)->real, 14);
    ASSERT_EQ(csfg_mat_get(M, 7, 0)->real, 10);

    ASSERT_EQ(*vec_get(reorder, 0), 0);
    ASSERT_EQ(*vec_get(reorder, 1), 2);
    ASSERT_EQ(*vec_get(reorder, 2), 3);
    ASSERT_EQ(*vec_get(reorder, 3), 1);
    ASSERT_EQ(*vec_get(reorder, 4), 4);
    ASSERT_EQ(*vec_get(reorder, 5), 6);
    ASSERT_EQ(*vec_get(reorder, 6), 7);
    ASSERT_EQ(*vec_get(reorder, 7), 5);

    csfg_mat_apply_reorder(M, reorder);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 2);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 4);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 6);
    ASSERT_EQ(csfg_mat_get(M, 4, 0)->real, 8);
    ASSERT_EQ(csfg_mat_get(M, 5, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 6, 0)->real, 12);
    ASSERT_EQ(csfg_mat_get(M, 7, 0)->real, 14);
}

TEST_F(NAME, swap_multiple_rows)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 8, 2), 0);
    ASSERT_EQ(csfg_mat_reorder_identity(&reorder, M), 0);
    /* clang-format off */
    csfg_mat_set_real(M,
         0.0,  1.0,
         2.0,  3.0,
         4.0,  5.0,
         6.0,  7.0,
         8.0,  9.0,
        10.0, 11.0,
        12.0, 13.0,
        14.0, 14.0);
    /* clang-format on */

    csfg_mat_swap_rows(M, reorder, 1, 3);
    csfg_mat_swap_rows(M, reorder, 5, 6);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 6);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 4);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 2);
    ASSERT_EQ(csfg_mat_get(M, 4, 0)->real, 8);
    ASSERT_EQ(csfg_mat_get(M, 5, 0)->real, 12);
    ASSERT_EQ(csfg_mat_get(M, 6, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 7, 0)->real, 14);

    ASSERT_EQ(*vec_get(reorder, 0), 0);
    ASSERT_EQ(*vec_get(reorder, 1), 3);
    ASSERT_EQ(*vec_get(reorder, 2), 2);
    ASSERT_EQ(*vec_get(reorder, 3), 1);
    ASSERT_EQ(*vec_get(reorder, 4), 4);
    ASSERT_EQ(*vec_get(reorder, 5), 6);
    ASSERT_EQ(*vec_get(reorder, 6), 5);
    ASSERT_EQ(*vec_get(reorder, 7), 7);

    csfg_mat_apply_reorder(M, reorder);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 2);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 4);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 6);
    ASSERT_EQ(csfg_mat_get(M, 4, 0)->real, 8);
    ASSERT_EQ(csfg_mat_get(M, 5, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 6, 0)->real, 12);
    ASSERT_EQ(csfg_mat_get(M, 7, 0)->real, 14);
}

TEST_F(NAME, swap_rows_multiple_times)
{
    ASSERT_EQ(csfg_mat_realloc(&M, 8, 2), 0);
    ASSERT_EQ(csfg_mat_reorder_identity(&reorder, M), 0);
    /* clang-format off */
    csfg_mat_set_real(M,
         0.0,  1.0,
         2.0,  3.0,
         4.0,  5.0,
         6.0,  7.0,
         8.0,  9.0,
        10.0, 11.0,
        12.0, 13.0,
        14.0, 14.0);
    /* clang-format on */

    csfg_mat_swap_rows(M, reorder, 1, 2);
    csfg_mat_swap_rows(M, reorder, 2, 3);
    csfg_mat_swap_rows(M, reorder, 5, 6);
    csfg_mat_swap_rows(M, reorder, 6, 7);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 4);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 6);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 2);
    ASSERT_EQ(csfg_mat_get(M, 4, 0)->real, 8);
    ASSERT_EQ(csfg_mat_get(M, 5, 0)->real, 12);
    ASSERT_EQ(csfg_mat_get(M, 6, 0)->real, 14);
    ASSERT_EQ(csfg_mat_get(M, 7, 0)->real, 10);

    ASSERT_EQ(*vec_get(reorder, 0), 0);
    ASSERT_EQ(*vec_get(reorder, 1), 2);
    ASSERT_EQ(*vec_get(reorder, 2), 3);
    ASSERT_EQ(*vec_get(reorder, 3), 1);
    ASSERT_EQ(*vec_get(reorder, 4), 4);
    ASSERT_EQ(*vec_get(reorder, 5), 6);
    ASSERT_EQ(*vec_get(reorder, 6), 7);
    ASSERT_EQ(*vec_get(reorder, 7), 5);

    csfg_mat_apply_reorder(M, reorder);

    ASSERT_EQ(csfg_mat_get(M, 0, 0)->real, 0);
    ASSERT_EQ(csfg_mat_get(M, 1, 0)->real, 2);
    ASSERT_EQ(csfg_mat_get(M, 2, 0)->real, 4);
    ASSERT_EQ(csfg_mat_get(M, 3, 0)->real, 6);
    ASSERT_EQ(csfg_mat_get(M, 4, 0)->real, 8);
    ASSERT_EQ(csfg_mat_get(M, 5, 0)->real, 10);
    ASSERT_EQ(csfg_mat_get(M, 6, 0)->real, 12);
    ASSERT_EQ(csfg_mat_get(M, 7, 0)->real, 14);
}
