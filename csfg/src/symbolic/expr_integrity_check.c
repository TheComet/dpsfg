#include "csfg/symbolic/expr.h"

/* ------------------------------------------------------------------------- */
int csfg_expr_integrity_check(struct csfg_expr_pool* pool)
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
