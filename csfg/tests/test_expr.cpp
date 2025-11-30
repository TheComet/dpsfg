#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p);
        csfg_var_table_init(&vt);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(p);
        csfg_var_table_deinit(&vt);
    }

    struct csfg_expr_pool* p;
    struct csfg_var_table  vt;
};

TEST_F(NAME, negative_literal)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("-4")), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(-4));
}

TEST_F(NAME, add_and_sub)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("1+2-3+4")), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(4));
}

TEST_F(NAME, mul_and_negate)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("1*-2*3-5")), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(-11));
}

TEST_F(NAME, exp_with_negate)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("4^-2")), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(1.0 / 16));
}

TEST_F(NAME, exp_with_negate2)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("4^-2^3")), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(1.0 / 65536));
}

TEST_F(NAME, eval_constant_expression)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("(2+3*4)^2 + 4")), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(200));
}

TEST_F(NAME, float_numbers)
{
    int e;
    ASSERT_THAT(
        e = csfg_expr_parse(&p, cstr_view("(2.5-3.54*4.5)^2 + 4.2")), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(184.56489519844058));
}

TEST_F(NAME, infinity)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("oo")), Ge(0));
    ASSERT_THAT(p->nodes[e].type, Eq(CSFG_EXPR_INF));
    ASSERT_THAT(
        csfg_expr_eval(p, e, NULL),
        DoubleEq(std::numeric_limits<double>::infinity()));
}

TEST_F(NAME, evaluate_with_variables)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("(a+3*c)^d + e")), Ge(0));
    csfg_var_table_set_lit(&vt, cstr_view("a"), 2);
    csfg_var_table_set_lit(&vt, cstr_view("c"), 4);
    csfg_var_table_set_lit(&vt, cstr_view("d"), 2);
    csfg_var_table_set_lit(&vt, cstr_view("e"), 5);
    ASSERT_THAT(csfg_expr_eval(p, e, &vt), DoubleEq(201));
}

TEST_F(NAME, generate_variable_table)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("a+b^c*d")), Ge(0));
    ASSERT_THAT(csfg_var_table_populate(&vt, p, e), Eq(0));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("a")), DoubleEq(1));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("b")), DoubleEq(1));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("c")), DoubleEq(1));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("d")), DoubleEq(1));
}
