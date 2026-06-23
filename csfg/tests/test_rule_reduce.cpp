#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/platform/mfile.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_rule_reduce

namespace {

struct TestParam
{
    const char* input;
    const char* expected_output;
    int expected_run_result;
};

/*
a*c + b*a + b*b + b*c
-------------------
      (b + c)
*/

const struct TestParam TEST_PARAMETERS[] = {
    {"a*(b+c)/(b+d)", "a*(b+c)/(b+d)", 0},
    {"a*(b+c)/(b+c)", "a", 1},
    {"(a*b + a*c + b*b + b*c)/(b+c)", "a+b", 1},
    {"a*b/(b+c) + a*c/(b+c)", "a", 1},

    {
     "-G1*(G1+G2+C*s)"
        "/(G2*(G1+G2+C*s) + C*s*(G1+G2+C*s))", "-G1/(C*s+G2)",
     1, },

    {"G1*(s*C+G2+G1)*(s*C+G2+G1)"
     "/((s*C+G2+G1)*(s*C+G2+G1)*(s*C+G2))", "-(G1/(C*s+G2))",
     1},

    /*   -G1*(G2+s*C)
     * -----------------
     * (G2+s*C)*(s*C+G2) */
    {"-1*(G1*(G2+s*C))"
     "/(1*((s*C+G2)*(G2+C*s)))", "-G1/(C*s+G2)",
     1},

    /*         -G1*(G1+G2+C*s)
     * --------------------------------
     * C*s*(C*s+G2+G1) + G2*(C*s+G1+G2) */
    {"-G1*(C*s+G1+G2) "
     "/ (C*s*(C*s+G2+G1) + G2*(G2+s*C+G1))", "-G1/(C*s+G2)",
     1},
};

std::ostream& operator<<(std::ostream& os, const TestParam& p)
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
        csfg_rulebook_init(&book);
        csfg_expr_pool_init(&pool);
        csfg_expr_pool_init(&expected_pool);
        ASSERT_EQ(mfile_map_read(&mf, filename, 1), 0);
        source = strview((const char*)mf.address, 0, mf.size);
        ASSERT_EQ(csfg_rulebook_parse(&book, filename, source), 0);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(expected_pool);
        csfg_expr_pool_deinit(pool);
        csfg_rulebook_deinit(&book);
        mfile_unmap(&mf);
    }

    const char* filename = "../../csfg/src/symbolic/rulebook.txt";
    struct mfile mf;
    struct strview source;
    struct csfg_rulebook book;
    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* expected_pool;
};

INSTANTIATE_TEST_SUITE_P(, NAME, ValuesIn(TEST_PARAMETERS));

TEST_P(NAME, test)
{
    int input = csfg_expr_parse(&pool, cstr_view(GetParam().input));
    int expected =
        csfg_expr_parse(&expected_pool, cstr_view(GetParam().expected_output));
    ASSERT_GE(input, 0);
    ASSERT_GE(expected, 0);

    EXPECT_EQ(
        csfg_rulebook_run(&book, "reduce", &pool, &input),
        GetParam().expected_run_result);

    ASSERT_TRUE(ExprEq(pool, input, expected_pool, expected));
}
