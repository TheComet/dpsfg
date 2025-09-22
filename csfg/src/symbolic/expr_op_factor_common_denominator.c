#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
static int factor_common_denominator(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_factor_common_denominator(struct csfg_expr_pool** pool)
{
    int modified = 0;
    while (factor_common_denominator(pool))
        modified = 1;
    return modified;
}
