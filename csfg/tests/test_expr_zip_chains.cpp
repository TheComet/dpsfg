#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr_zip_chains

using namespace testing;

struct NAME : public Test
{
    void SetUp() override { csfg_expr_pool_init(&p); }
    void TearDown() override { csfg_expr_pool_deinit(p); }

    struct csfg_expr_pool* p;
};

TEST_F(NAME, mul_chain1)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a*b"));
    int right_chain = csfg_expr_parse(&p, cstr_view("w*x*y*z"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("a*b*1*1 + w*x*y*z"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain2)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a*b"));
    int right_chain = csfg_expr_parse(&p, cstr_view("a*x*y*z"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("a*b*1*1 + a*x*y*z"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain3)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a*b*c*d"));
    int right_chain = csfg_expr_parse(&p, cstr_view("y*z"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("a*b*c*d + 1*1*y*z"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain4)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a*b*c*z"));
    int right_chain = csfg_expr_parse(&p, cstr_view("y*z"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("a*b*c*z + 1*1*y*z"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain5)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("q*r*s"));
    int right_chain = csfg_expr_parse(&p, cstr_view("a*s*z"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("q*r*s*1 + a*1*s*z"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain6)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("q*r*s"));
    int right_chain = csfg_expr_parse(&p, cstr_view("r*s*z"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("q*r*s*1 + 1*r*s*z"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain7)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a*a*a"));
    int right_chain = csfg_expr_parse(&p, cstr_view("b*b*b"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("a*a*a + b*b*b"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 3);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain8)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a*a*a"));
    int right_chain = csfg_expr_parse(&p, cstr_view("a*a*a"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("a*a*a + a*a*a"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 3);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, mul_chain_equal)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("x*y*z"));
    int right_chain = csfg_expr_parse(&p, cstr_view("x*y*z"));
    int actual      = csfg_expr_add(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("x*y*z + x*y*z"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 3);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, add_chain1)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a+b"));
    int right_chain = csfg_expr_parse(&p, cstr_view("w+x+y+z"));
    int actual      = csfg_expr_mul(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("(a+b+0+0) * (w+x+y+z)"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, add_chain2)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a+b"));
    int right_chain = csfg_expr_parse(&p, cstr_view("a+x+y+z"));
    int actual      = csfg_expr_mul(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("(a+b+0+0) * (a+x+y+z)"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, add_chain3)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a+b+c+d"));
    int right_chain = csfg_expr_parse(&p, cstr_view("y+z"));
    int actual      = csfg_expr_mul(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("(a+b+c+d) * (0+0+y+z)"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, add_chain4)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("a+b+c+z"));
    int right_chain = csfg_expr_parse(&p, cstr_view("y+z"));
    int actual      = csfg_expr_mul(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("(a+b+c+z) * (0+0+y+z)"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, add_chain5)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("q+r+s"));
    int right_chain = csfg_expr_parse(&p, cstr_view("a+s+z"));
    int actual      = csfg_expr_mul(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("(q+r+s+0) * (a+0+s+z)"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, add_chain6)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("q+r+s"));
    int right_chain = csfg_expr_parse(&p, cstr_view("r+s+z"));
    int actual      = csfg_expr_mul(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("(q+r+s+0) * (0+r+s+z)"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 4);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}

TEST_F(NAME, add_chain_equal)
{
    int left_chain  = csfg_expr_parse(&p, cstr_view("x+y+z"));
    int right_chain = csfg_expr_parse(&p, cstr_view("x+y+z"));
    int actual      = csfg_expr_mul(&p, left_chain, right_chain);
    int expected    = csfg_expr_parse(&p, cstr_view("(x+y+z) * (x+y+z)"));

    ASSERT_EQ(csfg_expr_zip_chains(&p, left_chain, right_chain), 3);

    ASSERT_TRUE(csfg_expr_equal(p, actual, p, expected));
}
