#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_opt.h"
}

#define NAME test_expr_opt_fold_constants

using namespace testing;

struct NAME : public Test
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }

    struct csfg_expr_pool* p;
};

TEST_F(NAME, simplify_constant_expression)
{
    int e = csfg_expr_parse(&p, "4+2*3");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    ASSERT_THAT(p->nodes[e].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[e].value.lit, DoubleEq(10));
}

TEST_F(NAME, simplify_constant_expression_with_negate)
{
    int e = csfg_expr_parse(&p, "a^-1");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    ASSERT_THAT(p->nodes[p->nodes[e].child[1]].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[p->nodes[e].child[1]].value.lit, DoubleEq(-1));
}

TEST_F(NAME, simplify_constant_expressions_exponent)
{
    int e = csfg_expr_parse(&p, "2^5");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    ASSERT_THAT(p->nodes[e].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[e].value.lit, DoubleEq(32));
}

TEST_F(NAME, simplify_constant_expressions_with_variables)
{
    int e = csfg_expr_parse(&p, "a*(3+2)");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    ASSERT_THAT(p->nodes[p->nodes[e].child[1]].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[p->nodes[e].child[1]].value.lit, DoubleEq(5));
}

TEST_F(NAME, constant_add_sub_chain)
{
    int e = csfg_expr_parse(&p, "16+a-4+b-2-c+3");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    int c1 = p->nodes[e].child[0];
    int c2 = p->nodes[c1].child[0];
    int c3 = p->nodes[c2].child[0];
    ASSERT_THAT(p->nodes[c3].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[c3].value.lit, DoubleEq(13));
}

TEST_F(NAME, constant_mul_div_chain)
{
    int e = csfg_expr_parse(&p, "16*a/4*b/2/c*3");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    int c1 = p->nodes[e].child[0];
    int c2 = p->nodes[c1].child[0];
    int c3 = p->nodes[c2].child[0];
    ASSERT_THAT(p->nodes[c3].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[c3].value.lit, DoubleEq(6));
}

TEST_F(NAME, constant_chain_of_additions_with_2_variables_in_middle)
{
    int e = csfg_expr_parse(&p, "5+a+b+8");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    int c1 = p->nodes[e].child[0];
    int c2 = p->nodes[c1].child[0];
    ASSERT_THAT(p->nodes[c2].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[c2].value.lit, DoubleEq(13));
}

TEST_F(NAME, constant_chain_of_products_with_2_variables_in_middle)
{
    int e = csfg_expr_parse(&p, "5*a*b*8");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    int c1 = p->nodes[e].child[0];
    int c2 = p->nodes[c1].child[0];
    ASSERT_THAT(p->nodes[c2].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[c2].value.lit, DoubleEq(40));
}

TEST_F(NAME, negations)
{
    int e = csfg_expr_parse(&p, "2+-(a+3)");
    ASSERT_THAT(e, Ge(0));
    ASSERT_THAT(csfg_expr_opt_fold_constants(&p, &e), Gt(0));
    int c1 = p->nodes[e].child[0];
    int c2 = p->nodes[c1].child[0];
    ASSERT_THAT(p->nodes[c2].type, Eq(CSFG_EXPR_LIT));
    ASSERT_THAT(p->nodes[c2].value.lit, DoubleEq(40));
}
