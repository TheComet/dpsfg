#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
static int lower_negates(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int child = (*pool)->nodes[n].child[0];
        if ((*pool)->nodes[n].type != CSFG_EXPR_NEG)
            continue;
        if ((*pool)->nodes[child].type == CSFG_EXPR_MUL)
        {
            csfg_expr_set_mul(
                pool,
                n,
                csfg_expr_set_neg(pool, child, (*pool)->nodes[child].child[0]),
                (*pool)->nodes[child].child[1]);
            modified = 1;
        }
        else if ((*pool)->nodes[child].type == CSFG_EXPR_POW)
        {
            int neg_one = csfg_expr_lit(pool, -1.0);
            if (csfg_expr_set_mul(pool, n, neg_one, child) == -1)
                return -1;
            modified = 1;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_lower_negates(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, lower_negates);
}
