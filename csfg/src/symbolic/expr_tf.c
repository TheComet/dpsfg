#include "csfg/symbolic/expr.h"

/* ------------------------------------------------------------------------- */
/*
 * Performs the following transformations:
 *   a/s    -->  as^-1
 *   a/s^x  -->  as^-x
 *   a-s    -->  a+s*-1
 *   a-s*x  -->  a+s*x*-1
 * This makes the tree easier to work with for later stages, because
 * the assumption that no division or subtraction operators exist can be
 * made.
 */
static int eliminate_div_and_sub(
    struct csfg_expr_pool** pool, int expr, struct strview variable)
{
    return -1;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** pool, int expr, struct strview variable)
{
    /*
     * Create the "split" operator, i.e. this is the expression that splits the
     * numerator from the denominator.
     */
    if ((*pool)->nodes[expr].type != CSFG_EXPR_OP_DIV)
    {
        int one = csfg_expr_lit(pool, 1.0);
        if (one == -1)
            return -1;
        expr = csfg_expr_binop(pool, CSFG_EXPR_OP_DIV, expr, one);
        if (expr == -1)
            return -1;
    }

    return -1;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_standard_tf_to_coeff(
    struct csfg_expr_pool** pool,
    int                     expr,
    struct strview          variable,
    struct csfg_expr_vec**  num,
    struct csfg_expr_vec**  den)
{
    return -1;
}
