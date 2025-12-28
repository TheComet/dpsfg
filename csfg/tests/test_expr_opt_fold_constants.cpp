#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_opt.h"
}

#define NAME test_expr_opt_fold_constants

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

TEST_F(NAME, simplify_constant_expression)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("4+2*3"));
    int r2 = csfg_expr_parse(&p2, cstr_view("10"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, simplify_constant_expression_with_negate)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a^-1"));
    ASSERT_GE(r1, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_EQ(p1->nodes[p1->nodes[r1].child[1]].type, CSFG_EXPR_LIT);
    ASSERT_FLOAT_EQ(p1->nodes[p1->nodes[r1].child[1]].value.lit, -1.0);
}

TEST_F(NAME, simplify_constant_expressions_exponent)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("2^5"));
    int r2 = csfg_expr_parse(&p2, cstr_view("32"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, simplify_constant_expressions_with_variables)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a*(3+2)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a*5"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, constant_add_sub_chain)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("16+a-4+b-2-c+3"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a+b-c+13"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, constant_mul_div_chain)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("16*a/4*b/2/c*3"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a*b*c^-1*6"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p2), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, constant_chain_of_additions_with_2_variables_in_middle)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("5+a+b+8"));
    int r2 = csfg_expr_parse(&p2, cstr_view("13+a+b"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, constant_blob_additions_with_2_variables_in_middle)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(5+a)+(b+8)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("13+a+b"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, constant_chain_of_products_with_2_variables_in_middle)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("5*a*b*8"));
    int r2 = csfg_expr_parse(&p2, cstr_view("40*a*b"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, constant_blob_of_products_with_2_variables_in_middle)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("(5*a)*(b*8)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("40*a*b"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, negations)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("2+-(a+3)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a-1"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_opt_fold_constants(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}
