#include "csfg/tests/PolyHelper.hpp"

#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/expr_tf.h"
#include "csfg/symbolic/rational.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_to_rational_limit

using namespace testing;

struct NAME : public Test, public PolyHelper
{
    void SetUp() override
    {
        csfg_expr_pool_init(&in_pool);
        csfg_expr_pool_init(&out_pool);
        csfg_rational_init(&rational);
        csfg_var_table_init(&vt);
    }

    void TearDown() override
    {
        csfg_var_table_deinit(&vt);
        csfg_rational_deinit(&rational);
        csfg_expr_pool_deinit(out_pool);
        csfg_expr_pool_deinit(in_pool);
    }

    struct csfg_expr_pool* in_pool;
    struct csfg_expr_pool* out_pool;
    struct csfg_rational   rational;
    struct csfg_var_table  vt;
};

TEST_F(NAME, zero_divided_by_zero)
{
    /*
     * 0*x^2 + 0*x |         0
     * ----------- |      = ---
     * 0*x^2 + 0*x |x->oo    0
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(0*x^2 + 0*x)/(0*x^2 + 0*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 0.0));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 0.0));
}

TEST_F(NAME, zero_numerator)
{
    /*
     * 0*x^2 + 0*x |
     * ----------- |      = 0
     * c*x^2 + d*x |x->oo
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(0*x^2 + 0*x)/(c*x^2 + d*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 0.0));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0));
}

TEST_F(NAME, positive_zero_denominator)
{
    /*
     * a*x^2 + b*x |        oo
     * ----------- |      = --
     * 0*x^2 + 0*x |x->oo   0
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(a*x^2 + b*x)/(0*x^2 + 0*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 1.0, CSFG_EXPR_INF));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 0.0));
}

TEST_F(NAME, negative_zero_denominator)
{
    /*
     * -a*x^2 + b*x |        -oo
     * ------------ |      = ---
     *  0*x^2 + 0*x |x->oo    0
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(-a*x^2 + b*x)/(0*x^2 + 0*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, -1.0, CSFG_EXPR_INF));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 0.0));
}

TEST_F(NAME, numerator_diverges_positive_inf)
{
    /*
     * a*x^2 + b*x |
     * ----------- |      = oo
     * 0*x^2 + d*x |x->oo
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(a*x^2 + b*x)/(0*x^2 + d*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 1.0, CSFG_EXPR_INF));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0));
}

TEST_F(NAME, numerator_diverges_negative_inf)
{
    /*
     * -a*x^2 + b*x |
     * ------------ |      = -oo
     *  0*x^2 + d*x |x->oo
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(-a*x^2 + b*x)/(0*x^2 + d*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, -1.0, CSFG_EXPR_INF));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0));
}

TEST_F(NAME, denominator_diverges)
{
    /*
     * 0*x^2 + b*x |
     * ----------- |      = 0
     * c*x^2 + d*x |x->oo
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(0*x^2 + b*x)/(c*x^2 + d*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 0.0));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0));
}

TEST_F(NAME, converge_1)
{
    /*
     * 0*x^2 + b*x |         b
     * ----------- |      = ---
     * 0*x^2 + d*x |x->oo    d
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(0*x^2 + b*x)/(0*x^2 + d*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 1.0, "b"));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0, "d"));
}

TEST_F(NAME, converge_2)
{
    /*
     * a*x^2 + b*x |         a
     * ----------- |      = ---
     * c*x^2 + d*x |x->oo    c
     */
    int expr =
        csfg_expr_parse(&in_pool, cstr_view("(a*x^2 + b*x)/(c*x^2 + d*x)"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational_limit(
            in_pool, expr, cstr_view("x"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 1.0, "a"));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0, "c"));
}
