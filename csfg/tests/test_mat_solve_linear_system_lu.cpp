#include "gtest/gtest.h"

extern "C" {
#include "csfg/numeric/mat.h"
}

#define NAME test_mat_solve_linear_system_lu

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_mat_init(&in);
        csfg_mat_init(&out);
        csfg_mat_init(&L);
        csfg_mat_init(&U);
    }
    void TearDown() override
    {
        csfg_mat_deinit(U);
        csfg_mat_deinit(L);
        csfg_mat_deinit(out);
        csfg_mat_deinit(in);
    }

    struct csfg_mat* in;
    struct csfg_mat* out;
    struct csfg_mat* L;
    struct csfg_mat* U;
};

TEST_F(NAME, example_paper)
{
    /*
     * 3x + 2y - z = 1
     * 2x - 2y + 4z = -2
     * -x + 1/2y - z = 0
     */
    ASSERT_EQ(csfg_mat_realloc(&in, 3, 3), 0);
    /* clang-format off */
    csfg_mat_set_real(in,
         3.0,  2.0, -1.0,
         2.0, -2.0,  4.0,
        -1.0,  0.5, -1.0);
    /* clang-format on */
    ASSERT_EQ(csfg_mat_lu_decomposition(&L, &U, in), 0);

    ASSERT_EQ(csfg_mat_realloc(&out, 3, 1), 0);
    ASSERT_EQ(csfg_mat_realloc(&in, 3, 1), 0);
    /* clang-format off */
    csfg_mat_set_real(in,
         1.0,
        -2.0,
         0.0);
    /* clang-format on */

    csfg_mat_solve_linear_lu(out, in, L, U);

    ASSERT_DOUBLE_EQ(csfg_mat_get(out, 0, 0)->real, 1);
    ASSERT_DOUBLE_EQ(csfg_mat_get(out, 1, 0)->real, -2);
    ASSERT_DOUBLE_EQ(csfg_mat_get(out, 2, 0)->real, -2);
}
