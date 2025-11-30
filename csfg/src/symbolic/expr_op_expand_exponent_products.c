#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* -------------------------------------------------------------------------- */
static int expand_exponent_products(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int                 base1, base2;
        int                 prod = (*pool)->nodes[n].child[0];
        int                 exp = (*pool)->nodes[n].child[1];
        enum csfg_expr_type type = (*pool)->nodes[n].type;

        if (type != CSFG_EXPR_POW)
            continue;
        if ((*pool)->nodes[prod].type != CSFG_EXPR_MUL)
            continue;

        base1 = (*pool)->nodes[prod].child[0];
        base2 = (*pool)->nodes[prod].child[1];

        if (csfg_expr_set_mul(
                pool,
                n,
                csfg_expr_set_pow(pool, prod, base1, exp),
                csfg_expr_pow(pool, base2, csfg_expr_dup_recurse(pool, exp))) == -1)
        {
            return -1;
        }

        modified = 1;
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_expand_exponent_products(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, expand_exponent_products);
}
