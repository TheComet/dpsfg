#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
static int find_same_expr(const struct csfg_expr_pool* pool, int n, int search)
{
    if (pool->nodes[search].type == CSFG_EXPR_OP_MUL)
    {
        int left = pool->nodes[search].child[0];
        int right = pool->nodes[search].child[1];
        search = find_same_expr(pool, n, left);
        if (search > -1)
            return search;
        search = find_same_expr(pool, n, right);
        if (search > -1)
            return search;
        return -1;
    }

    if (pool->nodes[search].type == CSFG_EXPR_OP_POW)
    {
        int base = pool->nodes[search].child[0];
        return find_same_expr(pool, n, base);
    }

    /* Ignore self */
    if (n != search && csfg_expr_equal(pool, n, pool, search))
        return search;

    return -1;
}

/* ------------------------------------------------------------------------- */
static int process_chain(struct csfg_expr_pool** pool, int n, int top)
{
    int match, sibling, parent = top;
    for (; top > -1 && (*pool)->nodes[top].type == CSFG_EXPR_OP_MUL;
         top = csfg_expr_find_parent((*pool), top))
    {
        match = find_same_expr((*pool), n, top);
        if (match == -1)
            continue;

        sibling = (*pool)->nodes[parent].child[0] == n
                      ? (*pool)->nodes[parent].child[1]
                      : (*pool)->nodes[parent].child[0];

        /* "match" might be the sibling of "n" */
        if (match == sibling)
        {
            int result;
            if ((*pool)->nodes[sibling].child[0] != -1)
                csfg_expr_mark_deleted_recursive(
                    *pool, (*pool)->nodes[sibling].child[0]);
            if ((*pool)->nodes[sibling].child[1] != -1)
                csfg_expr_mark_deleted_recursive(
                    *pool, (*pool)->nodes[sibling].child[1]);

            result = csfg_expr_set_pow(
                pool, parent, n, csfg_expr_set_lit(pool, sibling, 2.0));
            if (result == -1)
                return -1;
            return 1;
        }
        else /* match is in another subtree */
        {
            csfg_expr_collapse_sibling_into_parent(*pool, match);
            csfg_expr_set_pow(
                pool, n, csfg_expr_copy(pool, n), csfg_expr_lit(pool, 2.0));
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int simplify_products(struct csfg_expr_pool** pool)
{
    int n, top, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int left = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_MUL)
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
    int modified = 0;
again:
    switch (simplify_products(pool))
    {
        case -1: return -1;
        case 0: break;
        case 1: modified = 1; goto again;
    }
    return modified;
}
