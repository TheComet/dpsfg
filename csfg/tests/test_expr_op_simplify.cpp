#include "gtest/gtest.h"

extern "C" {
#include "csfg/platform/mfile.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_op_simplify

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        ASSERT_EQ(mfile_map_read(&mf, filename, 1), 0);
        source = strview((const char*)mf.address, 0, mf.size);
        csfg_expr_op_init(&op);
        ASSERT_EQ(csfg_expr_op_parse_def(&op, filename, source), 0);
        csfg_expr_pool_init(&pool);
        csfg_expr_pool_init(&expected_pool);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(expected_pool);
        csfg_expr_pool_deinit(pool);
        csfg_expr_op_deinit(&op);
        mfile_unmap(&mf);
    }

    const char*            filename = "../../csfg/src/symbolic/ops.def";
    struct mfile           mf;
    struct strview         source;
    struct csfg_expr_op    op;
    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* expected_pool;
};

TEST_F(NAME, pattern_match1)
{
    int actual = csfg_expr_parse(&pool, cstr_view("a*(b+c)/(b+c)"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("a"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_expr_op_run_def(&pool, &actual, &op, "simplify"), 1);
    actual = csfg_expr_gc(pool, actual);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}

TEST_F(NAME, pattern_match2)
{
    int actual = csfg_expr_parse(&pool, cstr_view("(b+c)*a/a"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("(b+c)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_expr_op_run_def(&pool, &actual, &op, "simplify"), 1);
    actual = csfg_expr_gc(pool, actual);

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
        &pool, cstr_view("-1*(G1*(G2+s*C))/(1*((G2+s*C)*(G2+s*C)))"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("-G1/(G2+s*C)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_expr_op_run_def(&pool, &actual, &op, "simplify"), 1);
    actual = csfg_expr_gc(pool, actual);

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
        cstr_view("-G1*(C*s+G1+G2) / (C*s*(C*s+G1+G2) + G2*(C*s+G1+G2))"));
    int expected = csfg_expr_parse(&expected_pool, cstr_view("-G1/(G2+s*C)"));
    ASSERT_GE(actual, 0);
    ASSERT_GE(expected, 0);

    ASSERT_EQ(csfg_expr_op_run_def(&pool, &actual, &op, "simplify"), 1);
    actual = csfg_expr_gc(pool, actual);

    ASSERT_TRUE(csfg_expr_equal(pool, actual, expected_pool, expected));
}
