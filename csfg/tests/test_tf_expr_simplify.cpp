#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/tf_expr.h"
}

#define NAME test_tf_expr_simplify

using namespace testing;

struct NAME : public Test, public ExprHelper
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p);
        csfg_tf_expr_init(&tf);
        csfg_tf_expr_init(&tf2);
        csfg_poly_expr_init(&gcd);
    }
    void TearDown() override
    {
        csfg_poly_expr_deinit(gcd);
        csfg_tf_expr_deinit(&tf2);
        csfg_tf_expr_deinit(&tf);
        csfg_expr_pool_deinit(p);
    }

    struct csfg_expr_pool* p;
    struct csfg_tf_expr tf;
    struct csfg_tf_expr tf2;
    struct csfg_poly_expr* gcd;
};

TEST_F(NAME, case1)
{
    /*
     *   -G1*(G2+s*C)
     * -----------------
     * (G2+s*C)*(G2+s*C)
     */
    int expr = csfg_expr_parse(
        &p, cstr_view("-1*(G1*(G2+s*C))/(1*((G2+s*C)*(s*C+G2)))"));
    ASSERT_GE(expr, 0);
    csfg_rule_fold_constants(&p);
    expr = csfg_expr_gc(p, expr);

    ASSERT_EQ(csfg_rule_remove_useless_ops(&p), 1);

    /*
     *         -G2*G1 - (G1*C)*s
     * ---------------------------------
     * G2*G2 + 2*(G2*C*G2*C)*s + C*C*s^2
     */
    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(vec_count(tf.num), 1);
    ASSERT_EQ(vec_count(tf.den), 2);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 1, 1.0, "C"));
}

TEST_F(NAME, case2)
{
    /*
     *           -G1*(C*s + G1 + G2)
     * ----------------------------------------
     * C*s*(C*s + G1 + G2) + G2*(C*s + G1 + G2)
     */
    int expr = csfg_expr_parse(
        &p, cstr_view("-G1*(C*s+G1+G2) / (C*s*(C*s+G1+G2) + G2*(C*s+G1+G2))"));
    ASSERT_GE(expr, 0);
    csfg_rule_fold_constants(&p);
    expr = csfg_expr_gc(p, expr);
    csfg_rule_remove_useless_ops(&p);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(vec_count(tf.num), 1);
    ASSERT_EQ(vec_count(tf.den), 2);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 1, 1.0, "C"));
}

TEST_F(NAME, case2_gcd_method)
{
    /*
     *           -G1*(C*s + G1 + G2)
     * ----------------------------------------
     * C*s*(C*s + G1 + G2) + G2*(C*s + G1 + G2)
     */
    int expr = csfg_expr_parse(
        &p, cstr_view("-G1*(C*s+G1+G2) / (C*s*(C*s+G1+G2) + G2*(C*s+G1+G2))"));
    ASSERT_GE(expr, 0);
    csfg_rule_fold_constants(&p);
    expr = csfg_expr_gc(p, expr);
    csfg_rule_remove_useless_ops(&p);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(csfg_poly_expr_gcd(&p, &gcd, tf.num, tf.den), 0);

    csfg_poly_expr_div(&p, &tf2.num, &tf.num, tf.num, gcd);
    csfg_poly_expr_div(&p, &tf2.den, &tf.den, tf.den, gcd);

    csfg_tf_expr_clear(&tf);
    csfg_expr_pool_clear(p);
    expr = csfg_rational_to_expr(&tf2, &p, "s");
    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(csfg_poly_expr_degree(tf.num), 0);
    ASSERT_EQ(csfg_poly_expr_degree(tf.den), 1);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 1, 1.0, "C"));
}

TEST_F(NAME, gcd_test)
{
    /*
     * a*b*c
     * -----
     * b*c*d
     */
    int expr = csfg_expr_parse(&p, cstr_view("(a*b*c) / (b*c*d)"));
    ASSERT_GE(expr, 0);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(csfg_poly_expr_gcd(&p, &gcd, tf.num, tf.den), 0);

    csfg_poly_expr_div(&p, &tf2.num, &tf.num, tf.num, gcd);
    csfg_poly_expr_div(&p, &tf2.den, &tf.den, tf.den, gcd);

    csfg_tf_expr_clear(&tf);
    csfg_expr_pool_clear(p);
    expr = csfg_rational_to_expr(&tf2, &p, "s");
    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(csfg_poly_expr_degree(tf.num), 0);
    ASSERT_EQ(csfg_poly_expr_degree(tf.den), 1);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 1, 1.0, "C"));
}
