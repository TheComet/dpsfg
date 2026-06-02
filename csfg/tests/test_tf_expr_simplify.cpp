#include "csfg/tests/PolyHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/tf_expr.h"
}

#define NAME test_tf_expr_simplify

using namespace testing;

struct NAME : public Test, public PolyHelper
{
    void SetUp() override
    {
        csfg_expr_pool_init(&pool1);
        csfg_expr_pool_init(&pool2);
        csfg_tf_expr_init(&tf);
    }
    void TearDown() override
    {
        csfg_tf_expr_deinit(&tf);
        csfg_expr_pool_deinit(pool2);
        csfg_expr_pool_deinit(pool1);
    }

    struct csfg_expr_pool* pool1;
    struct csfg_expr_pool* pool2;
    struct csfg_tf_expr tf;
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
    ASSERT_GE(expr, 0);
    csfg_rule_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);

    ASSERT_EQ(csfg_rule_remove_useless_ops(&pool1), 1);

    /*
     *         -G2*G1 - (G1*C)*s
     * ---------------------------------
     * G2*G2 + 2*(G2*C*G2*C)*s + C*C*s^2
     */
    ASSERT_EQ(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &tf), 0);

    ASSERT_EQ(vec_count(tf.num), 1);
    ASSERT_EQ(vec_count(tf.den), 2);
    ASSERT_TRUE(CoeffEq(pool2, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 1, 1.0, "C"));
}

TEST_F(NAME, case2)
{
    /*
     *           -G1*(C*s + G1 + G2)
     * ----------------------------------------
     * C*s*(C*s + G1 + G2) + G2*(C*s + G1 + G2)
     */
    int expr = csfg_expr_parse(
        &pool1,
        cstr_view("-G1*(C*s+G1+G2) / (C*s*(C*s+G1+G2) + G2*(C*s+G1+G2))"));
    ASSERT_GE(expr, 0);
    csfg_rule_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);
    csfg_rule_remove_useless_ops(&pool1);

    ASSERT_EQ(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &tf), 0);

    ASSERT_EQ(vec_count(tf.num), 1);
    ASSERT_EQ(vec_count(tf.den), 2);
    ASSERT_TRUE(CoeffEq(pool2, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 1, 1.0, "C"));
}

TEST_F(NAME, case2_gcd_method)
{
    /*
     *           -G1*(C*s + G1 + G2)
     * ----------------------------------------
     * C*s*(C*s + G1 + G2) + G2*(C*s + G1 + G2)
     */
    int expr = csfg_expr_parse(
        &pool1,
        cstr_view("-G1*(C*s+G1+G2) / (C*s*(C*s+G1+G2) + G2*(C*s+G1+G2))"));
    ASSERT_GE(expr, 0);
    csfg_rule_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);
    csfg_rule_remove_useless_ops(&pool1);

    ASSERT_EQ(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &tf), 0);

    struct csfg_poly_expr* gcd;
    csfg_poly_expr_init(&gcd);
    ASSERT_EQ(csfg_poly_expr_gcd(&pool2, &gcd, tf.num, tf.den), 0);

    struct csfg_tf_expr tf2;
    csfg_tf_expr_init(&tf2);
    csfg_poly_expr_div(&pool2, &tf2.num, &tf.num, tf.num, gcd);
    csfg_poly_expr_div(&pool2, &tf2.den, &tf.den, tf.den, gcd);

    csfg_tf_expr_clear(&tf);
    csfg_expr_pool_clear(pool1);
    expr = csfg_rational_to_expr(&tf2, pool2, &pool1);
    ASSERT_EQ(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &tf), 0);

    ASSERT_EQ(csfg_poly_expr_degree(tf.num), 0);
    ASSERT_EQ(csfg_poly_expr_degree(tf.den), 1);
    ASSERT_TRUE(CoeffEq(pool2, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 1, 1.0, "C"));
}

TEST_F(NAME, gcd_test)
{
    /*
     * a*b*c
     * -----
     * b*c*d
     */
    int expr = csfg_expr_parse(
        &pool1,
        cstr_view("(a*b*c) / (b*c*d)"));
    ASSERT_GE(expr, 0);

    ASSERT_EQ(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &tf), 0);

    struct csfg_poly_expr* gcd;
    csfg_poly_expr_init(&gcd);
    ASSERT_EQ(csfg_poly_expr_gcd(&pool2, &gcd, tf.num, tf.den), 0);

    struct csfg_tf_expr tf2;
    csfg_tf_expr_init(&tf2);
    csfg_poly_expr_div(&pool2, &tf2.num, &tf.num, tf.num, gcd);
    csfg_poly_expr_div(&pool2, &tf2.den, &tf.den, tf.den, gcd);

    csfg_tf_expr_clear(&tf);
    csfg_expr_pool_clear(pool1);
    expr = csfg_rational_to_expr(&tf2, pool2, &pool1);
    ASSERT_EQ(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &tf), 0);

    ASSERT_EQ(csfg_poly_expr_degree(tf.num), 0);
    ASSERT_EQ(csfg_poly_expr_degree(tf.den), 1);
    ASSERT_TRUE(CoeffEq(pool2, tf.num, 0, -1.0, "G1"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 0, 1.0, "G2"));
    ASSERT_TRUE(CoeffEq(pool2, tf.den, 1, 1.0, "C"));
}
