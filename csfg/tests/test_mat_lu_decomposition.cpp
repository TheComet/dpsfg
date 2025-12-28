#include "gmock/gmock.h"

extern "C" {
#include "csfg/numeric/mat.h"
}

#define NAME test_mat_lu_decomposition

using namespace testing;

namespace {
MATCHER_P2(
    ComplexEq,
    expected_real,
    expected_imag,
    "matches a csfg_complex approximately")
{
    return ExplainMatchResult(
        AllOf(
            Field(&csfg_complex::real, DoubleEq(expected_real)),
            Field(&csfg_complex::imag, DoubleEq(expected_imag))),
        arg,
        result_listener);
}

void AssertMatricesAreEqual(const csfg_mat* a, const csfg_mat* b)
{
    ASSERT_EQ(csfg_mat_rows(a), csfg_mat_rows(b));
    ASSERT_EQ(csfg_mat_cols(a), csfg_mat_cols(b));
    for (int r = 0; r != csfg_mat_rows(a); ++r)
        for (int c = 0; c != csfg_mat_cols(a); ++c)
        {
            ASSERT_DOUBLE_EQ(
                csfg_mat_get(a, r, c)->real, csfg_mat_get(b, r, c)->real);
            ASSERT_DOUBLE_EQ(
                csfg_mat_get(a, r, c)->imag, csfg_mat_get(b, r, c)->imag);
        }
}

} // namespace

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_mat_init(&M);
        csfg_mat_init(&N);
        csfg_mat_init(&L);
        csfg_mat_init(&U);
        csfg_mat_reorder_init(&reorder);
    }
    void TearDown() override
    {
        csfg_mat_reorder_deinit(reorder);
        csfg_mat_deinit(U);
        csfg_mat_deinit(L);
        csfg_mat_deinit(N);
        csfg_mat_deinit(M);
    }

    struct csfg_mat*         M;
    struct csfg_mat*         N;
    struct csfg_mat*         L;
    struct csfg_mat*         U;
    struct csfg_mat_reorder* reorder;
};

TEST_F(NAME, example_paper)
{
    csfg_mat_realloc(&M, 3, 3);
    /* clang-format off */
    csfg_mat_set_real(M,
        6.0, 18.0, 3.0,
        2.0, 12.0, 1.0,
        4.0, 15.0, 3.0);
    /* clang-format on */

    ASSERT_EQ(csfg_mat_lu_decomposition(&L, &U, &reorder, M), 0);

    csfg_mat_mul(&N, L, U);
    AssertMatricesAreEqual(N, M);

    ASSERT_THAT(*csfg_mat_get(L, 0, 0), ComplexEq(1, 0));
    ASSERT_THAT(*csfg_mat_get(L, 0, 1), ComplexEq(0, 0));
    ASSERT_THAT(*csfg_mat_get(L, 0, 2), ComplexEq(0, 0));
    ASSERT_THAT(*csfg_mat_get(L, 1, 0), ComplexEq(1 / 3.0, 0));
    ASSERT_THAT(*csfg_mat_get(L, 1, 1), ComplexEq(1, 0));
    ASSERT_THAT(*csfg_mat_get(L, 1, 2), ComplexEq(0, 0));
    ASSERT_THAT(*csfg_mat_get(L, 2, 0), ComplexEq(2 / 3.0, 0));
    ASSERT_THAT(*csfg_mat_get(L, 2, 1), ComplexEq(0.5, 0));
    ASSERT_THAT(*csfg_mat_get(L, 2, 2), ComplexEq(1, 0));
}

TEST_F(NAME, example4)
{
    csfg_mat_realloc(&M, 5, 5);
    /* clang-format off */
    csfg_mat_set_real(M,
        2.0,  1.0, -2.0,  2.0, -2.0,
        1.0, -4.0,  1.0, -3.0,  5.0,
        0.0,  6.0,  1.0,  0.0, -3.0,
        0.0, -4.0,  0.0,  1.0, -1.0,
        0.0,  1.0,  0.0,  0.0,  1.0);
    /* clang-format on */

    ASSERT_EQ(csfg_mat_lu_decomposition(&L, &U, &reorder, M), 0);

    csfg_mat_mul(&N, L, U);
    AssertMatricesAreEqual(N, M);
}

TEST_F(NAME, lp2_repeated_roots)
{
    /*
     *       k*wp^2                  k=1
     * --------------------   with   wp=1
     * s^2 + s*wp/qp + wp^2          qp=0.5
     *
     *        1           A1        A2      A1 + A2*s + A2
     * -------------- = ------- + ----- = ----------------
     *  s^2 + 2*s + 1   (s+1)^2   (s+1)       (s+1)^2
     *
     *  s^1: 0 = A2
     *  s^0: 1 = A1 + A2
     */
    csfg_mat_realloc(&M, 2, 2);
    /* clang-format off */
    csfg_mat_set_real(M,
        1.0, 0.0,
        1.0, 1.0);
    /* clang-format on */

    ASSERT_EQ(csfg_mat_lu_decomposition(&L, &U, &reorder, M), 0);

    csfg_mat_mul(&N, L, U);
    AssertMatricesAreEqual(N, M);
}

TEST_F(NAME, ramp_response)
{
    csfg_mat_realloc(&M, 4, 4);
    /* clang-format off */
    csfg_mat_set_real(M,
         1.0,   0.0,    0.0,    0.0,
        -1.154, 0.0,    0.0,    1.0,
         1.0,  -0.577, -0.577, -1.154,
         0.0,   1.0,    1.0,    1.0);
    csfg_mat_get(M, 2, 1)->imag =  0.817;
    csfg_mat_get(M, 2, 2)->imag = -0.817;
    /* clang-format on */

    ASSERT_EQ(csfg_mat_lu_decomposition(&L, &U, &reorder, M), 0);

    csfg_mat_mul(&N, L, U);
    csfg_mat_apply_reorder(N, reorder);
    AssertMatricesAreEqual(N, M);
}
