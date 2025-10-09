#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
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
        csfg_expr_pool_init(&expected_pool);
        csfg_rational_init(&rational);
        csfg_var_table_init(&vt);
    }

    void TearDown() override
    {
        csfg_var_table_deinit(&vt);
        csfg_rational_deinit(&rational);
        csfg_expr_pool_deinit(expected_pool);
        csfg_expr_pool_deinit(pool);
    }

    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* expected_pool;
    struct csfg_rational   rational;
    struct csfg_var_table  vt;
};

TEST_F(NAME, simple)
{
    // int expr = csfg_expr_parse(&pool, cstr_view("(s+1)^2"));
    int expr = csfg_expr_parse(&pool, cstr_view("(a+b)*(c+s)"));
    // (a+b)*s + (a+b)*c
    int expected_num =
        csfg_expr_parse(&expected_pool, cstr_view("s^2 + 2*s + 1"));
    int expected_den = csfg_expr_parse(&expected_pool, cstr_view("1"));
    ASSERT_THAT(expr, Ge(0));
    ASSERT_THAT(expected_num, Ge(0));
    ASSERT_THAT(expected_den, Ge(0));
    ASSERT_THAT(
        csfg_expr_to_rational(&pool, expr, cstr_view("s"), &rational), Eq(0));
}

TEST_F(NAME, multilevel_fraction)
{
    GTEST_SKIP();
    /*
     *       1
     *   ---------         a * s
     *    1     1   --->   -----
     *   --- - ---         a - s
     *    s     a
     */
    int expr = csfg_expr_parse(&pool, cstr_view("1/(1/s - 1/a)"));
    int expected_num = csfg_expr_parse(&expected_pool, cstr_view("a*s"));
    int expected_den = csfg_expr_parse(&expected_pool, cstr_view("a-s"));
    ASSERT_THAT(expr, Ge(0));
    ASSERT_THAT(expected_num, Ge(0));
    ASSERT_THAT(expected_den, Ge(0));
    ASSERT_THAT(
        csfg_expr_to_rational(&pool, expr, cstr_view("s"), &rational), Eq(0));
}
