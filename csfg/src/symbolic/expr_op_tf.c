#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_tf.h"

VEC_DEFINE(csfg_expr_vec, int, 8)

/* ------------------------------------------------------------------------- */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** num_pool,
    int*                    num_root,
    struct csfg_expr_pool** den_pool,
    int*                    den_root,
    struct strview          variable)
{
#if 0
    csfg_expr_op_distribute_exponents();
    csfg_expr_op_expand_exponents();
    csfg_expr_op_distribute_products();

    csfg_expr_opt_fold_constants();
    csfg_expr_opt_remove_useless_ops();

    csfg_expr_op_factor_negative_exponents();
    csfg_expr_op_eliminate_negative_exponents_in_fraction();
#endif

    return -1;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_extract_poly_coeff(
    struct csfg_expr_pool** pool,
    int                     root,
    struct strview          variable,
    struct csfg_expr_vec**  coeff)
{
    return -1;
}
