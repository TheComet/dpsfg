#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_insert_substitutions

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p);
        csfg_var_table_init(&vt);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(p);
        csfg_var_table_deinit(&vt);
    }

    struct csfg_expr_pool* p;
    struct csfg_var_table vt;
};

TEST_F(NAME, simple)
{
    int e1, e2;
    ASSERT_GE(e1 = csfg_expr_parse(&p, cstr_view("a")), 0);
    csfg_var_table_set_parse_expr(&vt, cstr_view("a"), cstr_view("b"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("b"), cstr_view("c"));

    ASSERT_GE(e2 = csfg_expr_insert_substitutions(&p, e1, &vt), 0);

    // Original expression should not be modified
    ASSERT_EQ(p->nodes[e1].type, CSFG_EXPR_VAR);
    ASSERT_STREQ(strlist_cstr(p->var_names, p->nodes[e1].value.var_idx), "a");
    ASSERT_EQ(p->nodes[e2].type, CSFG_EXPR_VAR);
    ASSERT_STREQ(strlist_cstr(p->var_names, p->nodes[e2].value.var_idx), "c");
}

TEST_F(NAME, with_cycles_fails)
{
    int e;
    ASSERT_GE(e = csfg_expr_parse(&p, cstr_view("a")), 0);
    csfg_var_table_set_parse_expr(&vt, cstr_view("a"), cstr_view("b"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("b"), cstr_view("c"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("c"), cstr_view("d"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("d"), cstr_view("b"));

    ASSERT_EQ(csfg_expr_insert_substitutions(&p, e, &vt), -1);

    // Original expression should not be modified
    ASSERT_EQ(p->nodes[e].type, CSFG_EXPR_VAR);
    ASSERT_STREQ(strlist_cstr(p->var_names, p->nodes[e].value.var_idx), "a");
}
