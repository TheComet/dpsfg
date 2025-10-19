#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
}

#define NAME test_expr_op_remove_useless_ops

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

TEST_F(NAME, remove_chained_negates)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("-(-a)"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, two_negated_products)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("-a*-b"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a*b"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, remove_zero_summands)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("0+a+0"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, remove_one_products)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("1*a*1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, remove_negative_one_products_left)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("-1*a"));
    int r2 = csfg_expr_parse(&p2, cstr_view("-a"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    csfg_expr_opt_fold_constants(&p1);
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, remove_negative_one_products_right)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a*-1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("-a"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    csfg_expr_opt_fold_constants(&p1);
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, remove_one_exponents)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a^1"));
    int r2 = csfg_expr_parse(&p2, cstr_view("a"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    csfg_expr_opt_fold_constants(&p1);
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, remove_zero_exponents)
{
    int r1 = csfg_expr_parse(&p1, cstr_view("a^0"));
    int r2 = csfg_expr_parse(&p2, cstr_view("1"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    csfg_expr_opt_fold_constants(&p1);
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}

TEST_F(NAME, cancel_products)
{
    /*
     *   -G1*(G2+s*C)
     * -----------------
     * (G2+s*C)*(G2+s*C)
     */
    int r1 = csfg_expr_parse(
        &p1, cstr_view("-1*(G1*(G2+s*C))/(1*((G2+s*C)*(s*C+G2)))"));
    int r2 = csfg_expr_parse(&p2, cstr_view("-G1/(s*C+G2)"));
    ASSERT_THAT(r1, Ge(0));
    ASSERT_THAT(r2, Ge(0));
    csfg_expr_opt_fold_constants(&p1);
    csfg_expr_opt_fold_constants(&p2);
    ASSERT_THAT(csfg_expr_opt_remove_useless_ops(&p1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(p1, r1, p2, r2), IsTrue());
}
