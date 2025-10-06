#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_tf.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_extract_poly_coeff

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p);
        csfg_expr_vec_init(&coeff);
    }
    void TearDown() override
    {
        csfg_expr_vec_deinit(coeff);
        csfg_expr_pool_deinit(p);
    }

    struct csfg_expr_pool* p;
    struct csfg_expr_vec*  coeff;
};

TEST_F(NAME, constant)
{
    int e;
    ASSERT_THAT(e = csfg_expr_parse(&p, cstr_view("1")), Ge(0));
    ASSERT_THAT(
        csfg_expr_extract_poly_coeff(&p, e, cstr_view("s"), &coeff), Eq(0));
}
