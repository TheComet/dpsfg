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

    struct csfg_rulebook   book;
    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* expected_pool;
};

TEST_F(NAME, factor1)
{
    const char* source = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(source)), 0);
    int actual = csfg_expr_parse(&pool, cstr_view("x*y+x*z"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factor2)
{
    const char* source = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(source)), 0);
    int actual = csfg_expr_parse(&pool, cstr_view("x*y+z*x"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factor3)
{
    const char* source = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(source)), 0);
    int actual = csfg_expr_parse(&pool, cstr_view("y*x+x*z"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, factor4)
{
    const char* source = "factor { \"a*b+a*c\" --> \"a*(b+c)\" }";
    ASSERT_EQ(csfg_rulebook_parse(&book, "<stdin>", cstr_view(source)), 0);
    int actual = csfg_expr_parse(&pool, cstr_view("y*x+z*x"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("x*(y+z)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}
