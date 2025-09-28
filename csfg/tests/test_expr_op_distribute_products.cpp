#include "gmock/gmock.h"

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
    int r1 = csfg_expr_parse(&p1, "s*(a+b)");
    int r2 = csfg_expr_parse(&p2, "s*a + s*b");
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    ASSERT_THAT(csfg_expr_op_distribute_products(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, expand_simple2)
{
    int r1 = csfg_expr_parse(&p1, "(a+b)*s");
    int r2 = csfg_expr_parse(&p2, "s*a + s*b");
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    ASSERT_THAT(csfg_expr_op_distribute_products(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, binomial_expansion)
{
    int r1 = csfg_expr_parse(&p1, "(a+b)*(c+d)");
    int r2 = csfg_expr_parse(&p2, "(a*c + a*d) + (b*c + b*d)");
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    ASSERT_THAT(csfg_expr_op_distribute_products(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, distribute_nested)
{
    int r1 = csfg_expr_parse(&p1, "(a+b)^((c*d)+(e*f)+(g*h)))");
    int r2 = csfg_expr_parse(&p2, "(a+b)^(c*d) * (a+b)^(e*f) * (a+b)^(g*h)");
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(csfg_expr_op_distribute_products(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}
