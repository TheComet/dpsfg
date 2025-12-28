#include "gtest/gtest.h"

extern "C" {
#include "csfg/numeric/mat.h"
}

#define NAME test_mat

using namespace testing;

struct NAME : public Test
{
    void SetUp() override { csfg_mat_init(&M); }
    void TearDown() override { csfg_mat_deinit(M); }

    struct csfg_mat* M;
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
