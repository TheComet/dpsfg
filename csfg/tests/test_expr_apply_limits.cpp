#include "csfg/tests/PolyHelper.hpp"

#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_apply_limits

using namespace testing;

struct NAME : public Test, public PolyHelper
{
    void SetUp() override
    {
        csfg_expr_pool_init(&in_pool);
        csfg_expr_pool_init(&out_pool);
        csfg_expr_pool_init(&tf_pool);
        csfg_tf_expr_init(&tf);
        csfg_var_table_init(&vt);
    }

    void TearDown() override
    {
        csfg_var_table_deinit(&vt);
        csfg_tf_expr_deinit(&tf);
        csfg_expr_pool_deinit(tf_pool);
        csfg_expr_pool_deinit(out_pool);
        csfg_expr_pool_deinit(in_pool);
    }

    struct csfg_expr_pool* in_pool;
    struct csfg_expr_pool* out_pool;
    struct csfg_expr_pool* tf_pool;
    struct csfg_tf_expr    tf;
    struct csfg_var_table  vt;
};

TEST_F(NAME, converge)
{
    /*
     * a*y + b*x*y |        b*y |         b
     * ----------- |      = --- |      = ---
     * b*y + d*x*y |x->oo   d*y |y->oo    d
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(a*y + b*x*y)/(b*y + d*x*y)"));
    ASSERT_THAT(expr, Ge(0));

    csfg_var_table_set_parse_expr(&vt, cstr_view("x"), cstr_view("oo"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("y"), cstr_view("oo"));

    expr = csfg_expr_apply_limits(in_pool, expr, &vt, &out_pool);
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational(out_pool, expr, cstr_view("s"), &tf_pool, &tf),
        Eq(0));

    ASSERT_THAT(vec_count(tf.num), Eq(1));
    ASSERT_THAT(vec_count(tf.den), Eq(1));
    ASSERT_TRUE(CoeffEq(tf_pool, tf.num, 0, 1.0, "b"));
    ASSERT_TRUE(CoeffEq(tf_pool, tf.den, 0, 1.0, "d"));
}
