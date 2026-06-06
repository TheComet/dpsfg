#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include <assert.h>

/* -------------------------------------------------------------------------- */
static int is_binop(enum csfg_expr_type type)
{
    switch (type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG: return 0;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW: break;
    }
    return 1;
}

/* -------------------------------------------------------------------------- */
static float identity_value_for_type(enum csfg_expr_type type)
{
    switch (type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG:
        case CSFG_EXPR_POW: break;

        case CSFG_EXPR_ADD: return 0.0;
        case CSFG_EXPR_MUL: return 1.0;
    }

    CSFG_DEBUG_ASSERT(0);
    return 0.0;
}

/* -------------------------------------------------------------------------- */
static int is_identity(
    const struct csfg_expr_pool* pool, int expr, enum csfg_expr_type op_type)
{
    if (pool->nodes[expr].type != CSFG_EXPR_LIT)
        return 0;
    return pool->nodes[expr].value.lit == identity_value_for_type(op_type);
}

/* -------------------------------------------------------------------------- */
static int insert_identity(
    struct csfg_expr_pool** pool, int chain, enum csfg_expr_type op_type)
{
    int left  = (*pool)->nodes[chain].child[0];
    int right = (*pool)->nodes[chain].child[1];
    if ((*pool)->nodes[chain].type == op_type)
        return csfg_expr_set_binop(
            *pool,
            chain,
            op_type,
            csfg_expr_binop(pool, op_type, left, right),
            csfg_expr_lit(pool, identity_value_for_type(op_type)));
    return csfg_expr_set_binop(
        *pool,
        chain,
        op_type,
        csfg_expr_dup_single(pool, chain),
        csfg_expr_lit(pool, identity_value_for_type(op_type)));
}

/* -------------------------------------------------------------------------- */
static int append_identity(
    struct csfg_expr_pool** pool, int chain, enum csfg_expr_type op_type)
{
    if (csfg_expr_set_binop(
            *pool,
            chain,
            op_type,
            csfg_expr_lit(pool, identity_value_for_type(op_type)),
            csfg_expr_dup_single(pool, chain)) < 0)
        return -1;
    return (*pool)->nodes[chain].child[0];
}

/* -------------------------------------------------------------------------- */
static int
next(const struct csfg_expr_pool* pool, int chain, enum csfg_expr_type op_type)
{
    if (pool->nodes[chain].type != op_type)
        return -1;
    return pool->nodes[chain].child[0];
}

/* -------------------------------------------------------------------------- */
static int
value(const struct csfg_expr_pool* pool, int chain, enum csfg_expr_type op_type)
{
    if (pool->nodes[chain].type == op_type)
        return pool->nodes[chain].child[1];
    return chain;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_zip_chains(struct csfg_expr_pool** pool, int chain_a, int chain_b)
{
    int i, val_a, val_b, compare, next_a, next_b;
    enum csfg_expr_type op_type;
    CSFG_DEBUG_ASSERT(csfg_expr_is_canonicalized(*pool, chain_a));
    CSFG_DEBUG_ASSERT(csfg_expr_is_canonicalized(*pool, chain_b));

    if (chain_a == chain_b)
        return 1;

    if (is_binop((*pool)->nodes[chain_a].type))
        op_type = (*pool)->nodes[chain_a].type;
    else if (is_binop((*pool)->nodes[chain_b].type))
        op_type = (*pool)->nodes[chain_b].type;
    else
        return 1;

    for (i = 0;; i++)
    {
        val_a = value(*pool, chain_a, op_type);
        val_b = value(*pool, chain_b, op_type);

        if (!is_identity(*pool, val_a, op_type) &&
            !is_identity(*pool, val_b, op_type))
        {
            compare = csfg_expr_lexicographical_compare(*pool, val_a, val_b);
            if (compare > 0 && insert_identity(pool, chain_a, op_type) < 0)
                return -1;
            if (compare < 0 && insert_identity(pool, chain_b, op_type) < 0)
                return -1;
        }

        next_a = next(*pool, chain_a, op_type);
        next_b = next(*pool, chain_b, op_type);
        if (next_a < 0 && next_b < 0)
            break;
        if (next_a < 0 &&
            (next_a = append_identity(pool, chain_a, op_type)) < 0)
            return -1;
        if (next_b < 0 &&
            (next_b = append_identity(pool, chain_b, op_type)) < 0)
            return -1;
        chain_a = next_a, chain_b = next_b;
    }

    return i + 1;
}
