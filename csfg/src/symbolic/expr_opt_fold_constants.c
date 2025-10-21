#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"

/* ------------------------------------------------------------------------- */
static int eval_subtrees(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
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
            modified = 1;
        }

        /* unary operator */
        else if (
            right == -1 && left != -1 &&
            (*pool)->nodes[left].type == CSFG_EXPR_LIT)
        {
            csfg_expr_set_lit(pool, n, csfg_expr_eval(*pool, n, NULL));
            csfg_expr_mark_deleted_recursive(*pool, left);
            modified = 1;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
static int find_other_constant_operand(
    const struct csfg_expr_pool* pool,
    enum csfg_expr_type          type,
    int                          n,
    int                          exclude)
{
    int left, right, result;

    if (n == exclude || n == -1)
        return -1;
    if (pool->nodes[n].type == CSFG_EXPR_LIT)
        return n;
    if (pool->nodes[n].type != type)
        return -1;

    left = pool->nodes[n].child[0];
    right = pool->nodes[n].child[1];

    result = find_other_constant_operand(pool, type, left, exclude);
    if (result > -1)
        return result;
    result = find_other_constant_operand(pool, type, right, exclude);
    if (result > -1)
        return result;

    return -1;
}

/* ------------------------------------------------------------------------- */
static int combine_constants(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        enum csfg_expr_type op_type = (*pool)->nodes[n].type;
        int                 left = (*pool)->nodes[n].child[0];
        int                 right = (*pool)->nodes[n].child[1];
        int                 constant, top, match;
        double              combined_value;

        if (op_type != CSFG_EXPR_ADD && op_type != CSFG_EXPR_MUL)
            continue;

        if ((*pool)->nodes[left].type == CSFG_EXPR_LIT)
            constant = left;
        else if ((*pool)->nodes[right].type == CSFG_EXPR_LIT)
            constant = right;
        else
            continue; /* No child is constant */

        top = n;
        while (1)
        {
            match = find_other_constant_operand(*pool, op_type, top, constant);
            if (match > -1)
                break;
            top = csfg_expr_find_parent(*pool, top);
            if (top == -1 || (*pool)->nodes[top].type != op_type)
                break;
        }
        if (match == -1)
            continue;

        if (op_type == CSFG_EXPR_ADD)
            combined_value = (*pool)->nodes[constant].value.lit +
                             (*pool)->nodes[match].value.lit;
        else
            combined_value = (*pool)->nodes[constant].value.lit *
                             (*pool)->nodes[match].value.lit;
        csfg_expr_set_lit(pool, constant, combined_value);
        csfg_expr_collapse_sibling_into_parent(*pool, match);
        modified = 1;
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_opt_fold_constants(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run(pool, eval_subtrees, combine_constants, NULL);
}
