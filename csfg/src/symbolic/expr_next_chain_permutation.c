#include "csfg/symbolic/expr.h"

/* -------------------------------------------------------------------------- */
/*     *
 *    / \
 *   *   a
 *  / \
 * c   b
 */
static int find_pivot(
    const struct csfg_expr_pool* pool, int chain, enum csfg_expr_type op_type)
{
    int value, next_value;
    value = pool->nodes[chain].child[1];
    while (1)
    {
        chain      = pool->nodes[chain].child[0];
        next_value = pool->nodes[chain].type == op_type
                       ? pool->nodes[chain].child[1]
                       : chain;
        if (next_value < value)
            return next_value;
        if (pool->nodes[chain].type != op_type)
            return -1;
        value = next_value;
    }
}

/* -------------------------------------------------------------------------- */
/*        *             *
 *       / \           / \
 *      *   a   -->   *   d
 *     / \           / \
 *    *   b         *   c
 *   / \           / \
 *  d   c         a   b
 */
static void reverse_chain(
    struct csfg_expr_pool* pool,
    int n,
    int stop_at_value,
    enum csfg_expr_type op_type)
{
}

/* -------------------------------------------------------------------------- */
int csfg_expr_next_chain_permutation(struct csfg_expr_pool* pool, int chain)
{
    enum csfg_expr_type op_type;
    int pivot;

    op_type = pool->nodes[chain].type;
    switch (op_type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG:
        case CSFG_EXPR_POW: return 0;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL: break;
    }

    pivot = find_pivot(pool, chain, op_type);
    /* If pivot point does not exist, reverse the whole array */
    if (pivot == -1)
    {
        reverse(arr.begin(), arr.end());
        return;
    }

    // find the element from the right that
    // is greater than pivot
    for (int i = n - 1; i > pivot; i--)
    {
        if (arr[i] > arr[pivot])
        {
            swap(arr[i], arr[pivot]);
            break;
        }
    }

    // Reverse the elements from pivot + 1 to the
    // end to get the next permutation
    reverse(arr.begin() + pivot + 1, arr.end());
}
