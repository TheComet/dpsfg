#include "csfg/symbolic/expr.h"

/* ------------------------------------------------------------------------- */
static int visit_all_nodes(struct csfg_expr_pool* pool, int n)
{
    int left, right;

    if (pool->nodes[n].visited)
        return -1;
    pool->nodes[n].visited = 1;

    left = pool->nodes[n].child[0];
    right = pool->nodes[n].child[1];
    if (left > -1)
        if (visit_all_nodes(pool, left) != 0)
            return -1;
    if (right > -1)
        if (visit_all_nodes(pool, right) != 0)
            return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_integrity_check(struct csfg_expr_pool* pool)
{
    int n, root;
    if (pool == NULL)
        return 0;

    /* Find any node that isn't GC */
    for (n = 0; n != pool->count; ++n)
        if (pool->nodes[n].type != CSFG_EXPR_GC)
            break;
    if (n == pool->count)
        return 0;
    root = n;

    /* Try to find the root, while considering that there could be loops */
    for (n = 0; n != pool->count; ++n)
        pool->nodes[n].visited = 0;
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

    for (n = 0; n != pool->count; ++n)
        pool->nodes[n].visited = 0;

    /* Try to visit every node */
    if (visit_all_nodes(pool, root) != 0)
        return -1;

    /* Check that all non-GC nodes were reachable */
    for (n = 0; n != pool->count; ++n)
        if (pool->nodes[n].type != CSFG_EXPR_GC && pool->nodes[n].visited == 0)
            return -1;

    return 0;
}
