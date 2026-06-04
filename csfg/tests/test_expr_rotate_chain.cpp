#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr_rotate_chain

using namespace testing;

struct NAME : public Test
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }

    struct csfg_expr_pool* p;
};

TEST_F(NAME, case1)
{
    int actual   = csfg_expr_parse(&p, cstr_view("a*b"));
    int expected = csfg_expr_parse(&p, cstr_view("b*a"));

    csfg_expr_rotate_chain(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, case2)
{
    int actual   = csfg_expr_parse(&p, cstr_view("a*b*c*d*e"));
    int expected = csfg_expr_parse(&p, cstr_view("b*c*d*e*a"));

    csfg_expr_rotate_chain(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, case3)
{
    int actual   = csfg_expr_parse(&p, cstr_view("a*b+c+d+e"));
    int expected = csfg_expr_parse(&p, cstr_view("c+d+e+a*b"));

    csfg_expr_rotate_chain(p, actual);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}
