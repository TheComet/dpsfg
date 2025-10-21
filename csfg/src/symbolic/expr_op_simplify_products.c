#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
/* Searches the subtree "n" recursively until it matches the node "search". The
 * returned node can either be the node that matched, or if the matching subtree
 * already has a constant exponent, returns the power operator where the base is
 * the matching node. */
static int find_same_expr(const struct csfg_expr_pool* pool, int n, int search)
{
    if (pool->nodes[n].type == CSFG_EXPR_MUL)
    {
        int left = pool->nodes[n].child[0];
        int right = pool->nodes[n].child[1];
        n = find_same_expr(pool, left, search);
        if (n > -1)
            return n;
        n = find_same_expr(pool, right, search);
        if (n > -1)
            return n;
        return -1;
    }

    if (pool->nodes[n].type == CSFG_EXPR_POW &&
        pool->nodes[pool->nodes[n].child[1]].type == CSFG_EXPR_LIT)
    {
        int base = pool->nodes[n].child[0];
        if (search != base && csfg_expr_equal(pool, search, pool, base))
            return n;
        return -1;
    }

    if (search != n && csfg_expr_equal(pool, search, pool, n))
        return n;

    return -1;
}

/* ------------------------------------------------------------------------- */
static int process_chain(struct csfg_expr_pool** pool, int n, int top)
{
    for (; top > -1 && (*pool)->nodes[top].type == CSFG_EXPR_MUL;
         top = csfg_expr_find_parent((*pool), top))
    {
        int match = find_same_expr(
            (*pool),
            top,
            /* If the product is raised to a power, want to search only for the
               product and not the whole power */
            (*pool)->nodes[n].type == CSFG_EXPR_POW
                ? (*pool)->nodes[n].child[0]
                : n);
        if (match == -1)
            continue;

        if ((*pool)->nodes[n].type == CSFG_EXPR_POW)
        {
            int exp = (*pool)->nodes[n].child[1];
            int sum1 = csfg_expr_dup_single(pool, exp);
            int sum2 =
                (*pool)->nodes[match].type == CSFG_EXPR_POW
                    ? csfg_expr_dup_single(pool, (*pool)->nodes[match].child[1])
                    : csfg_expr_lit(pool, 1.0);
            if (csfg_expr_set_add(pool, exp, sum1, sum2) == -1)
                return -1;
        }
        else
        {
            int exp =
                (*pool)->nodes[match].type == CSFG_EXPR_POW
                    ? csfg_expr_add(
                          pool,
                          csfg_expr_dup_single(pool, (*pool)->nodes[match].child[1]),
                          csfg_expr_lit(pool, 1.0))
                    : csfg_expr_lit(pool, 2.0);
            if (csfg_expr_set_pow(pool, n, csfg_expr_dup_single(pool, n), exp) == -1)
                return -1;
        }
        csfg_expr_collapse_sibling_into_parent(*pool, match);
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int simplify_products(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int left = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_MUL)
            continue;

        switch (process_chain(pool, left, n))
        {
            case -1: return -1;
            case 0: break;
            case 1: modified = 1; break;
        }
        switch (process_chain(pool, right, n))
        {
            case -1: return -1;
            case 0: break;
            case 1: modified = 1; break;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_simplify_products(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, simplify_products);
}
