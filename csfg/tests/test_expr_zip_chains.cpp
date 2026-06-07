#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr_zip_chains

namespace {

struct TestParam
{
    const char* left_chain;
    const char* right_chain;
    const char* expected;
    int expected_zip_retval;
    int (*combine_expr)(struct csfg_expr_pool**, int, int);
};

const struct TestParam TEST_PARAMETERS_NONE[] = {
    {"a*b", "c^2", "a*b + c^2", 0, csfg_expr_add},
};

const struct TestParam TEST_PARAMETERS_MUL[] = {
    {      "a",   "x*y",             "a*1*1 + 1*x*y", 3, csfg_expr_add},
    {    "a*b", "x*y*z",     "a*b*1*1*1 + 1*1*x*y*z", 5, csfg_expr_add},
    {  "a*c*e", "b*d*f", "a*1*c*1*e*1 + 1*b*1*d*1*f", 6, csfg_expr_add},
    {"a*b*c*z",   "y*z",     "a*b*c*1*z + 1*1*1*y*z", 5, csfg_expr_add},
    {  "q*r*s", "r*s*z",         "q*r*s*1 + 1*r*s*z", 4, csfg_expr_add},
    {  "a*a*a", "b*b*b", "a*a*a*1*1*1 + 1*1*1*b*b*b", 6, csfg_expr_add},
    {  "a*a*a", "a*a*a",             "a*a*a + a*a*a", 3, csfg_expr_add},
    {  "a*b*c", "a*b*c",             "a*b*c + a*b*c", 3, csfg_expr_add},
};

const struct TestParam TEST_PARAMETERS_ADD[] = {
    {      "a",   "x+y",             "(a+0+0) * (0+x+y)", 3, csfg_expr_mul},
    {    "a+b", "x+y+z",     "(a+b+0+0+0) * (0+0+x+y+z)", 5, csfg_expr_mul},
    {  "a+c+e", "b+d+f", "(a+0+c+0+e+0) * (0+b+0+d+0+f)", 6, csfg_expr_mul},
    {"a+b+c+z",   "y+z",     "(a+b+c+0+z) * (0+0+0+y+z)", 5, csfg_expr_mul},
    {  "q+r+s", "r+s+z",         "(q+r+s+0) * (0+r+s+z)", 4, csfg_expr_mul},
    {  "a+a+a", "b+b+b", "(a+a+a+0+0+0) * (0+0+0+b+b+b)", 6, csfg_expr_mul},
    {  "a+a+a", "a+a+a",             "(a+a+a) * (a+a+a)", 3, csfg_expr_mul},
    {  "a+b+c", "a+b+c",             "(a+b+c) * (a+b+c)", 3, csfg_expr_mul},
};

const struct TestParam TEST_PARAMETERS_CASES[] = {
    {"y*(x+5*w)", "z*(x+5*w)", "y*1*(x+5*w) + 1*z*(x+5*w)", 3, csfg_expr_add},
};

std::ostream& operator<<(std::ostream& os, const TestParam& p)
{
    os << "zip(\"" << p.left_chain << "\", \"" << p.right_chain << "\") --> \""
       << p.expected << "\"";
    return os;
}

} // namespace

using namespace testing;

struct NAME : public TestWithParam<TestParam>, public ExprHelper
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }
    struct csfg_expr_pool* p;
};

INSTANTIATE_TEST_SUITE_P(none, NAME, ValuesIn(TEST_PARAMETERS_NONE));
INSTANTIATE_TEST_SUITE_P(add, NAME, ValuesIn(TEST_PARAMETERS_ADD));
INSTANTIATE_TEST_SUITE_P(mul, NAME, ValuesIn(TEST_PARAMETERS_MUL));
INSTANTIATE_TEST_SUITE_P(cases, NAME, ValuesIn(TEST_PARAMETERS_CASES));

TEST_P(NAME, test)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view(GetParam().left_chain));
    int right_chain = csfg_expr_parse(&p, cstr_view(GetParam().right_chain));
    int expected    = csfg_expr_parse(&p, cstr_view(GetParam().expected));
    int actual      = GetParam().combine_expr(&p, left_chain, right_chain);

    EXPECT_EQ(
        csfg_expr_zip_chains(&p, left_chain, right_chain),
        GetParam().expected_zip_retval);

    ASSERT_TRUE(ExprEq(p, actual, p, expected));
}
