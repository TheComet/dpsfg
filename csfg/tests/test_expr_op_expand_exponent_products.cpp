#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
}

#define NAME test_expr_op_expand_exponent_products

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

TEST_F(NAME, expand_simple)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(a*b)^s"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a^s*b^s"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_expand_exponent_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, expand_nested)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("((c+d)*(e+f)*(g+h))^(a+b)"));
    int r2 = csfg_expr_parse(
        &p2, cstr_view("(c+d)^(a+b) * (e+f)^(a+b) * (g+h)^(a+b)"));
    ASSERT_GE(r1, 0);
    ASSERT_GT(csfg_expr_op_expand_exponent_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}
