#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_apply_limits

using namespace testing;

struct NAME : public Test, public ExprHelper
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p);
        csfg_tf_expr_init(&tf);
        csfg_var_table_init(&vt);
    }

    void TearDown() override
    {
        csfg_var_table_deinit(&vt);
        csfg_tf_expr_deinit(&tf);
        csfg_expr_pool_deinit(p);
    }

    struct csfg_expr_pool* p;
    struct csfg_tf_expr tf;
    struct csfg_var_table vt;
};

TEST_F(NAME, multiple_limits)
{
    /*
     * a*y + b*x*y |        b*y |         b
     * ----------- |      = --- |      = ---
     * b*y + d*x*y |x->oo   d*y |y->oo    d
     */
    int expr = csfg_expr_parse(&p, cstr_view("(a*y + b*x*y)/(b*y + d*x*y)"));
    ASSERT_GE(expr, 0);

    csfg_var_table_set_parse_expr(&vt, cstr_view("x"), cstr_view("oo"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("y"), cstr_view("oo"));

    expr = csfg_expr_apply_limits(&p, expr, &vt);
    ASSERT_GE(expr, 0);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(vec_count(tf.num), 1);
    ASSERT_EQ(vec_count(tf.den), 1);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, 1.0, "b"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 1.0, "d"));
}
