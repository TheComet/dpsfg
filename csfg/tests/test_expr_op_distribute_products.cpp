#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
}

#define NAME test_expr_op_distribute_products

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p1);
        csfg_expr_pool_init(&p2);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(p2);
        csfg_expr_pool_deinit(p1);
    }

    struct csfg_expr_pool* p1;
    struct csfg_expr_pool* p2;
};

TEST_F(NAME, distribute_simple1)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("s*(a+b)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s*a + s*b"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_distribute_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, expand_simple2)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(a+b)*s"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s*a + s*b"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_distribute_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, binomial_expansion)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(a+b)*(c+d)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("(a*c + a*d) + (b*c + b*d)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_distribute_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, distribute_nested)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(a+b)*((c*d)+(e*f)+(g*h)))"));
    int r2 = csfg_expr_parse(
        &p2,
        cstr_view(
            "a*(c*d) + a*(e*f) + a*(g*h) + ((b*(c*d) + b*(e*f)) + b*(g*h))"));
    ASSERT_GE(r1, 0);
    ASSERT_GT(csfg_expr_op_distribute_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}
