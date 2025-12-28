#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
}

#define NAME test_expr_op_factor_common_denominator

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

TEST_F(NAME, case1)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a+s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1+1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, case2)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a+b*s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1+b)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, case1_mirror)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("s^-1*b+a"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(b+a*s^1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, case2_mirror)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("s^-1*b+a"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(b+a*s^1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, case1_negated)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a-s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1-1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    csfg_expr_op_lower_negates(&p1);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, case2_negated)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a-b*s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1-b)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    csfg_expr_op_lower_negates(&p1);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, multiple_summands1)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a+b+s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*((a+b)*s^1+1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, multiple_summands2)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a+s^-1+b"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1 + 1 + b*s^1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, multiple_summands1_negated)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a-b-s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*((a-b)*s^1-1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    csfg_expr_op_lower_negates(&p1);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, multiple_summands2_negated)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a-s^-1-b"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1 - 1 + (-b)*s^1)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    csfg_expr_op_lower_negates(&p1);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, multiple_products1)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a + b*c*s^-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1 + b*c)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, multiple_products2)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a + b*s^-1*c"));
    int r2 = csfg_expr_parse(&p2, cstr_view("s^-1*(a*s^1 + b*c)"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_factor_common_denominator(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}
