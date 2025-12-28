#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
}

#define NAME test_expr_op_expand_constant_exponents

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

TEST_F(NAME, expand_positive)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a^5"));
    int r2 = csfg_expr_parse(&p2, cstr_view("(a*a*a*a*a)^1"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    ASSERT_GT(csfg_expr_op_expand_constant_exponents(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}

TEST_F(NAME, expand_negative)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a^-5"));
    int r2 = csfg_expr_parse(&p2, cstr_view("(a*a*a*a*a)^-1"));
    ASSERT_GE(r1, 0);
    ASSERT_GE(r2, 0);
    // neg(5) are 2 separate nodes, but the pass expects literal negative values
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_GT(csfg_expr_op_expand_constant_exponents(&p1), 0);
    ASSERT_TRUE(csfg_expr_equal(p1, r1, p2, r2));
}
