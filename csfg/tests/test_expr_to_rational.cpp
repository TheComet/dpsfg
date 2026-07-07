#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_to_rational

namespace {

struct TestParam
{
    const char* input;
    const char* expected_output;
};

const struct TestParam TEST_PARAMETERS[] = {
    // Try to test all codepaths in the quite large switch/case in tf_expr.c
    {              "5",                   "5/1"},
    {             "oo",                  "oo/1"},
    {              "a",                   "a/1"},
    {              "s",             "(0+1*s)/1"},
    {             "-5",                  "-5/1"},
    {            "-oo",                 "-oo/1"},
    {             "-a",                  "-a/1"},
    {             "-s",             "(0-1*s)/1"},
    {          "N1/D1",                 "N1/D1"},
    {"(N1/D1)+(N2/D2)", "(N2*D1+N1*D2)/(D1*D2)"},
    {"(N1/D1)*(N2/D2)",       "(N1*N2)/(D1*D2)"},
    {     "(N1/D1)^-3", "(D1*D1*D1)/(N1*N1*N1)"},
    {     "(N1/D1)^-2",       "(D1*D1)/(N1*N1)"},
    {     "(N1/D1)^-1",                 "D1/N1"},
    {      "(N1/D1)^0",                   "1/1"},
    {      "(N1/D1)^1",                 "N1/D1"},
    {      "(N1/D1)^2",       "(N1*N1)/(D1*D1)"},
    {      "(N1/D1)^3", "(N1*N1*N1)/(D1*D1*D1)"},
    // A few specific cases that caused problems in the past
    {  "1/(1/s - 1/a)",             "a*s/(a-s)"},
    // clang-format off
    {"1/(s/c+1)*1/((s/c)^2+s/c+1)",
                          "c*(c*c*c) /"
     "(c*(c*c*c) + (c*(c*c) + c*c*c)*s + (c*c+c*c)*s^2 + c*s^3)"},
    // clang-format on
};

std::ostream& operator<<(std::ostream& os, const struct TestParam& p)
{
    os << p.input << " --> " << p.expected_output;
    return os;
}

} // namespace

using namespace testing;

struct NAME : public TestWithParam<TestParam>, public ExprHelper
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p);
        csfg_tf_expr_init(&tf);
    }

    void TearDown() override
    {
        csfg_tf_expr_deinit(&tf);
        csfg_expr_pool_deinit(p);
    }

    struct csfg_expr_pool* p;
    struct csfg_tf_expr tf;
};

INSTANTIATE_TEST_SUITE_P(, NAME, ValuesIn(TEST_PARAMETERS));

TEST_P(NAME, test)
{
    int actual   = csfg_expr_parse(&p, cstr_view(GetParam().input));
    int expected = csfg_expr_parse(&p, cstr_view(GetParam().expected_output));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, actual, "s"), 0);
    actual = csfg_rational_to_expr(&tf, &p, "s");
    ASSERT_GE(actual, 0);
    csfg_rule_remove_useless_ops(&p);
    actual = csfg_expr_gc(p, actual);

    ASSERT_TRUE(ExprEq(p, actual, p, expected));
}

TEST_F(NAME, simple)
{
    int expr = csfg_expr_parse(&p, cstr_view("(s+3)^-2"));
    ASSERT_GE(expr, 0);
    csfg_rule_fold_constants(&p);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    /* 1 / (s^2 + 6s + 9) */
    ASSERT_EQ(vec_count(tf.num), 1);
    ASSERT_EQ(vec_count(tf.den), 3);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, 1.0));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 9.0));
    ASSERT_TRUE(CoeffEq(p, tf.den, 1, 6.0));
    ASSERT_TRUE(CoeffEq(p, tf.den, 2, 1.0));
}

TEST_F(NAME, sum_of_var_and_constant)
{
    int expr = csfg_expr_parse(&p, cstr_view("a+4"));
    ASSERT_GE(expr, 0);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(vec_count(tf.num), 1);
    ASSERT_EQ(vec_count(tf.den), 1);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, 1.0, "4+a"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 1.0));
}

TEST_F(NAME, multilevel_fraction)
{
    /*
     *       1
     *   ---------         a * s
     *    1     1   --->   -----
     *   --- - ---         a - s
     *    s     a
     */
    int expr = csfg_expr_parse(&p, cstr_view("1/(1/s - 1/a)"));
    ASSERT_GE(expr, 0);
    csfg_rule_remove_useless_ops(&p);
    expr = csfg_expr_gc(p, expr);

    ASSERT_EQ(csfg_expr_to_rational(&tf, &p, expr, "s"), 0);

    ASSERT_EQ(vec_count(tf.num), 2);
    ASSERT_EQ(vec_count(tf.den), 2);
    ASSERT_TRUE(CoeffEq(p, tf.num, 0, 0.0));
    ASSERT_TRUE(CoeffEq(p, tf.num, 1, 1.0, "a"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 0, 1.0, "a"));
    ASSERT_TRUE(CoeffEq(p, tf.den, 1, -1.0));
}
