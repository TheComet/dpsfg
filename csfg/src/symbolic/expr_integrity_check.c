#include "csfg/symbolic/expr.h"

/* -------------------------------------------------------------------------- */
int csfg_expr_integrity_check_allow_islands(struct csfg_expr_pool* pool)
{
    int n, n2, root;
    if (pool == NULL)
        return 0;

    /* Find any node that isn't GC */
    for (n = 0; n != pool->count; ++n)
    {
        if (pool->nodes[n].type == CSFG_EXPR_GC)
            continue;

        /* Try to find the root, while considering that there could be loops */
        root = n;
        for (n2 = 0; n2 != pool->count; ++n2)
            pool->nodes[n2].visited = 0;
        while (1)
        {
            int parent = csfg_expr_find_parent(pool, root);
            if (parent == -1)
                break;
            if (pool->nodes[parent].visited)
                return -1;
            pool->nodes[parent].visited = 1;
            root = parent;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_integrity_check_recurse(struct csfg_expr_pool* pool, int n)
{
    int left, right;

    if (n < 0 || n > csfg_expr_pool_count(pool))
        return -1;

    if (pool->nodes[n].visited)
        return -1;
    pool->nodes[n].visited = 1;

    left = pool->nodes[n].child[0];
    right = pool->nodes[n].child[1];

    switch (pool->nodes[n].type)
    {
        case CSFG_EXPR_GC: return -1;

        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF:
            if (left != -1 || right != -1)
                return -1;
            break;

        case CSFG_EXPR_NEG:
            if (right != -1)
                return -1;
            if (csfg_expr_integrity_check_recurse(pool, left) != 0)
                return -1;
            break;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            if (csfg_expr_integrity_check_recurse(pool, left) != 0 ||
                csfg_expr_integrity_check_recurse(pool, right) != 0)
                return -1;
            break;
    }

    return 0;
}
int csfg_expr_integrity_check(struct csfg_expr_pool* pool, int expr)
{
    int n;

    if (pool == NULL)
    {
        if (expr != -1)
            return -1;
        return 0;
    }

    for (n = 0; n != pool->count; ++n)
        pool->nodes[n].visited = 0;

    if (csfg_expr_integrity_check_recurse(pool, expr) != 0)
        return -1;

    for (n = 0; n != pool->count; ++n)
        if (pool->nodes[n].visited == 0)
            return -1;

    return 0;
}
