#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include <math.h>

/* ------------------------------------------------------------------------- */
static int is_almost_integer(double value, double epsilon)
{
    double nearest = round(value);
    return fabs(value - nearest) < epsilon;
}

/* ------------------------------------------------------------------------- */
static int expand_constant_exponents(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        double              value;
        int                 reps;
        int                 chain;
        int                 sign = 1;
        int                 base = (*pool)->nodes[n].child[0];
        int                 exp = (*pool)->nodes[n].child[1];
        enum csfg_expr_type type = (*pool)->nodes[n].type;

        if (type != CSFG_EXPR_OP_POW)
            continue;
        if ((*pool)->nodes[exp].type != CSFG_EXPR_LIT)
            continue;

        value = (*pool)->nodes[exp].value.lit;
        if (!is_almost_integer(value, 0.0000001))
            continue;

        if (value > 0.0)
            sign = 1, reps = value;
        else
            sign = -1, reps = -value;

        if (reps < 2)
            continue;

        chain = csfg_expr_set_mul(pool, exp, base, csfg_expr_dup(pool, base));
        for (; reps > 2; --reps)
            chain = csfg_expr_mul(pool, chain, csfg_expr_dup(pool, base));
        if (csfg_expr_set_pow(pool, n, chain, csfg_expr_lit(pool, sign)) == -1)
            return -1;

        modified = 1;
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_expand_constant_exponents(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, expand_constant_exponents);
}
