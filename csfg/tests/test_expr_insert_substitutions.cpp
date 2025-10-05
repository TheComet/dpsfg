#include "gmock/gmock.h"

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
    struct csfg_var_table  vt;
};

TEST_F(NAME, simple)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("a")), Ge(0));
    csfg_var_table_set_parse_expr(&vt, cstr_view("a"), cstr_view("b"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("b"), cstr_view("c"));
    ASSERT_THAT(csfg_expr_insert_substitutions(&p, e, &vt), Eq(0));
    ASSERT_THAT(p->nodes[e].type, Eq(CSFG_EXPR_VAR));
    ASSERT_THAT(
        strlist_cstr(p->var_names, p->nodes[e].value.var_idx), StrEq("c"));
}

TEST_F(NAME, with_cycles_fails)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("a")), Ge(0));
    csfg_var_table_set_parse_expr(&vt, cstr_view("a"), cstr_view("b"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("b"), cstr_view("c"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("c"), cstr_view("d"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("d"), cstr_view("b"));
    ASSERT_THAT(csfg_expr_insert_substitutions(&p, e, &vt), Eq(-1));
}
