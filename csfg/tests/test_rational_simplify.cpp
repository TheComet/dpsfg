#include "csfg/tests/PolyHelper.hpp"

#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/rational.h"
}

#define NAME test_rational_simplify

using namespace testing;

struct NAME : public Test, public PolyHelper
{
    void SetUp() override
    {
        csfg_expr_pool_init(&pool1);
        csfg_expr_pool_init(&pool2);
        csfg_rational_init(&rational);
    }
    void TearDown() override
    {
        csfg_rational_deinit(&rational);
        csfg_expr_pool_deinit(pool2);
        csfg_expr_pool_deinit(pool1);
    }

    struct csfg_expr_pool* pool1;
    struct csfg_expr_pool* pool2;
    struct csfg_rational   rational;
};

TEST_F(NAME, case1)
{
    /*
     *   -G1*(G2+s*C)
     * -----------------
     * (G2+s*C)*(G2+s*C)
     */
    int expr = csfg_expr_parse(
        &pool1, cstr_view("-1*(G1*(G2+s*C))/(1*((G2+s*C)*(s*C+G2)))"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);

    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&pool1), Eq(1));

    /*
     *         -G2*G1 - (G1*C)*s
     * ---------------------------------
     * G2*G2 + 2*(G2*C*G2*C)*s + C*C*s^2
     */
    ASSERT_THAT(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(2));
    ASSERT_TRUE(CoeffEq(pool2, rational.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(pool2, rational.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(pool2, rational.den, 1, 1.0, "C"));
}
