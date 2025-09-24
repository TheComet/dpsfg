#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
static int move_reciprocs(
    struct csfg_expr_pool** from,
    int                     from_root,
    struct csfg_expr_pool** to,
    int                     to_root)
{
    int    n, left, right, exp;
    double value;

    if ((*from)->nodes[n].type == CSFG_EXPR_OP_POW)
    {
        exp = (*from)->nodes[n].child[1];
        if ((*from)->nodes[exp].type != CSFG_EXPR_LIT)
            return 0;

        value = (*from)->nodes[exp].value.lit;
        if (value >= 0.0)
            return 0;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_rebalance_fraction(
    struct csfg_expr_pool** num_pool,
    int*                    num_root,
    struct csfg_expr_pool** den_pool,
    int*                    den_root)
{
    int modified = 0;
    switch (move_reciprocs(num_pool, *num_root, den_pool, *den_root))
    {
        case -1: return -1;
        case 0: break;
        case 1: modified = 1; break;
    }
    switch (move_reciprocs(den_pool, *den_root, num_pool, *num_root))
    {
        case -1: return -1;
        case 0: break;
        case 1: modified = 1; break;
    }
    return modified;
}
