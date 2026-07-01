#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_simplify

namespace {

struct TestParam
{
    const char* input;
    const char* expected_output;
};

const struct TestParam TEST_PARAMETERS[] = {
    {"1/(s*x)", "1/(s*x)"},
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

    actual = csfg_expr_simplify(&p, actual);
    ASSERT_GE(actual, 0);

    ASSERT_TRUE(ExprEq(p, actual, p, expected));
}
