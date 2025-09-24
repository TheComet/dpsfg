#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/expr_tf.h"

VEC_DEFINE(csfg_expr_vec, int, 8)

#define RUN1(func, pool, modified)                                             \
    switch (func(pool))                                                        \
    {                                                                          \
        case -1: return -1;                                                    \
        case 0: break;                                                         \
        case 1: modified = 1; break;                                           \
    }
#define RUN2(func, pool, root, modified)                                       \
    switch (func(pool, root))                                                  \
    {                                                                          \
        case -1: return -1;                                                    \
        case 0: break;                                                         \
        case 1: modified = 1; break;                                           \
    }

/* ------------------------------------------------------------------------- */
static int run_expansions(struct csfg_expr_pool** pool)
{
    int modified = 0;
    RUN1(csfg_expr_op_expand_constant_exponents, pool, modified);
    RUN1(csfg_expr_op_expand_exponent_sums, pool, modified);
    RUN1(csfg_expr_op_expand_exponent_products, pool, modified);
    RUN1(csfg_expr_op_distribute_products, pool, modified);
    return modified;
}

/* ------------------------------------------------------------------------- */
static int run_optimizations(struct csfg_expr_pool** pool, int* root)
{
    int modified = 0;
    RUN2(csfg_expr_opt_fold_constants, pool, root, modified);
    RUN2(csfg_expr_opt_remove_useless_ops, pool, root, modified);
    RUN1(csfg_expr_op_lower_negates, pool, modified);
    /* TODO: csfg_expr_opt_combine_common_factors */
    return modified;
}

/* ------------------------------------------------------------------------- */
static int run_factorizations(struct csfg_expr_pool** pool)
{
    int modified = 0;
    RUN1(csfg_expr_op_factor_common_denominator, pool, modified);
    return modified;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** num_pool,
    int*                    num_root,
    struct csfg_expr_pool** den_pool,
    int*                    den_root,
    struct strview          variable)
{
    while (1)
    {
        int done = 0;
        if (run_factorizations(num_pool) == -1)
            return -1;
        if (run_expansions(num_pool) == -1)
            return -1;
        if (run_optimizations(num_pool, num_root) == -1)
            return -1;

        switch (csfg_expr_op_rebalance_fraction(
            num_pool, num_root, den_pool, den_root))
        {
            case -1: return -1;
            case 0: done = 1; break;
            case 1: break; ;
        }

        if (done)
            break;
    }

#if 0

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
