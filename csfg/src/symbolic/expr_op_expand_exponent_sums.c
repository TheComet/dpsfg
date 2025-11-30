#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* -------------------------------------------------------------------------- */
static int expand_exponent_sums(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int                 exp1, exp2;
        int                 base = (*pool)->nodes[n].child[0];
        int                 sum = (*pool)->nodes[n].child[1];
        enum csfg_expr_type type = (*pool)->nodes[n].type;

        if (type != CSFG_EXPR_POW)
            continue;
        if ((*pool)->nodes[sum].type != CSFG_EXPR_ADD)
            continue;

        exp1 = (*pool)->nodes[sum].child[0];
        exp2 = (*pool)->nodes[sum].child[1];

        if (csfg_expr_set_mul(
                pool,
                n,
                csfg_expr_pow(pool, base, exp1),
                csfg_expr_set_pow(
                    pool, sum, csfg_expr_dup_recurse(pool, base), exp2)) == -1)
        {
            return -1;
        }

        modified = 1;
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_expand_exponent_sums(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, expand_exponent_sums);
}
