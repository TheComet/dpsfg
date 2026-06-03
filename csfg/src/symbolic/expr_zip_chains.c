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
static int insert_identity_into_chain(struct csfg_expr_pool** pool, int expr)
{
    enum csfg_expr_type op_type = (*pool)->nodes[expr].type;
    int left                    = (*pool)->nodes[expr].child[0];
    int right                   = (*pool)->nodes[expr].child[1];
    return csfg_expr_set_binop(
        *pool,
        expr,
        op_type,
        csfg_expr_binop(pool, op_type, left, right),
        csfg_expr_lit(pool, identity_value_for_type(op_type)));
}

/* -------------------------------------------------------------------------- */
static int insert_identity_left(
    struct csfg_expr_pool** pool, int expr, enum csfg_expr_type op_type)
{
    return csfg_expr_set_binop(
        *pool,
        expr,
        op_type,
        csfg_expr_lit(pool, identity_value_for_type(op_type)),
        csfg_expr_dup_recurse(pool, expr));
}
static int insert_identity_right(
    struct csfg_expr_pool** pool, int expr, enum csfg_expr_type op_type)
{
    return csfg_expr_set_binop(
        *pool,
        expr,
        op_type,
        csfg_expr_dup_recurse(pool, expr),
        csfg_expr_lit(pool, identity_value_for_type(op_type)));
}

/* -------------------------------------------------------------------------- */
static int zip_chains(
    struct csfg_expr_pool** pool,
    int chain_a,
    int chain_b,
    enum csfg_expr_type op_type)
{
    enum csfg_expr_type type_a, type_b;
    int right_a, right_b, compare, child_count;

    type_a = (*pool)->nodes[chain_a].type;
    type_b = (*pool)->nodes[chain_b].type;
    if (type_a == op_type && type_b == op_type)
    {
        right_a = (*pool)->nodes[chain_a].child[1];
        right_b = (*pool)->nodes[chain_b].child[1];
        compare = csfg_expr_lexicographical_compare(*pool, right_a, right_b);
        if (compare > 0 && insert_identity_into_chain(pool, chain_a) < 0)
            return -1;
        if (compare < 0 && insert_identity_into_chain(pool, chain_b) < 0)
            return -1;
    }
    else if (type_a == op_type && !is_binop(type_b))
    {
        right_a = (*pool)->nodes[chain_a].child[1];
        compare = csfg_expr_lexicographical_compare(*pool, right_a, chain_b);
        if (compare >= 0 && insert_identity_left(pool, chain_b, op_type) < 0)
            return -1;
        if (compare < 0 && insert_identity_right(pool, chain_b, op_type) < 0)
            return -1;
    }
    else if (!is_binop(type_a) && type_b == op_type)
    {
        right_b = (*pool)->nodes[chain_b].child[1];
        compare = csfg_expr_lexicographical_compare(*pool, right_b, chain_a);
        if (compare >= 0 && insert_identity_left(pool, chain_a, op_type) < 0)
            return -1;
        if (compare < 0 && insert_identity_right(pool, chain_a, op_type) < 0)
            return -1;
    }
    else
        return 1;

    child_count = zip_chains(
        pool,
        (*pool)->nodes[chain_a].child[0],
        (*pool)->nodes[chain_b].child[0],
        op_type);
    return child_count + 1;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_zip_chains(struct csfg_expr_pool** pool, int chain_a, int chain_b)
{
    CSFG_DEBUG_ASSERT(csfg_expr_is_canonicalized(*pool, chain_a));
    CSFG_DEBUG_ASSERT(csfg_expr_is_canonicalized(*pool, chain_b));
    CSFG_DEBUG_ASSERT(
        (*pool)->nodes[chain_a].type == (*pool)->nodes[chain_b].type);
    return zip_chains(pool, chain_a, chain_b, (*pool)->nodes[chain_a].type);
}
