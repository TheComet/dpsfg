#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
}

#define NAME test_expr_op_rebalance_fraction

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&n1);
        csfg_expr_pool_init(&n2);
        csfg_expr_pool_init(&d1);
        csfg_expr_pool_init(&d2);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(d2);
        csfg_expr_pool_deinit(d1);
        csfg_expr_pool_deinit(n2);
        csfg_expr_pool_deinit(n1);
    }

    struct csfg_expr_pool* n1;
    struct csfg_expr_pool* n2;
    struct csfg_expr_pool* d1;
    struct csfg_expr_pool* d2;
};

TEST_F(NAME, distribute_single)
{
    int nr1 = csfg_expr_parse(&n1, "s^-1");
    int nr2 = csfg_expr_parse(&n2, "1");
    int dr1 = csfg_expr_parse(&d1, "1");
    int dr2 = csfg_expr_parse(&d2, "s^1*1");
    ASSERT_THAT(nr1, Ge(0));
    ASSERT_THAT(nr2, Ge(0));
    ASSERT_THAT(dr1, Ge(0));
    ASSERT_THAT(dr2, Ge(0));
    csfg_expr_op_run_until_complete(&n1, csfg_expr_opt_fold_constants, NULL);
    ASSERT_THAT(csfg_expr_op_rebalance_fraction(&n1, nr1, &d1, dr1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(n1, nr1, n2, nr2), IsTrue());
    ASSERT_THAT(csfg_expr_equal(d1, dr1, d2, dr2), IsTrue());
}

TEST_F(NAME, distribute_multiple_products)
{
    int nr1 = csfg_expr_parse(&n1, "a*s^-1*c");
    int nr2 = csfg_expr_parse(&n2, "a*1*c");
    int dr1 = csfg_expr_parse(&d1, "1");
    int dr2 = csfg_expr_parse(&d2, "s^1*1");
    ASSERT_THAT(nr1, Ge(0));
    ASSERT_THAT(nr2, Ge(0));
    ASSERT_THAT(dr1, Ge(0));
    ASSERT_THAT(dr2, Ge(0));
    csfg_expr_op_run_until_complete(&n1, csfg_expr_opt_fold_constants, NULL);
    ASSERT_THAT(csfg_expr_op_rebalance_fraction(&n1, nr1, &d1, dr1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(n1, nr1, n2, nr2), IsTrue());
    ASSERT_THAT(csfg_expr_equal(d1, dr1, d2, dr2), IsTrue());
}

TEST_F(NAME, dont_distribute_if_top_is_not_mul)
{
    int nr1 = csfg_expr_parse(&n1, "d+a*s^-1*c");
    int nr2 = csfg_expr_parse(&n2, "d+a*s^-1*c");
    int dr1 = csfg_expr_parse(&d1, "1");
    int dr2 = csfg_expr_parse(&d2, "1");
    ASSERT_THAT(nr1, Ge(0));
    ASSERT_THAT(nr2, Ge(0));
    ASSERT_THAT(dr1, Ge(0));
    ASSERT_THAT(dr2, Ge(0));
    csfg_expr_op_run_until_complete(&n1, csfg_expr_opt_fold_constants, NULL);
    csfg_expr_op_run_until_complete(&n2, csfg_expr_opt_fold_constants, NULL);
    ASSERT_THAT(csfg_expr_op_rebalance_fraction(&n1, nr1, &d1, dr1), Eq(0));
    ASSERT_THAT(csfg_expr_equal(n1, nr1, n2, nr2), IsTrue());
    ASSERT_THAT(csfg_expr_equal(d1, dr1, d2, dr2), IsTrue());
}

TEST_F(NAME, distribute_num_and_den)
{
    int nr1 = csfg_expr_parse(&n1, "a*s^-3*c*x^-2");
    int nr2 = csfg_expr_parse(&n2, "g^5*(s^4*(a*1*c*1))");
    int dr1 = csfg_expr_parse(&d1, "e*s^-4*f*g^-5");
    int dr2 = csfg_expr_parse(&d2, "x^2*(s^3*(e*1*f*1))");
    ASSERT_THAT(nr1, Ge(0));
    ASSERT_THAT(nr2, Ge(0));
    ASSERT_THAT(dr1, Ge(0));
    ASSERT_THAT(dr2, Ge(0));
    csfg_expr_op_run_until_complete(&n1, csfg_expr_opt_fold_constants, NULL);
    csfg_expr_op_run_until_complete(&d1, csfg_expr_opt_fold_constants, NULL);
    ASSERT_THAT(csfg_expr_op_rebalance_fraction(&n1, nr1, &d1, dr1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(n1, nr1, n2, nr2), IsTrue());
    ASSERT_THAT(csfg_expr_equal(d1, dr1, d2, dr2), IsTrue());
}
