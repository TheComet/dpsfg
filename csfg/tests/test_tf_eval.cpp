#include "gmock/gmock.h"

extern "C" {
#include "csfg/numeric/tf.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_tf_eval

using namespace testing;

namespace {

MATCHER_P2(
    ComplexEq,
    expected_real,
    expected_imag,
    "matches a csfg_complex approximately")
{
    return ExplainMatchResult(
        AllOf(
            Field(&csfg_complex::real, DoubleNear(expected_real, 1e-6)),
            Field(&csfg_complex::imag, DoubleNear(expected_imag, 1e-6))),
        arg,
        result_listener);
}

} // namespace

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&pool);
        csfg_expr_pool_init(&tf_pool);
        csfg_tf_expr_init(&tf_expr);
        csfg_var_table_init(&vt);
        csfg_tf_init(&tf);
    }
    void TearDown() override
    {
        csfg_tf_deinit(&tf);
        csfg_var_table_deinit(&vt);
        csfg_tf_expr_deinit(&tf_expr);
        csfg_expr_pool_deinit(tf_pool);
        csfg_expr_pool_deinit(pool);
    }

    struct csfg_expr_pool* pool;
    struct csfg_expr_pool* tf_pool;
    struct csfg_tf_expr    tf_expr;
    struct csfg_var_table  vt;
    struct csfg_tf         tf;
    struct csfg_complex    z;
};

TEST_F(NAME, bandpass)
{
    const struct csfg_coeff_expr* coeff;

    /*
     *        k*wp*s
     * --------------------
     * s^2 + wp/qp*s + wp^2
     */
    int expr = csfg_expr_parse(&pool, cstr_view("k*wp*s/(s^2+wp/qp*s+wp^2)"));
    csfg_expr_to_rational(pool, expr, cstr_view("s"), &tf_pool, &tf_expr);
    vec_for_each (tf_expr.num, coeff)
        csfg_var_table_populate(&vt, tf_pool, coeff->expr);
    vec_for_each (tf_expr.den, coeff)
        csfg_var_table_populate(&vt, tf_pool, coeff->expr);

    /* k=1, wp=1, qp=1 */
    csfg_tf_from_symbolic(&tf, tf_pool, &tf_expr, &vt);
    ASSERT_THAT(csfg_tf_eval(&tf, csfg_complex(0, 0)), ComplexEq(0, 0));
    ASSERT_THAT(csfg_tf_eval(&tf, csfg_complex(0, 1)), ComplexEq(1, 0));
    ASSERT_THAT(
        csfg_tf_eval(&tf, csfg_complex(0, 2)),
        ComplexEq(0.3076923, -0.4615384));
    ASSERT_THAT(
        csfg_tf_eval(&tf, csfg_complex(0, 5)),
        ComplexEq(0.0415973, -0.1996672));

    /* k=4, wp=0.1, qp=0.5 */
    csfg_var_table_set_lit(&vt, cstr_view("k"), 4);
    csfg_var_table_set_lit(&vt, cstr_view("wp"), 0.1);
    csfg_var_table_set_lit(&vt, cstr_view("qp"), 0.5);
    csfg_tf_from_symbolic(&tf, tf_pool, &tf_expr, &vt);
    ASSERT_THAT(csfg_tf_eval(&tf, csfg_complex(0, 0)), ComplexEq(0, 0));
    ASSERT_THAT(
        csfg_tf_eval(&tf, csfg_complex(0, 1)),
        ComplexEq(0.07842368, -0.3881972));
    ASSERT_THAT(
        csfg_tf_eval(&tf, csfg_complex(0, 2)),
        ComplexEq(0.01990037, -0.1985062));
    ASSERT_THAT(
        csfg_tf_eval(&tf, csfg_complex(0, 5)),
        ComplexEq(0.00319744, -0.0799040));
}
