#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
static int move_reciprocs(
    struct csfg_expr_pool** from,
    int                     from_root,
    struct csfg_expr_pool** to,
    int                     to_root)
{
    double value;
    int    base, exp;

    if ((*from)->nodes[from_root].type == CSFG_EXPR_OP_MUL)
    {
        int child_modified = 0;
        int left = (*from)->nodes[from_root].child[0];
        int right = (*from)->nodes[from_root].child[1];
        switch (move_reciprocs(from, left, to, to_root))
        {
            case -1: return -1;
            case 0: break;
            case 1: child_modified = 1; break;
        }
        switch (move_reciprocs(from, right, to, to_root))
        {
            case -1: return -1;
            case 0: break;
            case 1: child_modified = 1; break;
        }
        return child_modified;
    }

    if ((*from)->nodes[from_root].type != CSFG_EXPR_OP_POW)
        return 0;

    base = (*from)->nodes[from_root].child[0];
    exp = (*from)->nodes[from_root].child[1];
    if ((*from)->nodes[exp].type != CSFG_EXPR_LIT)
        return 0;

    value = (*from)->nodes[exp].value.lit;
    if (value >= 0.0)
        return 0;

    if (csfg_expr_set_mul(
            to,
            to_root,
            csfg_expr_pow(
                to,
                csfg_expr_dup_from(to, *from, base),
                csfg_expr_lit(to, -value)),
            csfg_expr_copy(to, to_root)) == -1)
    {
        return -1;
    }

    csfg_expr_mark_deleted_recursive(*from, base);
    csfg_expr_mark_deleted(*from, exp);
    csfg_expr_set_lit(from, from_root, 1.0);

    return 1;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_rebalance_fraction(
    struct csfg_expr_pool** num_pool,
    int                     num_root,
    struct csfg_expr_pool** den_pool,
    int                     den_root)
{
    int modified = 0;
    switch (move_reciprocs(num_pool, num_root, den_pool, den_root))
    {
        case -1: return -1;
        case 0: break;
        case 1: modified = 1; break;
    }
    switch (move_reciprocs(den_pool, den_root, num_pool, num_root))
    {
        case -1: return -1;
        case 0: break;
        case 1: modified = 1; break;
    }
    return modified;
}
