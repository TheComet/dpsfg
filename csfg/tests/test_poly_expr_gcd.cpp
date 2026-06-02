#include "csfg/tests/PolyHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly_expr.h"
}

#define NAME test_poly_expr_gcd

using namespace testing;

struct NAME : public Test, public PolyHelper
{
    void SetUp() override
    {
        csfg_poly_expr_init(&gcd);
        csfg_poly_expr_init(&p1);
        csfg_poly_expr_init(&p2);
        csfg_poly_expr_init(&p1div);
        csfg_poly_expr_init(&p2div);
        csfg_expr_pool_init(&pool);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(pool);
        csfg_poly_expr_deinit(p2div);
        csfg_poly_expr_deinit(p1div);
        csfg_poly_expr_deinit(p2);
        csfg_poly_expr_deinit(p1);
        csfg_poly_expr_deinit(gcd);
    }

    struct csfg_poly_expr* gcd;
    struct csfg_poly_expr* p1;
    struct csfg_poly_expr* p2;
    struct csfg_poly_expr* p1div;
    struct csfg_poly_expr* p2div;
    struct csfg_expr_pool* pool;
};

TEST_F(NAME, example1)
{
    // x^4 - 7x^3 + 17x^2 - 17x + 6
    csfg_poly_expr_push(&p1, csfg_coeff_expr(6.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(-17.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(17.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(-7.0, -1));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(1.0, -1));
    // x^3 - 7x^2 + 14x - 8
    csfg_poly_expr_push(&p2, csfg_coeff_expr(-8.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(14.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(-7.0, -1));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(1.0, -1));

    ASSERT_EQ(csfg_poly_expr_gcd(&pool, &gcd, p1, p2), 0);

    // 3x^2 - 9x + 6
    ASSERT_EQ(csfg_poly_expr_degree(gcd), 2);
    ASSERT_TRUE(CoeffEq(pool, gcd, 0, 6.0));
    ASSERT_TRUE(CoeffEq(pool, gcd, 1, -9.0));
    ASSERT_TRUE(CoeffEq(pool, gcd, 2, 3.0));

    csfg_poly_expr_div(&pool, &p1div, &p1, p1, gcd);
    csfg_poly_expr_div(&pool, &p2div, &p2, p2, gcd);
    // 1/3x^2 - 4/3x + 1
    ASSERT_EQ(csfg_poly_expr_degree(p1div), 2);
    ASSERT_TRUE(CoeffEq(pool, p1div, 0, 1.0));
    ASSERT_TRUE(CoeffEq(pool, p1div, 1, -4.0 / 3.0));
    ASSERT_TRUE(CoeffEq(pool, p1div, 2, 1.0 / 3.0));
    // 1/3x - 4/3
    ASSERT_EQ(csfg_poly_expr_degree(p2div), 1);
    ASSERT_TRUE(CoeffEq(pool, p2div, 0, -4.0 / 3.0));
    ASSERT_TRUE(CoeffEq(pool, p2div, 1, 1.0 / 3.0));
}
