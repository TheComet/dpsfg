#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_opt.h"
#include <math.h>

/* ------------------------------------------------------------------------- */
static int floats_equal(double f1, double f2, double epsilon)
{
    return fabs(f1 - f2) < epsilon;
}

/* ------------------------------------------------------------------------- */
static int remove_chained_negates(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int grandchild, child = (*pool)->nodes[n].child[0];
        if ((*pool)->nodes[n].type != CSFG_EXPR_NEG)
            continue;
        if ((*pool)->nodes[child].type != CSFG_EXPR_NEG)
            continue;

        grandchild = (*pool)->nodes[child].child[0];
        (*pool)->nodes[n] = (*pool)->nodes[grandchild];
        csfg_expr_mark_deleted(*pool, child);
        csfg_expr_mark_deleted(*pool, grandchild);
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int remove_negated_products(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int left = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_MUL)
            continue;

        if ((*pool)->nodes[left].type == CSFG_EXPR_NEG &&
            (*pool)->nodes[right].type == CSFG_EXPR_NEG)
        {
            csfg_expr_collapse_into_parent(
                *pool, (*pool)->nodes[left].child[0], left);
            csfg_expr_collapse_into_parent(
                *pool, (*pool)->nodes[right].child[0], right);
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int remove_double_reciprocs(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int base2, exp2;
        int exp1 = (*pool)->nodes[n].child[1];
        int base1 = (*pool)->nodes[n].child[0];
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_POW ||
            (*pool)->nodes[base1].type != CSFG_EXPR_OP_POW)
        {
            continue;
        }

        base2 = (*pool)->nodes[base1].child[0];
        exp2 = (*pool)->nodes[base1].child[1];
        if ((*pool)->nodes[exp1].type != CSFG_EXPR_LIT ||
            (*pool)->nodes[exp2].type != CSFG_EXPR_LIT)
        {
            continue;
        }

        if (!floats_equal((*pool)->nodes[exp1].value.lit, -1.0, 0.0000001) ||
            !floats_equal((*pool)->nodes[exp2].value.lit, -1.0, 0.0000001))
        {
            continue;
        }

        (*pool)->nodes[n] = (*pool)->nodes[base2];
        csfg_expr_mark_deleted(*pool, base2);
        csfg_expr_mark_deleted(*pool, exp2);
        csfg_expr_mark_deleted(*pool, base1);
        csfg_expr_mark_deleted(*pool, exp1);
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int remove_zero_summands(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int left = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_ADD)
            continue;

        if ((*pool)->nodes[left].type == CSFG_EXPR_LIT &&
            floats_equal((*pool)->nodes[left].value.lit, 0.0, 0.0000001))
        {
            csfg_expr_collapse_into_parent(*pool, right, n);
            return 1;
        }

        if ((*pool)->nodes[right].type == CSFG_EXPR_LIT &&
            floats_equal((*pool)->nodes[right].value.lit, 0.0, 0.0000001))
        {
            csfg_expr_collapse_into_parent(*pool, left, n);
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int remove_one_products(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int c;
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_MUL)
            continue;

        for (c = 0; c != 2; ++c)
        {
            int child = (*pool)->nodes[n].child[c];
            int sibling = (*pool)->nodes[n].child[1 - c];
            if ((*pool)->nodes[child].type != CSFG_EXPR_LIT)
                continue;

            if (floats_equal((*pool)->nodes[child].value.lit, 1.0, 0.0000001))
            {
                csfg_expr_collapse_into_parent(*pool, sibling, n);
                return 1;
            }
            if (floats_equal((*pool)->nodes[child].value.lit, -1.0, 0.0000001))
            {
                csfg_expr_mark_deleted(*pool, child);
                csfg_expr_set_neg(pool, n, sibling);
                return 1;
            }
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int remove_one_and_zero_exponents(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        double value;
        int    left = (*pool)->nodes[n].child[0];
        int    right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_POW)
            continue;

        if ((*pool)->nodes[right].type != CSFG_EXPR_LIT)
            continue;

        value = (*pool)->nodes[right].value.lit;
        if (floats_equal(value, 1.0, 0.0000001))
        {
            csfg_expr_collapse_into_parent(*pool, left, n);
            return 1;
        }
        if (floats_equal(value, 0.0, 0.0000001))
        {
            csfg_expr_mark_deleted_recursive(*pool, left);
            csfg_expr_mark_deleted(*pool, right);
            csfg_expr_set_lit(pool, n, 1.0);
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_opt_remove_useless_ops(struct csfg_expr_pool** pool)
{
#define RUN(func, pool, modified)                                              \
    switch (func(pool))                                                        \
    {                                                                          \
        case -1: return -1;                                                    \
        case 0: break;                                                         \
        case 1: modified = 1; break;                                           \
    }
    int modified = 0;
    RUN(remove_chained_negates, pool, modified);
    RUN(remove_negated_products, pool, modified);
    RUN(remove_zero_summands, pool, modified);
    RUN(remove_one_products, pool, modified);
    RUN(remove_one_and_zero_exponents, pool, modified);
    RUN(remove_double_reciprocs, pool, modified);
    return modified;
#undef RUN
}
