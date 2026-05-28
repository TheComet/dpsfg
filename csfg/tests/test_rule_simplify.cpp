#include "gtest/gtest.h"

extern "C" {
#include "csfg/platform/mfile.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_rule_simplify

using namespace testing;

struct NAME : public Test
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

    const char*            filename = "../../csfg/src/symbolic/rulebook.txt";
    struct mfile           mf;
    struct strview         source;
    struct csfg_rulebook   book;
    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* expected_pool;
};

TEST_F(NAME, pattern_match1)
{
    int actual = csfg_expr_parse(&pool, cstr_view("a*(b+c)/(b+c)"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("a"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "simplify", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, pattern_match2)
{
    int actual = csfg_expr_parse(&pool, cstr_view("(x+y)*s/(x+y)"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("a"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "simplify", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, pattern_match3)
{
    int actual = csfg_expr_parse(&pool, cstr_view("a*(b+c)+b*(b+c)"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("(b+c)*(a+b)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, pattern_match4)
{
    int actual = csfg_expr_parse(
        &pool, cstr_view("-G1*(C*s+G1+G2)/(C*s*(C*s+G1+G2) + G2*(C*s+G1+G2))"));
    int expected = csfg_expr_parse(
        &expected_pool, cstr_view("-G1*(C*s+G1+G2)/((C*s+G2)*(C*s+G1+G2))"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "simplify", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, pattern_match5)
{
    int actual = csfg_expr_parse(&pool, cstr_view("a*b/(b+c) + a*c/(b+c)"));
    int expected =
        csfg_expr_parse(&expected_pool, cstr_view("a*(b/(b+c) + c/(b+c))"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "factor", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, no_simplification1)
{
    int actual = csfg_expr_parse(&pool, cstr_view("a*(b+c)/(b+d)"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("a*(b+c)/(b+d)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "simplify", &pool, &actual), 0);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, case1)
{
    /*
     *   -G1*(G2+s*C)
     * -----------------
     * (G2+s*C)*(s*C+G2)
     */
    int actual = csfg_expr_parse(
        &pool, cstr_view("-1*(G1*(G2+s*C))/(1*((s*C+G2)*(G2+C*s)))"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("-(G1/(s*C+G2))"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "simplify", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, case2)
{
    /*
     *         -G1*(G1+G2+C*s)
     * --------------------------------
     * C*s*(C*s+G2+G1) + G2*(C*s+G1+G2)
     */
    int actual = csfg_expr_parse(
        &pool,
        cstr_view("-G1*(C*s+G1+G2) / (C*s*(C*s+G2+G1) + G2*(G2+s*C+G1))"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("-(G1/(C*s+G2))"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "simplify", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, case3)
{
    int actual = csfg_expr_parse(
        &pool,
        cstr_view(
            "G1*(s*C+G2+G1)*(s*C+G2+G1)/"
            "((s*C+G2+G1)*(s*C+G2+G1)*(s*C+G2))"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("-(G1/(C*s+G2))"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_rulebook_run(&book, "simplify", &pool, &actual), 1);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}
