#include "gtest/gtest.h"

extern "C" {
#include "csfg/platform/mfile.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_op_parse

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        ASSERT_EQ(mfile_map_read(&mf, filename, 1), 0);
        source = strview((const char*)mf.address, 0, mf.size);
        csfg_expr_op_init(&op);
        csfg_expr_pool_init(&pool);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(pool);
        csfg_expr_op_deinit(&op);
        mfile_unmap(&mf);
    }

    const char*            filename = "../../csfg/src/symbolic/ops.def";
    struct mfile           mf;
    struct strview         source;
    struct csfg_expr_op    op;
    struct csfg_expr_pool* pool;
};

TEST_F(NAME, parse_nested_groups_correctly)
{
    struct strview src = cstr_view(
        "simplify {\n"
        "    \"s*1\" --> \"1*s\"\n"
        "    \"s*2\" --> \"2*s\"\n"
        "    {\n"
        "        \"s*3\" --> \"3*s\"\n"
        "        \"s*4\" --> \"4*s\"\n"
        "    }\n"
        "    {}\n"
        "    {\n"
        "        \"s*5\" --> \"5*s\"\n"
        "        \"s*6\" --> \"6*s\"\n"
        "        {\n"
        "            {\n"
        "                \"s*7\" --> \"7*s\"\n"
        "                \"s*9\" --> \"6*s\"\n"
        "            }\n"
        "            {\n"
        "                \"s*11\" --> \"5*s\"\n"
        "                \"s*13\" --> \"6*s\"\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    {\n"
        "        \"s*5\" --> \"5*s\"\n"
        "        \"s*6\" --> \"6*s\"\n"
        "        {\n"
        "            {\n"
        "                \"s*7\" --> \"7*s\"\n"
        "                \"s*9\" --> \"6*s\"\n"
        "            }\n"
        "            {\n"
        "                \"s*11\" --> \"5*s\"\n"
        "                \"s*13\" --> \"6*s\"\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n");

    ASSERT_EQ(csfg_expr_op_parse_def(&op, "<stdin>", src), 0);
}

TEST_F(NAME, insert_named_substitutions_into_groups)
{
    struct strview src = cstr_view(
        "distribute_products {\n"
        "    \"s*(a+b)\"       --> \"s*a + s*b\"\n"
        "}\n"
        "factor_common_denominator {\n"
        "    \"a + s^-c\"      --> \"s^-c * (a*s^c + 1)\"\n"
        "    \"a + b*s^-c\"    --> \"s^-c * (a*s^c + b)\"\n"
        "}\n"
        "simplify {\n"
        "    distribute_products\n"
        "    factor_common_denominator\n"
        "}\n");

    ASSERT_EQ(csfg_expr_op_parse_def(&op, "<stdin>", src), 0);
}
