#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

namespace {

struct TestParamInputOutput
{
    const char* input;
    const char* expected_output;
    int expected_return_value;
};

struct TestParamFullCycle
{
    const char* input;
    int expected_number_of_permutations;
};

const struct TestParamInputOutput TEST_PARAMS_INPUT_OUTPUT[] = {
    {      "a",       "a", 0},
    {    "a*b",     "b*a", 1},
    {    "b*a",     "a*b", 0},
    {"a*b*c*d", "a*b*d*c", 1},
    {"d*c*b*a", "a*b*c*d", 0},
    {"a*b*b*a", "b*a*a*b", 1},
    {"b*b*a*a", "a*a*b*b", 0},

    {      "a",       "a", 0},
    {    "a+b",     "b+a", 1},
    {    "b+a",     "a+b", 0},
    {"a+b+c+d", "a+b+d+c", 1},
    {"d+c+b+a", "a+b+c+d", 0},
    {"a+b+b+a", "b+a+a+b", 1},
    {"b+b+a+a", "a+a+b+b", 0},
};

const struct TestParamFullCycle TEST_PARAMS_FULL_CYCLE[] = {
    // clang-format off
    {        "a",               1}, 
    {      "a*a",               1},
    {    "a*a*a",               1},
    {  "a*a*a*a",               1},
    {      "a*b",             1*2},
    {    "a*b*c",           1*2*3},
    {  "a*b*c*d",         1*2*3*4},
    {"a*b*c*d*e",       1*2*3*4*5},
    {  "a*a*b*b",   1*2*3*4/(2*2)},
    {  "a*a*b*c", 1*2*3*4/(2*1*1)},
    // clang-format on
};

std::ostream& operator<<(std::ostream& os, const struct TestParamInputOutput& p)
{
    os << p.input << " --> " << p.expected_output;
    return os;
}

std::ostream& operator<<(std::ostream& os, const struct TestParamFullCycle& p)
{
    os << p.input;
    return os;
}

} // namespace

using namespace testing;

struct test_expr_next_chain_permutation_input_output
    : public TestWithParam<TestParamInputOutput>,
      public ExprHelper
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }
    struct csfg_expr_pool* p;
};
struct test_expr_next_chain_permutation_full_cycle
    : public TestWithParam<TestParamFullCycle>,
      public ExprHelper
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }
    struct csfg_expr_pool* p;
};
struct test_expr_next_chain_permutation_sort : public Test, public ExprHelper
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }
    struct csfg_expr_pool* p;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    test_expr_next_chain_permutation_input_output,
    ValuesIn(TEST_PARAMS_INPUT_OUTPUT));
INSTANTIATE_TEST_SUITE_P(
    ,
    test_expr_next_chain_permutation_full_cycle,
    ValuesIn(TEST_PARAMS_FULL_CYCLE));

TEST_P(test_expr_next_chain_permutation_input_output, test)
{
    int actual   = csfg_expr_parse(&p, cstr_view(GetParam().input));
    int expected = csfg_expr_parse(&p, cstr_view(GetParam().expected_output));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    EXPECT_EQ(
        csfg_expr_next_chain_permutation(p, actual),
        GetParam().expected_return_value);

    EXPECT_TRUE(ExprEq(p, actual, p, expected));
}

TEST_P(test_expr_next_chain_permutation_full_cycle, test)
{
    int input    = csfg_expr_parse(&p, cstr_view(GetParam().input));
    int expected = csfg_expr_parse(&p, cstr_view(GetParam().input));
    ASSERT_GE(input, 0);
    ASSERT_GE(expected, 0);

    int permutations = 1;
    while (csfg_expr_next_chain_permutation(p, input))
        permutations++;

    EXPECT_EQ(permutations, GetParam().expected_number_of_permutations);
    EXPECT_TRUE(ExprEq(p, input, p, expected));
}

TEST_F(test_expr_next_chain_permutation_sort, test)
{
    int input    = csfg_expr_parse(&p, cstr_view("c*a*b*d"));
    int expected = csfg_expr_parse(&p, cstr_view("a*b*c*d"));
    ASSERT_GE(input, 0);
    ASSERT_GE(expected, 0);

    csfg_expr_canonicalize(p, input);
    int permutations = 1;
    while (csfg_expr_next_chain_permutation(p, input))
        permutations++;

    EXPECT_EQ(permutations, 24);
    EXPECT_TRUE(ExprEq(p, input, p, expected));
}
