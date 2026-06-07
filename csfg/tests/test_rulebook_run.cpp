#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_rulebook_run

namespace {

struct TestParam
{
    struct
    {
        const char* name;
        const char* ruleset;
    } rule;
    const char* input;
    const char* expected_output;
};

// NOTE: All rules assume that the expression tree has been sorted! If you are
// writing a test, make sure to either explicitly call
// csfg_expr_canonicalize(), or write a sorted expression.
const struct TestParam TEST_PARAMETERS[] = {
    /* These test whether the rule is permutated correctly */
    {{"factor", "factor { \"a*x+b*x\" --> \"(a+b)*x\" }"},
     "x*y + x*z",           "x*(y+z)"                },
    {{"factor", "factor { \"a*x+b*x\" --> \"(a+b)*x\" }"},
     "x*y + y*z",           "y*(x+z)"                },
    {{"factor", "factor { \"a*x+b*x\" --> \"(a+b)*x\" }"},
     "x*z + y*z",           "z*(x+y)"                },

    {{"factor", "factor { \"a*x+b*x\" --> \"(a+b)*x\" }"},
     "(x+(5*w))*y + (x+(5*w))*z", "(x+(5*w)) * (y+z)"},

    {{"factor", "factor { \"a*x+b*x\" --> \"(a+b)*x\" }"},
     "m*q*s + p*q*z",     "q*(m*s + p*z)"            },
};

std::ostream& operator<<(std::ostream& os, const TestParam& p)
{
    os << p.rule.name << ", " << p.rule.ruleset << ", " << p.input << ", "
       << p.expected_output;
    return os;
}

} // namespace

using namespace testing;

struct NAME : public TestWithParam<TestParam>, public ExprHelper
{
    void SetUp() override
    {
        csfg_rulebook_init(&book);
        csfg_expr_pool_init(&pool);
        csfg_expr_pool_init(&expected_pool);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(expected_pool);
        csfg_expr_pool_deinit(pool);
        csfg_rulebook_deinit(&book);
    }

    struct csfg_rulebook book;
    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* expected_pool;
};

INSTANTIATE_TEST_SUITE_P(, NAME, ValuesIn(TEST_PARAMETERS));

TEST_P(NAME, test)
{
    struct strview rule            = cstr_view(GetParam().rule.ruleset);
    struct strview input           = cstr_view(GetParam().input);
    struct strview expected_output = cstr_view(GetParam().expected_output);
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", rule), 0);
    int input_expr    = csfg_expr_parse(&pool, input);
    int expected_expr = csfg_expr_parse(&expected_pool, expected_output);
    ASSERT_GE(input_expr, 0);
    ASSERT_GE(expected_expr, 0);

    csfg_expr_canonicalize(pool, input_expr);

    int run_result =
        csfg_rulebook_run(&book, GetParam().rule.name, &pool, &input_expr);
    ASSERT_EQ(run_result, 1);

    ASSERT_TRUE(ExprEq(pool, input_expr, expected_pool, expected_expr));
}
