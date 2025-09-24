#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
}

#define NAME test_expr_op_rebalance_fraction

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&n1);
        csfg_expr_pool_init(&n2);
        csfg_expr_pool_init(&d1);
        csfg_expr_pool_init(&d2);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(d2);
        csfg_expr_pool_deinit(d1);
        csfg_expr_pool_deinit(n2);
        csfg_expr_pool_deinit(n1);
    }

    struct csfg_expr_pool* n1;
    struct csfg_expr_pool* n2;
    struct csfg_expr_pool* d1;
    struct csfg_expr_pool* d2;
};

TEST_F(NAME, distribute_simple)
{
    int nr1 = csfg_expr_parse(&n1, "");
    int nr2 = csfg_expr_parse(&n2, "s*a + s*b");
    ASSERT_THAT(nr1, Ge(0));
    ASSERT_THAT(nr2, Ge(0));
    int dr1;
    ASSERT_THAT(csfg_expr_op_rebalance_fraction(&n1, &nr1, &d1, &dr1), Gt(0));
    ASSERT_THAT(csfg_expr_equal(n1, nr1, n2, nr2), IsTrue());
}
