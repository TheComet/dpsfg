#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr_canonicalize

namespace {

struct TestParam
{
    const char* input;
    const char* expected_output;
};

const struct TestParam TEST_PARAMETERS[] = {
    {"1*1", "1*1"},
    {"c+b+a", "a+b+c"},
    {"s*C+G1", "G1+C*s"},
    {"G1+s*C", "G1+C*s"},
    {"(i+g+h)*(c+b+a)", "(a+b+c)*(g+h+i)"},
    {"y*(c+b+a) + x*(i+g+h) + (i+h+g)*y", /**/
     "x*(g+h+i) + y*(a+b+c) + y*(g+h+i)"},
    {
     "a+(b+(c+(d+(e+(f+(g+(h+(i+(j+(k+(l+(m+(n+(o+p))))))))))))))", /**/
        "a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p", },
    {"(((a+b)+(c+d))+((e+f)+(g+h)))+(((i+j)+(k+l))+((m+n)+(o+p)))",
     "a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p"},
    {"-G1*(s*C+G1+G2) / (C*s*(s*C+G1+G2) + G2*(s*C+G1+G2))",
     "-G1*(G1+G2+C*s) / (G2*(G1+G2+C*s) + C*s*(G1+G2+C*s))"},
};

std::ostream& operator<<(std::ostream& os, const TestParam& p)
{
    os << p.input << " --> " << p.expected_output;
    return os;
}
} // namespace

using namespace testing;

struct NAME : public TestWithParam<TestParam>
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

    csfg_expr_canonicalize(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}
