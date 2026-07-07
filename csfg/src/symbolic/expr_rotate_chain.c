#include "csfg/symbolic/expr.h"

/* -------------------------------------------------------------------------- */
void csfg_expr_rotate_chain(struct csfg_expr_pool* pool, int chain)
{
    int first, next;

    enum csfg_expr_type op_type = pool->nodes[chain].type;
    switch (op_type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_IMAG:
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG:
        case CSFG_EXPR_POW : return;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL: break;
    }

    /*
     *     *               *
     *    / \             / \
     *   *   c    -->    *  b
     *  / \             / \
     * a   b           c   a
     */
    first = pool->nodes[chain].child[1];
    while (1)
    {
        next = pool->nodes[chain].child[0];
        if (pool->nodes[next].type == op_type)
            pool->nodes[chain].child[1] = pool->nodes[next].child[1];
        else
        {
            pool->nodes[chain].child[1] = pool->nodes[chain].child[0];
            break;
        }
        chain = next;
    }
    pool->nodes[chain].child[0] = first;
}
