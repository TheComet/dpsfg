#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_opt.h"

/* ------------------------------------------------------------------------- */
static int eval_subtrees(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int left = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];

        if ((*pool)->nodes[n].type == CSFG_EXPR_GC)
            continue;

        /* binary operator */
        if (left != -1 && right != -1 &&
            (*pool)->nodes[left].type == CSFG_EXPR_LIT &&
            (*pool)->nodes[right].type == CSFG_EXPR_LIT)
        {
            csfg_expr_set_lit(pool, n, csfg_expr_eval(*pool, n, NULL));
            csfg_expr_mark_deleted_recursive(*pool, left);
            csfg_expr_mark_deleted_recursive(*pool, right);
            return 1;
        }

        /* unary operator */
        if (right == -1 && left != -1 &&
            (*pool)->nodes[left].type == CSFG_EXPR_LIT)
        {
            csfg_expr_set_lit(pool, n, csfg_expr_eval(*pool, n, NULL));
            csfg_expr_mark_deleted_recursive(*pool, left);
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int find_constant_operand(
    const struct csfg_expr_pool* pool, int n, enum csfg_expr_type type)
{
    int child, result;

    if (n == -1)
        return -1;
    if (pool->nodes[n].type == CSFG_EXPR_LIT)
        return n;
    if (pool->nodes[n].type != type)
        return -1;

    result = find_constant_operand(pool, pool->nodes[n].child[0], type);
    if (result > -1)
        return result;
    result = find_constant_operand(pool, pool->nodes[n].child[1], type);
    if (result > -1)
        return result;

    return -1;
}
static int combine_constants(struct csfg_expr_pool** pool)
{
    int n;
    for (n = 0; n != (*pool)->count; ++n)
    {
        enum csfg_expr_type type = (*pool)->nodes[n].type;
        int                 left = (*pool)->nodes[n].child[0];
        int                 right = (*pool)->nodes[n].child[1];
        int                 constant, subtree, child_constant;
        double              combined_value;

        if (type != CSFG_EXPR_OP_ADD && type != CSFG_EXPR_OP_MUL)
            continue;

        /* Select which child is the literal and which is the subtree */
        if ((*pool)->nodes[left].type == CSFG_EXPR_LIT)
            constant = left, subtree = right;
        else if ((*pool)->nodes[right].type == CSFG_EXPR_LIT)
            constant = right, subtree = left;
        else
            continue; /* No child is constant */

        child_constant = find_constant_operand(*pool, subtree, type);
        if (child_constant == -1)
            continue;

        if (type == CSFG_EXPR_OP_ADD)
            combined_value = (*pool)->nodes[constant].value.lit +
                             (*pool)->nodes[child_constant].value.lit;
        else
            combined_value = (*pool)->nodes[constant].value.lit *
                             (*pool)->nodes[child_constant].value.lit;
        csfg_expr_set_lit(pool, child_constant, combined_value);
        csfg_expr_collapse_into_parent(*pool, subtree, n);
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_opt_fold_constants(struct csfg_expr_pool** pool)
{
#define RUN(func, pool, modified)                                              \
    switch (func(pool))                                                        \
    {                                                                          \
        case -1: return -1;                                                    \
        case 0: break;                                                         \
        case 1: modified = 1; break;                                           \
    }
    int modified = 0;
    RUN(eval_subtrees, pool, modified);
    RUN(combine_constants, pool, modified);
    return modified;
#undef RUN
}
