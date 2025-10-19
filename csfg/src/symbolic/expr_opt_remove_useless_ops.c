#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
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
    int n, modified = 0;
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
        modified = 1;
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
static int remove_negated_products(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
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
            modified = 1;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
static int remove_double_reciprocs(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
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
        modified = 1;
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
static int remove_zero_summands(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
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
            modified = 1;
        }
        else if (
            (*pool)->nodes[right].type == CSFG_EXPR_LIT &&
            floats_equal((*pool)->nodes[right].value.lit, 0.0, 0.0000001))
        {
            csfg_expr_collapse_into_parent(*pool, left, n);
            modified = 1;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
static int remove_one_products(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int c;
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_MUL)
            continue;

        for (c = 0; c != 2; ++c)
        {
            double value;
            int    child = (*pool)->nodes[n].child[c];
            int    sibling = (*pool)->nodes[n].child[1 - c];
            if ((*pool)->nodes[child].type != CSFG_EXPR_LIT)
                continue;

            value = (*pool)->nodes[child].value.lit;
            if (floats_equal(value, 1.0, 0.0000001))
            {
                csfg_expr_collapse_into_parent(*pool, sibling, n);
                modified = 1;
                break;
            }
            else if (floats_equal(value, -1.0, 0.0000001))
            {
                csfg_expr_mark_deleted(*pool, child);
                csfg_expr_set_neg(pool, n, sibling);
                modified = 1;
                break;
            }
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
static int remove_one_and_zero_exponents(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
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
            modified = 1;
        }
        else if (floats_equal(value, 0.0, 0.0000001))
        {
            csfg_expr_mark_deleted_recursive(*pool, left);
            csfg_expr_mark_deleted(*pool, right);
            csfg_expr_set_lit(pool, n, 1.0);
            modified = 1;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
static int
find_common_products(struct csfg_expr_pool* pool, int expr1, int expr2)
{
    int left, right, rc;
    if (csfg_expr_equal(pool, expr1, pool, expr2))
    {
        // TODO: This crashes if there is no parent. Need to replace with "1" in
        // this case
        csfg_expr_collapse_sibling_into_parent(pool, expr1);
        csfg_expr_collapse_sibling_into_parent(pool, expr2);
        return 1;
    }

    if (pool->nodes[expr1].type == CSFG_EXPR_OP_MUL ||
        pool->nodes[expr1].type == CSFG_EXPR_NEG)
    {
        left = pool->nodes[expr1].child[0];
        right = pool->nodes[expr1].child[1];
        if ((rc = find_common_products(pool, left, expr2)))
            return rc;
        if ((rc = find_common_products(pool, right, expr2)))
            return rc;
    }

    if (pool->nodes[expr2].type == CSFG_EXPR_OP_MUL ||
        pool->nodes[expr2].type == CSFG_EXPR_NEG)
    {
        left = pool->nodes[expr2].child[0];
        right = pool->nodes[expr2].child[1];
        if ((rc = find_common_products(pool, expr1, left)))
            return rc;
        if ((rc = find_common_products(pool, expr1, right)))
            return rc;
    }

    return 0;
}
static int cancel_products(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        double value;
        int    left, right, parent;
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_POW)
            continue;

        left = (*pool)->nodes[n].child[0];
        right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[right].type != CSFG_EXPR_LIT)
            continue;

        value = (*pool)->nodes[right].value.lit;
        if (!floats_equal(value, -1.0, 0.0000001))
            continue;

        parent = csfg_expr_find_parent(*pool, n);
        if (parent < 0)
            continue;

        switch (find_common_products(*pool, parent, left))
        {
            case -1: return -1;
            case 1: modified = 1;
            case 0: break;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_opt_remove_useless_ops(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run(
        pool,
        remove_chained_negates,
        remove_negated_products,
        remove_zero_summands,
        remove_one_products,
        remove_one_and_zero_exponents,
        cancel_products,
        remove_double_reciprocs,
        NULL);
}
