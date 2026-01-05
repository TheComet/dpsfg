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
    int expected = csfg_expr_parse(&p, cstr_view("a+b+c"));
    int e = csfg_expr_parse(&p, cstr_view("c+b+a"));
    csfg_expr_canonicalize(p, e);
    ASSERT_TRUE(csfg_expr_equal(p, e, p, expected));
}

TEST_F(NAME, right_tree)
{
    int expected =
        csfg_expr_parse(&p, cstr_view("a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p"));
    int e = csfg_expr_parse(
        &p,
        cstr_view(
            "a+(b+(c+(d+(e+(f+(g+(h+(i+(j+(k+(l+(m+(n+(o+p))))))))))))))"));
    csfg_expr_canonicalize(p, e);
    ASSERT_TRUE(csfg_expr_equal(p, e, p, expected));
}

TEST_F(NAME, balanced_tree)
{
    int expected =
        csfg_expr_parse(&p, cstr_view("a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p"));
    int e = csfg_expr_parse(
        &p,
        cstr_view(
            "(((a+b)+(c+d))+((e+f)+(g+h)))+(((i+j)+(k+l))+((m+n)+(o+p)))"));
    csfg_expr_canonicalize(p, e);
    ASSERT_TRUE(csfg_expr_equal(p, e, p, expected));
}

TEST_F(NAME, sums_and_products)
{
    int expected = csfg_expr_parse(&p, cstr_view("G1+C*s"));
    int e1 = csfg_expr_parse(&p, cstr_view("s*C+G1"));
    int e2 = csfg_expr_parse(&p, cstr_view("G1+s*C"));
    csfg_expr_canonicalize(p, e1);
    csfg_expr_canonicalize(p, e2);
    ASSERT_TRUE(csfg_expr_equal(p, e1, p, expected));
    ASSERT_TRUE(csfg_expr_equal(p, e2, p, expected));
}
