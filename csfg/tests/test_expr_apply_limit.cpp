#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_tf.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_apply_limit

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

TEST_F(NAME, infinity)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("oo")), Ge(0));
    ASSERT_THAT(csfg_expr_apply_limit(p), Eq(0));
}

