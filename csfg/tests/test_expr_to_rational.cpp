#include "csfg/tests/PolyHelper.hpp"

#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/expr_tf.h"
#include "csfg/symbolic/rational.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_to_rational

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

    bool NodeEqualsVar(int n, const char* var_name) const
    {
        if (n < 0)
            return false;
        if (in_pool->nodes[n].type != CSFG_EXPR_VAR)
            return false;
        int                  idx = in_pool->nodes[n].value.var_idx;
        const struct strview view = strlist_view(in_pool->var_names, idx);
        return strview_eq_cstr(view, var_name);
    }

    struct csfg_expr_pool* in_pool;
    struct csfg_expr_pool* out_pool;
    struct csfg_rational   rational;
    struct csfg_var_table  vt;
};

TEST_F(NAME, simple)
{
    int expr = csfg_expr_parse(&in_pool, cstr_view("(s+3)^-2"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_fold_constants(&in_pool);

    ASSERT_THAT(
        csfg_expr_to_rational(
            in_pool, expr, cstr_view("s"), &out_pool, &rational),
        Eq(0));

    /* 1 / (s^2 + 6s + 9) */
    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(3));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 1.0));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 9.0));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 1, 6.0));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 2, 1.0));
}

TEST_F(NAME, sum_of_var_and_constant)
{
    int expr = csfg_expr_parse(&in_pool, cstr_view("a+4"));
    ASSERT_THAT(expr, Ge(0));

    ASSERT_THAT(
        csfg_expr_to_rational(
            in_pool, expr, cstr_view("s"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 1.0, "4+a"));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0));
}

TEST_F(NAME, multilevel_fraction)
{
    /*
     *       1
     *   ---------         a * s
     *    1     1   --->   -----
     *   --- - ---         a - s
     *    s     a
     */
    int expr = csfg_expr_parse(&in_pool, cstr_view("1/(1/s - 1/a)"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_remove_useless_ops(&in_pool);
    expr = csfg_expr_gc(in_pool, expr);

    ASSERT_THAT(
        csfg_expr_to_rational(
            in_pool, expr, cstr_view("s"), &out_pool, &rational),
        Eq(0));

    ASSERT_THAT(vec_count(rational.num), Eq(2));
    ASSERT_THAT(vec_count(rational.den), Eq(2));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 0, 0.0));
    ASSERT_TRUE(CoeffEq(out_pool, rational.num, 1, 1.0, "a"));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 0, 1.0, "a"));
    ASSERT_TRUE(CoeffEq(out_pool, rational.den, 1, -1.0));
}
