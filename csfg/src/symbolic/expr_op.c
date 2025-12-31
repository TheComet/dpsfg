#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/util/bmap.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

VEC_DEFINE(csfg_expr_op_group_vec, struct csfg_expr_op_group, 16)
HMAP_DEFINE_STR(extern, csfg_expr_op_hmap, int, 16)

struct match_info
{
    int pattern_node;
    int target_node;
};

BMAP_DECLARE(match_info_bmap, int, struct match_info, 8)
BMAP_DEFINE(match_info_bmap, int, struct match_info, 8)

/* -------------------------------------------------------------------------- */
int csfg_expr_op_run_pass(
    struct csfg_expr_pool** pool, csfg_expr_pass_func pass)
{
    int modified = 0;
again:
    switch (pass(pool))
    {
        case -1: return -1;
        case 0: break;
        case 1:
            modified = 1;
            CSFG_DEBUG_ASSERT(
                csfg_expr_integrity_check_allow_islands(*pool) == 0);
            goto again;
    }
    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_runv(struct csfg_expr_pool** pool, va_list ap)
{
    csfg_expr_pass_func pass;
    int                 pass_modified, modified;
    va_list             copy;

    modified = 0;
again:
    pass_modified = 0;
    va_copy(copy, ap);
    while (1)
    {
        pass = va_arg(copy, csfg_expr_pass_func);
        if (pass == NULL)
            break;
        switch (pass(pool))
        {
            case -1: return -1;
            case 0: break;
            case 1:
                pass_modified = 1;
                modified = 1;
                CSFG_DEBUG_ASSERT(
                    csfg_expr_integrity_check_allow_islands(*pool) == 0);
                break;
        }
    }
    va_end(copy);

    if (pass_modified)
        goto again;

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_run(struct csfg_expr_pool** pool, ...)
{
    int     result;
    va_list ap;
    va_start(ap, pool);
    result = csfg_expr_op_runv(pool, ap);
    va_end(ap);
    return result;
}

/* -------------------------------------------------------------------------- */
void csfg_expr_op_init(struct csfg_expr_op* op)
{
    csfg_expr_op_hmap_init(&op->group_map);
    csfg_expr_pool_init(&op->pool);
    csfg_expr_op_group_vec_init(&op->groups);
}

/* -------------------------------------------------------------------------- */
void csfg_expr_op_deinit(struct csfg_expr_op* op)
{
    csfg_expr_op_group_vec_deinit(op->groups);
    csfg_expr_pool_deinit(op->pool);
    csfg_expr_op_hmap_deinit(op->group_map);
}

/* -------------------------------------------------------------------------- */
static int match_subtree(
    struct match_info_bmap**     matched_nodes,
    const struct csfg_expr_pool* pool,
    int                          n,
    const struct csfg_expr_pool* pattern_pool,
    int                          pattern_n)
{
    struct match_info*           match_info;
    const struct csfg_expr_node* node = &pool->nodes[n];
    const struct csfg_expr_node* pattern_node = &pattern_pool->nodes[pattern_n];

    switch (pattern_node->type)
    {
        case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); break;

        case CSFG_EXPR_VAR: {
            switch (match_info_bmap_emplace_or_get(
                matched_nodes, pattern_node->value.var_idx, &match_info))
            {
                case BMAP_OOM: return -1;
                case BMAP_NEW:
                    match_info->pattern_node = pattern_n;
                    match_info->target_node = n;
                    return 1;
                case BMAP_EXISTS:
                    if (csfg_expr_structurally_mathematically_equivalent(
                            pool, n, pool, match_info->target_node))
                    {
                        return 1;
                    }
                    return 0;
            }
            break;
        }

        case CSFG_EXPR_LIT: {
            if (node->type != pattern_node->type ||
                node->value.lit != pattern_node->value.lit)
                return 0;
            return 1;
        }

        case CSFG_EXPR_INF: {
            if (pool->nodes[n].type != pattern_pool->nodes[pattern_n].type)
                return 0;
            return 1;
        }

        case CSFG_EXPR_NEG: {
            if (pool->nodes[n].type != pattern_pool->nodes[pattern_n].type)
                return 0;
            return match_subtree(
                matched_nodes,
                pool,
                node->child[0],
                pattern_pool,
                pattern_node->child[0]);
        }

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW: {
            int left_result, right_result;
            if (pool->nodes[n].type != pattern_pool->nodes[pattern_n].type)
                return 0;

            left_result = match_subtree(
                matched_nodes,
                pool,
                node->child[0],
                pattern_pool,
                pattern_node->child[0]);
            right_result = match_subtree(
                matched_nodes,
                pool,
                node->child[1],
                pattern_pool,
                pattern_node->child[1]);
            if (left_result == 1 && right_result == 1)
                return 1;
            if (left_result < 0 || right_result < 0)
                return -1;
            return 0;
        }
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
static int dup_subtree(
    struct csfg_expr_pool**       pool,
    const struct csfg_expr_pool*  pattern_pool,
    int                           pattern_n,
    const struct match_info_bmap* matched_nodes)
{
    const struct csfg_expr_node* pattern_node = &pattern_pool->nodes[pattern_n];

    switch (pattern_node->type)
    {
        case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); break;
        case CSFG_EXPR_LIT: return csfg_expr_lit(pool, pattern_node->value.lit);
        case CSFG_EXPR_INF: return csfg_expr_inf(pool);
        case CSFG_EXPR_NEG:
            return csfg_expr_neg(
                pool,
                dup_subtree(
                    pool, pattern_pool, pattern_node->child[0], matched_nodes));
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            return csfg_expr_binop(
                pool,
                pattern_node->type,
                dup_subtree(
                    pool, pattern_pool, pattern_node->child[0], matched_nodes),
                dup_subtree(
                    pool, pattern_pool, pattern_node->child[1], matched_nodes));
        case CSFG_EXPR_VAR: {
            int                      idx, var_idx;
            const struct match_info* match_info;
            bmap_for_each (matched_nodes, idx, var_idx, match_info)
            {
                if (strview_eq(
                        strlist_view(pattern_pool->var_names, var_idx),
                        strlist_view(
                            pattern_pool->var_names,
                            pattern_node->value.var_idx)))
                {
                    return csfg_expr_dup_recurse(pool, match_info->target_node);
                }
            }
            CSFG_DEBUG_ASSERT(0);
            return -1;
        }
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
static int replace_subtree(
    struct csfg_expr_pool**       pool,
    int                           n,
    const struct csfg_expr_pool*  pattern_pool,
    int                           pattern_n,
    const struct match_info_bmap* matched_nodes)
{
    int replace_n = dup_subtree(pool, pattern_pool, pattern_n, matched_nodes);
    if (replace_n < 0)
        return -1;
    csfg_expr_mark_deleted_recursive(*pool, n);
    (*pool)->nodes[n] = (*pool)->nodes[replace_n];
    csfg_expr_mark_deleted(*pool, replace_n);
    return 0;
}

/* -------------------------------------------------------------------------- */
static int run_group(
    struct csfg_expr_pool**          pool,
    int*                             expr,
    struct match_info_bmap**         matched_nodes,
    const struct csfg_expr_pool*     pattern_pool,
    const struct csfg_expr_op_group* group)
{
    int modified = 0;
    if (group->extern_pass != NULL)
    {
        switch (group->extern_pass(pool))
        {
            case -1: return -1;
            case 0: break;
            case 1: modified = 1; break;
        }
        *expr = csfg_expr_gc(*pool, *expr);
    }
    else if (group->expr_from > -1 && group->expr_to > -1)
    {
        int n;
        for (n = 0; n < vec_count(*pool); ++n)
        {
            match_info_bmap_clear(*matched_nodes);
            switch (match_subtree(
                matched_nodes, *pool, n, pattern_pool, group->expr_from))
            {
                case -1: return -1;
                case 0: continue;
                case 1: break;
            }
            if (replace_subtree(
                    pool, n, pattern_pool, group->expr_to, *matched_nodes) != 0)
                return -1;
            modified = 1;
        }
        *expr = csfg_expr_gc(*pool, *expr);
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
static int process_group_recurse(
    struct csfg_expr_pool**    pool,
    int*                       expr,
    struct match_info_bmap**   matched_nodes,
    const struct csfg_expr_op* op,
    int                        group_idx)
{
    const struct csfg_expr_op_group* group;
    int                              modified = 0;

    for (; group_idx != -1; group_idx = group->next)
    {
        group = vec_get(op->groups, group_idx);

        switch (run_group(pool, expr, matched_nodes, op->pool, group))
        {
            case -1: return -1;
            case 0: break;
            case 1: modified = 1; break;
        }

        switch (
            process_group_recurse(pool, expr, matched_nodes, op, group->child))
        {
            case -1: return -1;
            case 0: break;
            case 1: modified = 1;
        }
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_run_def(
    struct csfg_expr_pool**    pool,
    int*                       expr,
    const struct csfg_expr_op* op,
    const char*                name)
{
    struct match_info_bmap* matched_nodes;
    int*                    group;
    int                     modified;

    match_info_bmap_init(&matched_nodes);

    group = csfg_expr_op_hmap_find(op->group_map, cstr_view(name));
    if (group == NULL)
        goto fail;

    modified = 0;
    while (1)
    {
        switch (process_group_recurse(pool, expr, &matched_nodes, op, *group))
        {
            case -1: goto fail;
            case 0: break;
            case 1: modified = 1; continue;
        }
        break;
    }

    match_info_bmap_deinit(matched_nodes);
    return modified;

fail:
    match_info_bmap_deinit(matched_nodes);
    return -1;
}
