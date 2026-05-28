#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr_canonicalize

using namespace testing;

struct NAME : public Test
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }

    struct csfg_expr_pool* p;
};

TEST_F(NAME, sort_tree)
{
    int actual = csfg_expr_parse(&p, cstr_view("c+b+a"));
    int expected = csfg_expr_parse(&p, cstr_view("a+b+c"));

    csfg_expr_canonicalize(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, right_tree)
{
    int actual = csfg_expr_parse(
        &p,
        cstr_view(
            "a+(b+(c+(d+(e+(f+(g+(h+(i+(j+(k+(l+(m+(n+(o+p))))))))))))))"));
    int expected =
        csfg_expr_parse(&p, cstr_view("a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p"));

    csfg_expr_canonicalize(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, balanced_tree)
{
    int actual = csfg_expr_parse(
        &p,
        cstr_view(
            "(((a+b)+(c+d))+((e+f)+(g+h)))+(((i+j)+(k+l))+((m+n)+(o+p)))"));
    int expected =
        csfg_expr_parse(&p, cstr_view("a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p"));

    csfg_expr_canonicalize(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, sums_and_products)
{
    int actual1 = csfg_expr_parse(&p, cstr_view("s*C+G1"));
    int actual2 = csfg_expr_parse(&p, cstr_view("G1+s*C"));
    int expected = csfg_expr_parse(&p, cstr_view("G1+C*s"));

    csfg_expr_canonicalize(p, actual1);
    csfg_expr_canonicalize(p, actual2);

    ASSERT_TRUE(csfg_expr_equal(p, actual1, p, expected));
    ASSERT_TRUE(csfg_expr_equal(p, actual2, p, expected));
}

TEST_F(NAME, sub_chains1)
{
    int actual = csfg_expr_parse(&p, cstr_view("(i+g+h)*(c+b+a)"));
    int expected = csfg_expr_parse(&p, cstr_view("(a+b+c)*(g+h+i)"));

    csfg_expr_canonicalize(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, sub_chains2)
{
    int actual =
        csfg_expr_parse(&p, cstr_view("y*(c+b+a) + x*(i+g+h) + (i+h+g)*y"));
    int expected =
        csfg_expr_parse(&p, cstr_view("x*(g+h+i) + y*(a+b+c) + y*(g+h+i)"));

    csfg_expr_canonicalize(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, case2)
{
    int actual = csfg_expr_parse(
        &p, cstr_view("-G1*(s*C+G1+G2) / (C*s*(s*C+G1+G2) + G2*(s*C+G1+G2))"));
    int expected = csfg_expr_parse(
        &p, cstr_view("-G1*(G1+G2+C*s) / (G2*(G1+G2+C*s) + C*s*(G1+G2+C*s))"));

    csfg_expr_canonicalize(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}
