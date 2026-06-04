#include "csfg/symbolic/expr.h"

/* -------------------------------------------------------------------------- */
/*
 *     *               *
 *    / \             / \
 *   *   c    -->    *  a
 *  / \             / \
 * a   b           b   c
 */
static int rotate_recurse(
    struct csfg_expr_pool* pool, int expr, enum csfg_expr_type op_type)
{
    int bottom;
    int left  = pool->nodes[expr].child[0];
    int right = pool->nodes[expr].child[1];
    if (pool->nodes[left].type != op_type)
    {
        pool->nodes[expr].child[0] = right;
        return left;
    }

    bottom                     = rotate_recurse(pool, left, op_type);
    pool->nodes[left].child[1] = right;
    return bottom;
}

/* -------------------------------------------------------------------------- */
void csfg_expr_rotate_chain(struct csfg_expr_pool* pool, int chain)
{
    enum csfg_expr_type op_type = pool->nodes[chain].type;
    switch (op_type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG:
        case CSFG_EXPR_POW: return;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
            pool->nodes[chain].child[1] = rotate_recurse(pool, chain, op_type);
            break;
    }
}
