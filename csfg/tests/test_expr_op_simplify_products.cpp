#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
}

#define NAME test_expr_op_simplify_products

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

TEST_F(NAME, one_product)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(a+2)*(a+2)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("(a+2)^2"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_simplify_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, product_with_exponent_1)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(a+2)^2*(a+2)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("(a+2)^(2+1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_simplify_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, product_with_exponent_2)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(a+2)*(a+2)^2"));
    int r2 = csfg_expr_parse(&p2, cstr_view("(a+2)^(2+1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_simplify_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, parity)
{
    GTEST_SKIP();
    int r1 = csfg_expr_parse(&p1, cstr_view("(a+2)*(2+a)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("(a+2)^2"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_simplify_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, multiple_products)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(s*a)*(s*s)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a*(s^(2+1))"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_simplify_products(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, products_of_exponents)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("s^3*a*s^-5*s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-3*a"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_simplify_products(&p1), 0);
    csfg_expr_opt_fold_constants(&p1);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}
