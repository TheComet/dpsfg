#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
static int simplify_sums(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_simplify_sums(struct csfg_expr_pool** pool)
{
    int modified = 0;
again:
    switch (simplify_sums(pool))
    {
        case -1: return -1;
        case 0: break;
        case 1: modified = 1; goto again;
    }
    return modified;
}
