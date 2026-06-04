#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_rulebook_run

using namespace testing;

struct NAME : public Test
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

// NOTE: All rules assume that the expression tree has been sorted! If you are
// writing a test, make sure to either explicitly call
// csfg_expr_canonicalize(), or write a sorted expression.

TEST_F(NAME, factorize_match_single_vars_permutation_1)
{
    const char* rule = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(rule)), 0);
    int actual   = csfg_expr_parse(&pool, cstr_view("x*y + x*z"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factorize_match_single_vars_permutation_2)
{
    const char* rule = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(rule)), 0);
    int actual   = csfg_expr_parse(&pool, cstr_view("x*y + z*x"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factorize_match_single_vars_permutation_3)
{
    const char* rule = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(rule)), 0);
    int actual   = csfg_expr_parse(&pool, cstr_view("y*x + x*z"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factorize_match_single_vars_permutation_4)
{
    const char* rule = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(rule)), 0);
    int actual   = csfg_expr_parse(&pool, cstr_view("y*x + z*x"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factorize_match_subtrees_permutation_1)
{
    const char* rule = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(rule)), 0);
    int actual = csfg_expr_parse(&pool, cstr_view("(x+(w*5))*y + (x+(w*5))*z"));
    int expected =
        csfg_expr_parse(&expected_pool, cstr_view("(x+(w*5)) * (y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factorize_multiplication_chains_permutation_1)
{
    const char* rule = "factor { \"b*a+c*a\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(rule)), 0);
    int actual   = csfg_expr_parse(&pool, cstr_view("m*q*s + p*q*z"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(w*y + w*z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}
