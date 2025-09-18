#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
}

#define NAME test_expr

using namespace testing;

struct NAME : public Test
{
    void SetUp() override { csfg_expr_init(&e); }
    void TearDown() override { csfg_expr_deinit(e); }

    struct csfg_expr* e;
};

TEST_F(NAME, eval_constant_expression)
{
    ASSERT_THAT(csfg_expr_parse(&e, "(2+3*4)^2 + 4"), Eq(0));
    ASSERT_THAT(csfg_expr_eval(e), DoubleEq(200));
}
