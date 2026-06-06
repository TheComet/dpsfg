#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr_rotate_chain

namespace {

struct TestParam
{
    const char* input;
    const char* expected_output;
};

const struct TestParam TEST_PARAMETERS[] = {
    {      "a*b",       "b*a"},
    {"a*b*c*d*e", "e*a*b*c*d"},
    {"a*b+c+d+e", "e+a*b+c+d"},
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

    csfg_expr_rotate_chain(p, actual);

    ASSERT_TRUE(ExprEq(p, actual, p, expected));
}
