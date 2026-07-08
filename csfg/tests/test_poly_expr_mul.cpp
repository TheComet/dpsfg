#include "csfg/tests/ExprHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly_expr.h"
}

#define NAME test_poly_expr_mul

namespace {

struct Coeff
{
    double factor;
    const char* expr;
};

struct TestParam
{
    std::vector<Coeff> input1;
    std::vector<Coeff> input2;
    std::vector<Coeff> expected_output;
};

const struct TestParam TEST_PARAMETERS[] = {
    { {{0, ""}},  {{0, ""}},     {{0, ""}}},
    {{{0, "a"}},  {{0, ""}},     {{0, ""}}},
    { {{0, ""}}, {{0, "b"}},     {{0, ""}}},
    {{{0, "a"}}, {{0, "b"}},     {{0, ""}}},

    { {{1, ""}},  {{1, ""}},     {{1, ""}}},
    {{{1, "a"}},  {{1, ""}},    {{1, "a"}}},
    { {{1, ""}}, {{1, "b"}},    {{1, "b"}}},
    {{{1, "a"}}, {{1, "b"}},  {{1, "a*b"}}},

    { {{1, ""}},  {{3, ""}},     {{3, ""}}},
    {{{1, "a"}},  {{3, ""}},    {{3, "a"}}},
    { {{1, ""}}, {{3, "b"}},    {{3, "b"}}},
    {{{1, "a"}}, {{3, "b"}},  {{3, "a*b"}}},

    { {{5, ""}},  {{1, ""}},     {{5, ""}}},
    {{{5, "a"}},  {{1, ""}},    {{5, "a"}}},
    { {{5, ""}}, {{1, "b"}},    {{5, "b"}}},
    {{{5, "a"}}, {{1, "b"}},  {{5, "a*b"}}},

    { {{5, ""}},  {{3, ""}},    {{15, ""}}},
    {{{5, "a"}},  {{3, ""}},   {{15, "a"}}},
    { {{5, ""}}, {{3, "b"}},   {{15, "b"}}},
    {{{5, "a"}}, {{3, "b"}}, {{15, "a*b"}}},

    // clang-format off
    {{{1, "a"}, {1, ""}},  // a + s
     {{1, "a*a*a"}, {1, "a*a"}, {1, "a"}}, // a^3 + s*a^2 + s^2*a
     {{1, "a*(a*a*a)"}, {1, "a*(a*a)+a*a*a"}, {1, "a*a+a*a"}, {1, "a"}}},
    {{{1, "a"}, {1, ""}},  // a + s
     {{1, "b*b*b"}, {1, "b*b"}, {1, "b"}}, // b^3 + s*b^2 + s^2*b
     {{1, "a*(b*b*b)"}, {1, "a*(b*b)+b*b*b"}, {1, "a*b+b*b"}, {1, "b"}}},
    // clang-format on
};

std::ostream& operator<<(std::ostream& os, const std::vector<Coeff>& p)
{
    int n = 0;
    for (const Coeff& c : p)
    {
        if (n)
            os << " + ";
        os << c.factor;
        if (*c.expr)
            os << "*" << c.expr;
        if (n == 1)
            os << "s";
        if (n > 1)
            os << "s^" << n;
        n++;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const struct TestParam& p)
{
    os << "(" << p.input1 << ") * (" << p.input2 << ") = " << p.expected_output;
    return os;
}

} // namespace

using namespace testing;

struct NAME : public TestWithParam<TestParam>, public ExprHelper
{
    void SetUp() override
    {
        csfg_poly_expr_init(&out);
        csfg_poly_expr_init(&p1);
        csfg_poly_expr_init(&p2);
        csfg_expr_pool_init(&pool);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(pool);
        csfg_poly_expr_deinit(p2);
        csfg_poly_expr_deinit(p1);
        csfg_poly_expr_deinit(out);
    }

    struct csfg_poly_expr* out;
    struct csfg_poly_expr* p1;
    struct csfg_poly_expr* p2;
    struct csfg_expr_pool* pool;
};

INSTANTIATE_TEST_SUITE_P(, NAME, ValuesIn(TEST_PARAMETERS));

TEST_P(NAME, test)
{
    for (const Coeff& c : GetParam().input1)
    {
        int expr = *c.expr ? csfg_expr_parse(&pool, cstr_view(c.expr)) : -1;
        csfg_poly_expr_push(&p1, csfg_coeff_expr(c.factor, expr));
    }
    for (const Coeff& c : GetParam().input2)
    {
        int expr = *c.expr ? csfg_expr_parse(&pool, cstr_view(c.expr)) : -1;
        csfg_poly_expr_push(&p2, csfg_coeff_expr(c.factor, expr));
    }

    ASSERT_EQ(csfg_poly_expr_mul(&pool, &out, p1, p2), 0);

    int n = 0;
    for (const Coeff& c : GetParam().expected_output)
    {
        if (*c.expr)
            ASSERT_TRUE(CoeffEq(pool, out, n, c.factor, c.expr)) << n;
        else
            ASSERT_TRUE(CoeffEq(pool, out, n, c.factor)) << n;
        n++;
    }
}

TEST_F(NAME, multiple_same_polynomial_multiple_times)
{
    int e = csfg_expr_parse(&pool, cstr_view("s"));
    csfg_poly_expr_push(&p1, csfg_coeff_expr(2.0, e));
    csfg_poly_expr_push(&p2, csfg_coeff_expr(1.0, e));

    ASSERT_EQ(csfg_poly_expr_mul(&pool, &out, p1, p2), 0);
    ASSERT_EQ(csfg_poly_expr_mul(&pool, &p2, p1, out), 0);
    ASSERT_EQ(csfg_poly_expr_mul(&pool, &out, p1, p2), 0);

    ASSERT_TRUE(CoeffEq(pool, out, 0, 8.0, "s*(s*s*s)"));
}
