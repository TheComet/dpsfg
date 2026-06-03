#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include <assert.h>

/* -------------------------------------------------------------------------- */
static void rebalance_tree(struct csfg_expr_pool* pool, int n)
{
    switch ((enum csfg_expr_type)pool->nodes[n].type)
    {
        case CSFG_EXPR_GC : CSFG_DEBUG_ASSERT(0);
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG: break;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL: {
            /*
             *     +             +
             *    / \           / \
             *   a   +   -->   +   b
             *      / \       / \
             *     b   c     a   c
             */
            int a, op, b, c;

            a  = pool->nodes[n].child[0];
            op = pool->nodes[n].child[1];
            while (pool->nodes[op].type == pool->nodes[n].type)
            {
                b = pool->nodes[op].child[0];
                c = pool->nodes[op].child[1];

                csfg_expr_set_binop(
                    pool,
                    n,
                    pool->nodes[n].type,
                    csfg_expr_set_binop(pool, op, pool->nodes[n].type, a, c),
                    b);

                a  = pool->nodes[n].child[0];
                op = pool->nodes[n].child[1];
            }

            break;
        }

        case CSFG_EXPR_POW: break;
    }

    if (pool->nodes[n].child[0] > -1)
        rebalance_tree(pool, pool->nodes[n].child[0]);
    if (pool->nodes[n].child[1] > -1)
        rebalance_tree(pool, pool->nodes[n].child[1]);
}

/* -------------------------------------------------------------------------- */
static int compare_and_swap(struct csfg_expr_pool* pool, int n)
{
    int a, b, c, op;
    /*
     *      +
     *     / \
     *    +   c
     *   / \
     *  a   b
     */
    op = pool->nodes[n].child[0];
    c  = pool->nodes[n].child[1];
    if (pool->nodes[op].type != pool->nodes[n].type)
    {
        if (csfg_expr_lexicographical_compare(pool, op, c) > 0)
            return 0;
        csfg_expr_set_binop(pool, n, pool->nodes[n].type, c, op);
        return 1;
    }

    a = pool->nodes[op].child[0];
    b = pool->nodes[op].child[1];
    if (csfg_expr_lexicographical_compare(pool, b, c) > 0)
        return 0;

    /* Swap b <-> c */
    csfg_expr_set_binop(
        pool,
        n,
        pool->nodes[n].type,
        csfg_expr_set_binop(pool, op, pool->nodes[n].type, a, c),
        b);
    return 1;
}
static void bubble_sort_chain(struct csfg_expr_pool* pool, int top_of_chain)
{
    int n, swapped;
    enum csfg_expr_type type = pool->nodes[top_of_chain].type;

    do
    {
        swapped = 0;
        for (n = top_of_chain;
             pool->nodes[n].type == type && pool->nodes[n].child[0] > -1;
             n = pool->nodes[n].child[0])
        {
            if (compare_and_swap(pool, n))
                swapped = 1;
        }
    } while (swapped);
}
static void bubble_sort_tree(struct csfg_expr_pool* pool, int n)
{
    /* Want to sort from leaf to root, because
     * csfg_expr_lexicographical_compare() assumes sub-chains are sorted */
    if (pool->nodes[n].child[0] > -1)
        bubble_sort_tree(pool, pool->nodes[n].child[0]);
    if (pool->nodes[n].child[1] > -1)
        bubble_sort_tree(pool, pool->nodes[n].child[1]);

    switch ((enum csfg_expr_type)pool->nodes[n].type)
    {
        case CSFG_EXPR_GC : CSFG_DEBUG_ASSERT(0);
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG: break;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL: {
            bubble_sort_chain(pool, n);
            break;
        }

        case CSFG_EXPR_POW: break;
    }
}

/* -------------------------------------------------------------------------- */
void csfg_expr_canonicalize(struct csfg_expr_pool* pool, int expr)
{
    rebalance_tree(pool, expr);
    bubble_sort_tree(pool, expr);
}

/* -------------------------------------------------------------------------- */
int csfg_expr_is_canonicalized(const struct csfg_expr_pool* pool, int expr)
{
    (void)pool, (void)expr;
    return 1; /* TODO */
}
