#include "csfg/symbolic/expr.h"
#include "csfg/util/mem.h"
#include <assert.h>

/* -------------------------------------------------------------------------- */
static int*
find_pivot(struct csfg_expr_pool* pool, int chain, enum csfg_expr_type op_type)
{
    int next_chain, *value, *next_value;
    value = &pool->nodes[chain].child[1];
    while (1)
    {
        next_chain = pool->nodes[chain].child[0];
        next_value = pool->nodes[next_chain].type == op_type
                       ? &pool->nodes[next_chain].child[1]
                       : &pool->nodes[chain].child[0];
        if (csfg_expr_lexicographical_compare(pool, *value, *next_value) < 0)
            return next_value;
        value = next_value;
        chain = next_chain;
        if (pool->nodes[chain].type != op_type)
            return NULL;
    }
}

/* -------------------------------------------------------------------------- */
static int* find_successor(
    struct csfg_expr_pool* pool,
    int chain,
    int pivot,
    enum csfg_expr_type op_type)
{
    int* value;
    while (1)
    {
        /* The successor must always exist, otherwise something is wrong with
         * the algoritihm */
        CSFG_DEBUG_ASSERT(pool->nodes[chain].type == op_type);
        value = &pool->nodes[chain].child[1];
        if (csfg_expr_lexicographical_compare(pool, pivot, *value) > 0)
            return value;
        chain = pool->nodes[chain].child[0];
    }
}

/* -------------------------------------------------------------------------- */
static int reverse_chain(
    struct csfg_expr_pool* pool,
    int chain,
    int stop_at_value,
    enum csfg_expr_type op_type)
{
    int n, value;

    int values_count = 0, values_capacity = 64;
    int values_stack[64];
    int* values_vec = values_stack;
#define values_vec_push(value)                                                 \
    if (values_count == values_capacity)                                       \
    {                                                                          \
        int* new_values;                                                       \
        values_capacity *= 2;                                                  \
        new_values =                                                           \
            values_count == sizeof(values_stack) / sizeof(*values_stack)       \
                ? mem_alloc(sizeof(*values_stack) * values_capacity)           \
                : mem_realloc(                                                 \
                      values_vec, sizeof(*values_stack) * values_capacity);    \
        if (new_values == NULL)                                                \
            return -1;                                                         \
    }                                                                          \
    values_vec[values_count++] = value;

    n = chain;
    while (1)
    {
        value = pool->nodes[n].type == op_type ? pool->nodes[n].child[1] : n;
        if (value == stop_at_value)
            break;
        values_vec_push(value);
        if (pool->nodes[n].type != op_type)
            break;
        n = pool->nodes[n].child[0];
    }

    n = chain;
    while (values_count > 0)
    {
        pool->nodes[n].child[1] = values_vec[--values_count];
        if (pool->nodes[pool->nodes[n].child[0]].type != op_type)
            if (values_count > 0)
                pool->nodes[n].child[0] = values_vec[--values_count];
        n = pool->nodes[n].child[0];
    }

    if (values_capacity != 64)
        mem_free(values_vec);

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_next_chain_permutation(struct csfg_expr_pool* pool, int chain)
{
    enum csfg_expr_type op_type;
    int tmp, *pivot, *successor;

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
    if (pivot == NULL)
    {
        /* If pivot point does not exist, reverse the whole array */
        if (reverse_chain(pool, chain, -1, op_type) != 0)
            return -1;
        return 0;
    }

    successor  = find_successor(pool, chain, *pivot, op_type);
    tmp        = *pivot;
    *pivot     = *successor;
    *successor = tmp;

    if (reverse_chain(pool, chain, *pivot, op_type) != 0)
        return -1;
    return 1;
}
