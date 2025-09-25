#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/expr_tf.h"

VEC_DEFINE(csfg_expr_vec, int, 8)

/* ------------------------------------------------------------------------- */
static int run_expansions(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_until_complete(
        pool,
        csfg_expr_op_expand_constant_exponents,
        csfg_expr_op_expand_exponent_sums,
        csfg_expr_op_expand_exponent_products,
        csfg_expr_op_distribute_products,
        NULL);
}

/* ------------------------------------------------------------------------- */
static int run_optimizations(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_until_complete(
        pool,
        csfg_expr_opt_fold_constants,
        csfg_expr_opt_remove_useless_ops,
        csfg_expr_op_simplify_products,
        csfg_expr_op_simplify_sums,
        NULL);
}

/* ------------------------------------------------------------------------- */
static int run_factorizations(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_until_complete(
        pool, csfg_expr_op_factor_common_denominator, NULL);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** num_pool,
    int*                    num_root,
    struct csfg_expr_pool** den_pool,
    int*                    den_root,
    struct strview          variable)
{
    /* Init the denominator by dividing by 1.0 */
    csfg_expr_pool_clear(*den_pool);
    *den_root = csfg_expr_lit(den_pool, 1.0);
    if (*den_root == -1)
        return -1;

    while (1)
    {
        int done = 0;

        if (run_factorizations(num_pool) == -1 ||
            run_factorizations(den_pool) == -1 ||
            run_expansions(num_pool) == -1 || run_expansions(den_pool) == -1)
        {
            return -1;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        /* rebalance fraction requires this pass */
        if (csfg_expr_op_run_until_complete(
                num_pool, csfg_expr_op_lower_negates, NULL) == -1)
            return -1;
        if (csfg_expr_op_run_until_complete(
                den_pool, csfg_expr_op_lower_negates, NULL) == -1)
            return -1;

        switch (csfg_expr_op_rebalance_fraction(
            num_pool, *num_root, den_pool, *den_root))
        {
            case -1: return -1;
            case 0: done = 1; break;
            case 1: break;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        if (done)
            break;
    }

    if (run_expansions(num_pool) == -1 ||
        run_expansions(den_pool) == -1 ||    /* make clang-format happy */
        run_optimizations(num_pool) == -1 || /* make clang-format happy */
        run_optimizations(den_pool) == -1)
    {
        return -1;
    }
    *num_root = csfg_expr_gc(*num_pool, *num_root);
    *den_root = csfg_expr_gc(*den_pool, *den_root);

    return 0;
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
