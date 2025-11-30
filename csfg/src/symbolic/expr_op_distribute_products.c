#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* -------------------------------------------------------------------------- */
static int distribute_products(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int                 sum, fact, sum1, sum2;
        int                 left = (*pool)->nodes[n].child[0];
        int                 right = (*pool)->nodes[n].child[1];
        enum csfg_expr_type type = (*pool)->nodes[n].type;

        if (type != CSFG_EXPR_MUL)
            continue;

        if ((*pool)->nodes[left].type == CSFG_EXPR_ADD)
            sum = left, fact = right;
        else if ((*pool)->nodes[right].type == CSFG_EXPR_ADD)
            sum = right, fact = left;
        else
            continue;

        sum1 = (*pool)->nodes[sum].child[0];
        sum2 = (*pool)->nodes[sum].child[1];

        if (csfg_expr_set_add(
                pool,
                n,
                csfg_expr_mul(pool, fact, sum1),
                csfg_expr_set_mul(
                    pool, sum, csfg_expr_dup_recurse(pool, fact), sum2)) == -1)
        {
            return -1;
        }

        modified = 1;
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_distribute_products(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, distribute_products);
}
