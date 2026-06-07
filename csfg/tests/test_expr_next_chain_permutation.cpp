#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr_next_chain_permutation

namespace {

struct TestParam
{
    const char* input;
    const char* expected_output;
    int expected_return_value;
};

const struct TestParam TEST_PARAMETERS[] = {
    {  "a",   "a", 0},
    {"a*b", "b*a", 1},
    {"b*a", "a*b", 0},
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
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }

    struct csfg_expr_pool* p;
};

INSTANTIATE_TEST_SUITE_P(, NAME, ValuesIn(TEST_PARAMETERS));

TEST_P(NAME, test)
{
    int actual   = csfg_expr_parse(&p, cstr_view(GetParam().input));
    int expected = csfg_expr_parse(&p, cstr_view(GetParam().expected_output));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    EXPECT_EQ(
        csfg_expr_next_chain_permutation(p, actual),
        GetParam().expected_return_value);

    ASSERT_TRUE(ExprEq(p, actual, p, expected));
}
