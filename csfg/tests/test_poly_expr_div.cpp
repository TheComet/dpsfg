#include "csfg/tests/PolyHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly_expr.h"
}

#define NAME test_poly_expr_div

using namespace testing;

struct NAME : public Test, public PolyHelper
{
    void SetUp() override
    {
        csfg_poly_expr_init(&out);
        csfg_poly_expr_init(&remainder);
        csfg_poly_expr_init(&p1);
        csfg_poly_expr_init(&p2);
        csfg_expr_pool_init(&pool);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(pool);
        csfg_poly_expr_deinit(p2);
        csfg_poly_expr_deinit(p1);
        csfg_poly_expr_deinit(remainder);
        csfg_poly_expr_deinit(out);
    }

    struct csfg_poly_expr* out;
    struct csfg_poly_expr* remainder;
    struct csfg_poly_expr* p1;
    struct csfg_poly_expr* p2;
    struct csfg_expr_pool* pool;
};

TEST_F(NAME, example1)
{
    // 6x^4 + 2x^3 + 5x^2 + 3x + 2
    csfg_poly_expr_push(&p1, csfg_coeff_expr(2.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(3.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(5.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(2.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(6.0, -1));
    // 2x^2 + 1
    csfg_poly_expr_push(&p2, csfg_coeff_expr(1.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(0.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(2.0, -1));

    //           ____________________________  3x^2 + x + 1
    // 2x^2 + 1 ) 6x^4 + 2x^3 + 5x^2 + 3x + 2
    //           -6x^4        - 3x^2
    //               r = 2x^3 + 2x^2 + 3x + 2
    //                  -2x^3        - x
    //                      r = 2x^2 + 2x + 2
    //                         -2x^2      - 1
    //                             r = 2x + 1
    ASSERT_EQ(csfg_poly_expr_div(&pool, &out, &remainder, p1, p2), 0);

    // 3x^2 + x + 1
    ASSERT_EQ(csfg_poly_expr_degree(out), 2);
    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0));
    ASSERT_TRUE(CoeffEq(pool, out, 1, 1.0));
    ASSERT_TRUE(CoeffEq(pool, out, 2, 3.0));
    // 2x + 1
    ASSERT_EQ(csfg_poly_expr_degree(remainder), 1);
    ASSERT_TRUE(CoeffEq(pool, remainder, 0, 1.0));
    ASSERT_TRUE(CoeffEq(pool, remainder, 1, 2.0));
}

TEST_F(NAME, example2)
{
    // 6x^4 + 5x^2 + 3x + 2
    csfg_poly_expr_push(&p1, csfg_coeff_expr(2.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(3.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(5.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(0.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(6.0, -1));
    // 2x^2 + 1
    csfg_poly_expr_push(&p2, csfg_coeff_expr(1.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(0.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(2.0, -1));

    //           ____________________________  3x^2 + 1
    // 2x^2 + 1 ) 6x^4 + 5x^2 + 3x + 2
    //           -6x^4 - 3x^2
    //               r = 2x^2 + 3x + 2
    //                  -2x^3      - 1
    //                      r = 3x + 1
    ASSERT_EQ(csfg_poly_expr_div(&pool, &out, &remainder, p1, p2), 0);

    // 3x^2 + 1
    ASSERT_EQ(csfg_poly_expr_degree(out), 2);
    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0));
    ASSERT_TRUE(CoeffEq(pool, out, 1, 0.0));
    ASSERT_TRUE(CoeffEq(pool, out, 2, 3.0));
    // 3x + 1
    ASSERT_EQ(csfg_poly_expr_degree(remainder), 1);
    ASSERT_TRUE(CoeffEq(pool, remainder, 0, 1.0));
    ASSERT_TRUE(CoeffEq(pool, remainder, 1, 3.0));
}

TEST_F(NAME, div_1_1)
{
    csfg_poly_expr_push(&p1, csfg_coeff_expr(1.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(1.0, -1));

    ASSERT_EQ(csfg_poly_expr_div(&pool, &out, &remainder, p1, p2), 0);

    ASSERT_EQ(csfg_poly_expr_degree(out), 0);
    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0));
    ASSERT_EQ(vec_count(remainder), 0);
}
