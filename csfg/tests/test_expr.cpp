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

TEST_F(NAME, add_and_sub)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, "1+2-3+4"), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(4));
}

TEST_F(NAME, eval_constant_expression)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, "(2+3*4)^2 + 4"), Ge(0));
    ASSERT_THAT(csfg_expr_eval(p, e, NULL), DoubleEq(200));
}

TEST_F(NAME, evaluate_with_variables)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, "(a+3*c)^d + e"), Ge(0));
    csfg_var_table_set_lit(&vt, cstr_view("a"), 2);
    csfg_var_table_set_lit(&vt, cstr_view("c"), 4);
    csfg_var_table_set_lit(&vt, cstr_view("d"), 2);
    csfg_var_table_set_lit(&vt, cstr_view("e"), 5);
    ASSERT_THAT(csfg_expr_eval(p, e, &vt), DoubleEq(201));
}

TEST_F(NAME, generate_variable_table)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, "a+b^c*d"), Ge(0));
    ASSERT_THAT(csfg_var_table_populate(&vt, p, e), Eq(0));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("a")), DoubleEq(0));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("b")), DoubleEq(0));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("c")), DoubleEq(1));
    EXPECT_THAT(csfg_var_table_eval(&vt, cstr_view("d")), DoubleEq(1));
}
