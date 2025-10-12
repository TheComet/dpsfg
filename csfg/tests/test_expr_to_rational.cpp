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

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&pool);
        csfg_expr_pool_init(&pool_expected);
        csfg_rational_init(&rational);
        csfg_var_table_init(&vt);
    }

    void TearDown() override
    {
        csfg_var_table_deinit(&vt);
        csfg_rational_deinit(&rational);
        csfg_expr_pool_deinit(pool_expected);
        csfg_expr_pool_deinit(pool);
    }

    bool NodeEqualsVar(int n, const char* var_name) const
    {
        if (n < 0)
            return false;
        if (pool->nodes[n].type != CSFG_EXPR_VAR)
            return false;
        int                  idx = pool->nodes[n].value.var_idx;
        const struct strview view = strlist_view(pool->var_names, idx);
        return strview_eq_cstr(view, var_name);
    }

    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* pool_expected;
    struct csfg_rational   rational;
    struct csfg_var_table  vt;
};

TEST_F(NAME, simple)
{
    int expr = csfg_expr_parse(&pool, cstr_view("(s+3)^-2"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_fold_constants(&pool);
    ASSERT_THAT(
        csfg_expr_to_rational(&pool, expr, cstr_view("s"), &rational), Eq(0));
    /* 1 / (s^2 + 6s + 9) */
    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_get(rational.num, 0)->factor, DoubleEq(1.0));
    ASSERT_THAT(vec_get(rational.num, 0)->expr, Eq(-1));
    ASSERT_THAT(vec_count(rational.den), Eq(3));
    ASSERT_THAT(vec_get(rational.den, 0)->factor, DoubleEq(9.0));
    ASSERT_THAT(vec_get(rational.den, 0)->expr, Eq(-1));
    ASSERT_THAT(vec_get(rational.den, 1)->factor, DoubleEq(6.0));
    ASSERT_THAT(vec_get(rational.den, 1)->expr, Eq(-1));
    ASSERT_THAT(vec_get(rational.den, 2)->factor, DoubleEq(1.0));
    ASSERT_THAT(vec_get(rational.den, 2)->expr, Eq(-1));
}

TEST_F(NAME, sum_of_var_and_constant)
{
    int expr = csfg_expr_parse(&pool, cstr_view("a+4"));
    int expr_expected = csfg_expr_parse(&pool_expected, cstr_view("4+a"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_remove_useless_ops(&pool);
    expr = csfg_expr_gc(pool, expr);
    ASSERT_THAT(
        csfg_expr_to_rational(&pool, expr, cstr_view("s"), &rational), Eq(0));
    ASSERT_THAT(vec_count(rational.num), Eq(1));
    ASSERT_THAT(vec_get(rational.num, 0)->factor, DoubleEq(1.0));
    ASSERT_THAT(
        csfg_expr_equal(
            pool, vec_get(rational.num, 0)->expr, pool_expected, expr_expected),
        IsTrue());
    ASSERT_THAT(vec_count(rational.den), Eq(1));
    ASSERT_THAT(vec_get(rational.den, 0)->factor, DoubleEq(1.0));
    ASSERT_THAT(vec_get(rational.den, 0)->expr, Eq(-1));
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
    int expr = csfg_expr_parse(&pool, cstr_view("1/(1/s - 1/a)"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_remove_useless_ops(&pool);
    expr = csfg_expr_gc(pool, expr);
    ASSERT_THAT(
        csfg_expr_to_rational(&pool, expr, cstr_view("s"), &rational), Eq(0));
    ASSERT_THAT(vec_count(rational.num), Eq(2));
    ASSERT_THAT(vec_get(rational.num, 0)->factor, DoubleEq(0.0));
    ASSERT_THAT(vec_get(rational.num, 0)->expr, Eq(-1));
    ASSERT_THAT(vec_get(rational.num, 1)->factor, DoubleEq(1.0));
    ASSERT_THAT(NodeEqualsVar(vec_get(rational.num, 1)->expr, "a"), IsTrue());
    ASSERT_THAT(vec_count(rational.den), Eq(2));
    ASSERT_THAT(vec_get(rational.den, 0)->factor, DoubleEq(1.0));
    ASSERT_THAT(NodeEqualsVar(vec_get(rational.num, 0)->expr, "a"), IsTrue());
    ASSERT_THAT(vec_get(rational.den, 1)->factor, DoubleEq(-1.0));
    ASSERT_THAT(vec_get(rational.den, 1)->expr, Eq(-1));
}
