#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define DEBUG_PRINTF 1

VEC_DEFINE(csfg_expr_op_group_vec, struct csfg_expr_op_group, 16)
HMAP_DEFINE_STR(extern, csfg_expr_op_hmap, int, 16)

VEC_DECLARE(node_idx_vec, int, 16)
VEC_DEFINE(node_idx_vec, int, 16)

struct match_info
{
    int var_idx;
    int pattern_node;
    int target_node;
};

VEC_DECLARE(match_info_vec, struct match_info, 8)
VEC_DEFINE(match_info_vec, struct match_info, 8)

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
static int find_all_commutative_operands(
    struct node_idx_vec** chain_idxs, const struct csfg_expr_pool* pool, int n)
{
    int i;
    for (i = 0; i != 2; ++i)
    {
        int child = pool->nodes[n].child[i];
        if (pool->nodes[child].type != pool->nodes[n].type)
        {
            if (node_idx_vec_push(chain_idxs, child) != 0)
                return -1;
            continue;
        }

        if (find_all_commutative_operands(chain_idxs, pool, child) != 0)
            return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static void undo_match_info_entries(
    struct match_info_vec*       matched_nodes,
    const struct csfg_expr_pool* pool,
    int                          n)
{
    switch (pool->nodes[n].type)
    {
        case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); break;
        case CSFG_EXPR_LIT: break;
        case CSFG_EXPR_VAR: {
            int                      i;
            const struct match_info* match_info;
            vec_enumerate (matched_nodes, i, match_info)
                if (match_info->pattern_node == n)
                {
                    match_info_vec_erase(matched_nodes, i);
                    break;
                }
            break;
        }
        case CSFG_EXPR_INF: break;
        case CSFG_EXPR_NEG:
            undo_match_info_entries(
                matched_nodes, pool, pool->nodes[n].child[0]);
            break;
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            undo_match_info_entries(
                matched_nodes, pool, pool->nodes[n].child[0]);
            undo_match_info_entries(
                matched_nodes, pool, pool->nodes[n].child[1]);
            break;
    }
}

/* -------------------------------------------------------------------------- */
static int match_subtree(
    struct match_info_vec**      matched_nodes,
    const struct csfg_expr_pool* pool,
    int                          n,
    const struct csfg_expr_pool* pattern_pool,
    int                          pattern_n);
static int match_pattern(
    struct match_info_vec**      matched_nodes,
    const struct csfg_expr_pool* pool,
    const struct csfg_expr_pool* pattern_pool,
    const struct node_idx_vec*   pattern_operands,
    struct node_idx_vec*         target_operands,
    int                          pattern_operand_idx)
{
    int target_operand_idx;

    if (pattern_operand_idx == vec_count(pattern_operands))
        return 1; /* all pattern elements matched */

    for (target_operand_idx = 0;
         target_operand_idx != vec_count(target_operands);
         ++target_operand_idx)
    {
        if (*vec_get(target_operands, target_operand_idx) < 0)
            continue;

        if (match_subtree(
                matched_nodes,
                pool,
                *vec_get(target_operands, target_operand_idx),
                pattern_pool,
                *vec_get(pattern_operands, pattern_operand_idx)) == 1)
        {
            *vec_get(target_operands, target_operand_idx) =
                -1 - *vec_get(target_operands, target_operand_idx);

            if (match_pattern(
                    matched_nodes,
                    pool,
                    pattern_pool,
                    pattern_operands,
                    target_operands,
                    pattern_operand_idx + 1) == 1)
            {
                return 1;
            }

            *vec_get(target_operands, target_operand_idx) =
                -1 - *vec_get(target_operands, target_operand_idx);
        }

        /* backtrack */
        undo_match_info_entries(
            *matched_nodes,
            pattern_pool,
            *vec_get(pattern_operands, pattern_operand_idx));
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int match_subtree(
    struct match_info_vec**      matched_nodes,
    const struct csfg_expr_pool* pool,
    int                          n,
    const struct csfg_expr_pool* pattern_pool,
    int                          pattern_n)
{
    const struct csfg_expr_node* node = &pool->nodes[n];
    const struct csfg_expr_node* pattern_node = &pattern_pool->nodes[pattern_n];

    switch (pattern_node->type)
    {
        case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); break;

        case CSFG_EXPR_VAR: {
            int                i;
            struct match_info* match_info;

            /* We can key on var_idx here instead of having to compare strings,
             * because csfg_expr_pool::var_names guarantees uniqueness. */
            vec_enumerate (*matched_nodes, i, match_info)
                if (match_info->var_idx == pattern_node->value.var_idx)
                    if (csfg_expr_mathematically_equivalent(
                            pool, n, pool, match_info->target_node))
                    {
                        match_info = match_info_vec_emplace(matched_nodes);
                        if (match_info == NULL)
                            return -1;
                        match_info->var_idx = pattern_node->value.var_idx;
                        match_info->pattern_node = pattern_n;
                        match_info->target_node = n;
                        return 1;
                    }

            /* Don't want to match if a different symbol already matched the
             * current node */
            vec_enumerate (*matched_nodes, i, match_info)
                if (csfg_expr_mathematically_equivalent(
                        pool, n, pool, match_info->target_node))
                    return 0;

            match_info = match_info_vec_emplace(matched_nodes);
            if (match_info == NULL)
                return -1;
            match_info->var_idx = pattern_node->value.var_idx;
            match_info->pattern_node = pattern_n;
            match_info->target_node = n;

            return 1;
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
        case CSFG_EXPR_MUL: {
            struct node_idx_vec* pattern_operands;
            struct node_idx_vec* target_operands;

            if (pool->nodes[n].type != pattern_pool->nodes[pattern_n].type)
                return 0;

            node_idx_vec_init(&pattern_operands);
            node_idx_vec_init(&target_operands);

            if (find_all_commutative_operands(
                    &pattern_operands, pattern_pool, pattern_n) != 0)
                goto fail;
            if (find_all_commutative_operands(&target_operands, pool, n) != 0)
                goto fail;

            switch (match_pattern(
                matched_nodes,
                pool,
                pattern_pool,
                pattern_operands,
                target_operands,
                0))
            {
                case -1: goto fail;
                case 0: break;
                case 1: goto success;
            }

            node_idx_vec_deinit(target_operands);
            node_idx_vec_deinit(pattern_operands);
            return 0;
        success:
            node_idx_vec_deinit(target_operands);
            node_idx_vec_deinit(pattern_operands);
            return 1;
        fail:
            node_idx_vec_deinit(target_operands);
            node_idx_vec_deinit(pattern_operands);
            return -1;
        }

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
    struct csfg_expr_pool**      pool,
    const struct csfg_expr_pool* pattern_pool,
    int                          pattern_n,
    const struct match_info_vec* matched_nodes)
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
            const struct match_info* match_info;
            vec_for_each (matched_nodes, match_info)
            {
                int pattern_var = pattern_node->value.var_idx;
                int match_var = match_info->var_idx;
                if (strview_eq(
                        strlist_view(pattern_pool->var_names, pattern_var),
                        strlist_view(pattern_pool->var_names, match_var)))
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
static void collapse_gc_nodes(struct csfg_expr_pool* pool)
{
    int n, i;
again:
    for (n = 0; n != csfg_expr_pool_count(pool); ++n)
        for (i = 0; i != 2; ++i)
        {
            int child = pool->nodes[n].child[i];
            int sibling = pool->nodes[n].child[1 - i];
            if (child > -1 && pool->nodes[n].type != CSFG_EXPR_GC &&
                pool->nodes[child].type == CSFG_EXPR_GC)
            {
                if (sibling > -1)
                {
                    pool->nodes[n] = pool->nodes[sibling];
                    csfg_expr_mark_deleted(pool, sibling);
                }
                else
                    csfg_expr_mark_deleted(pool, n);
                goto again;
            }
        }
}

/* -------------------------------------------------------------------------- */
static int replace_subtree(
    struct csfg_expr_pool**      pool,
    int                          n,
    const struct csfg_expr_pool* pattern_pool,
    int                          pattern_n,
    const struct match_info_vec* matched_nodes)
{
    int                      replace_n;
    const struct match_info* match_info;
    enum csfg_expr_type      type = (*pool)->nodes[n].type;

    replace_n = dup_subtree(pool, pattern_pool, pattern_n, matched_nodes);
    if (replace_n < 0)
        return -1;

    vec_for_each (matched_nodes, match_info)
        csfg_expr_mark_deleted_recursive(*pool, match_info->target_node);

    switch (type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF: CSFG_DEBUG_ASSERT(0); return -1;

        case CSFG_EXPR_NEG: CSFG_DEBUG_ASSERT(0); return -1;

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            csfg_expr_set_binop(
                *pool, n, type, replace_n, csfg_expr_dup_single(pool, n));
            break;
    }

    collapse_gc_nodes(*pool);

    return 0;
}

/* -------------------------------------------------------------------------- */
#if DEBUG_PRINTF == 1
/* clang-format off */
static int  debug_depth = 0;
static void debug_depth_inc(void)   { debug_depth += 1;}
static void debug_depth_dec(void)   { debug_depth -= 1;}
static void debug_depth_reset(void) { debug_depth  = 0;}
/* clang-format on */
static void debug_print_run_group(
    const struct csfg_expr_pool*     pool,
    const struct csfg_expr_op_group* group,
    int                              group_idx,
    int                              is_rerun)
{
    struct str* str;

    int i = debug_depth;
    while (i--)
        fprintf(stderr, " ");

    if (group->expr_from < 0 || group->expr_to < 0)
    {
        fprintf(stderr, "%03d: Pseudo-group", group_idx);
        if (is_rerun)
            fprintf(stderr, " (Re-run)");
        fprintf(stderr, "\n");
        return;
    }

    str_init(&str);
    csfg_expr_to_str(&str, pool, group->expr_from);
    fprintf(stderr, "%03d: Trying \"%s\" --> ", group_idx, str_cstr(str));
    str_clear(str);
    csfg_expr_to_str(&str, pool, group->expr_to);
    fprintf(stderr, "\"%s\"", str_cstr(str));
    if (is_rerun)
        fprintf(stderr, " (Re-run)");
    fprintf(stderr, "\n");
    str_deinit(str);
}
static void debug_print_expr(const struct csfg_expr_pool* pool, int n)
{
    struct str* str;
    str_init(&str);
    csfg_expr_to_str(&str, pool, n);
    fprintf(stderr, "\"%s\"", str_cstr(str));
    str_deinit(str);
}
static void
debug_print_replace_subtree_before(const struct csfg_expr_pool* pool, int n)
{
    int i = debug_depth;
    while (i--)
        fprintf(stderr, " ");
    fprintf(stderr, "> Replace: ");
    debug_print_expr(pool, n);
    fprintf(stderr, "\n");
}
static void
debug_print_replace_subtree_after(const struct csfg_expr_pool* pool, int n)
{
    int i = debug_depth;
    while (i--)
        fprintf(stderr, " ");
    fprintf(stderr, "> With   : ");
    debug_print_expr(pool, n);
    fprintf(stderr, "\n");
}
#else
#    define debug_depth_inc()
#    define debug_depth_dec()
#    define debug_depth_reset()
#    define debug_print_run_group(pool, group, group_idx, is_rerun)
#    define debug_print_expr(pool, n)
#    define debug_print_replace_subtree_before(pool, n)
#    define debug_print_replace_subtree_after(pool, n)
#endif

/* -------------------------------------------------------------------------- */
static int run_group(
    struct csfg_expr_pool**          pool,
    int*                             expr,
    struct match_info_vec**          matched_nodes,
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
            match_info_vec_clear(*matched_nodes);
            switch (match_subtree(
                matched_nodes, *pool, n, pattern_pool, group->expr_from))
            {
                case -1: return -1;
                case 0: continue;
                case 1: break;
            }
            debug_print_replace_subtree_before(*pool, *expr);
            if (replace_subtree(
                    pool, n, pattern_pool, group->expr_to, *matched_nodes) != 0)
                return -1;
            *expr = csfg_expr_gc(*pool, *expr);
            debug_print_replace_subtree_after(*pool, *expr);
            modified = 1;
        }
    }

    return group->ignore ? 0 : modified;
}

/* -------------------------------------------------------------------------- */
static int process_group_recurse(
    struct csfg_expr_pool**    pool,
    int*                       expr,
    struct match_info_vec**    matched_nodes,
    const struct csfg_expr_op* op,
    int                        group_idx)
{
    const struct csfg_expr_op_group* group;
    int                              modified = 0;

    debug_depth_inc();

    for (; group_idx != -1; group_idx = group->next)
    {
        group = vec_get(op->groups, group_idx);

        debug_print_run_group(op->pool, group, group_idx, 0);
        switch (run_group(pool, expr, matched_nodes, op->pool, group))
        {
            case -1: return -1;
            case 0: break;
            case 1:
                if (!group->ignore)
                    modified = 1;
                break;
        }

        while (1)
        {
            switch (process_group_recurse(
                pool, expr, matched_nodes, op, group->child))
            {
                case -1: return -1;
                case 0: break;
                case 1:
                    if (!group->ignore)
                        modified = 1;

                    /* Don't need to re-run group if there is only one child,
                     * because the child will have already re-run itself  */
                    if (vec_get(op->groups, group->child)->next == -1)
                        break;

                    debug_print_run_group(op->pool, group, group_idx, 1);
                    continue;
            }
            break;
        }
    }

    debug_depth_dec();

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_run_def(
    struct csfg_expr_pool**    pool,
    int*                       expr,
    const struct csfg_expr_op* op,
    const char*                name)
{
    struct match_info_vec* matched_nodes;
    int*                   group;
    int                    modified;

    match_info_vec_init(&matched_nodes);
    debug_depth_reset();

    group = csfg_expr_op_hmap_find(op->group_map, cstr_view(name));
    if (group == NULL)
        goto fail;

    modified = 0;
    while (1)
    {
#if DEBUG_PRINTF == 1
        fprintf(stderr, "Input: ");
        debug_print_expr(*pool, *expr);
        fprintf(stderr, "\n");
#endif
        switch (process_group_recurse(pool, expr, &matched_nodes, op, *group))
        {
            case -1: goto fail;
            case 0: break;
            case 1: modified = 1; continue;
        }
        break;
    }
#if DEBUG_PRINTF == 1
    fprintf(stderr, "Result: ");
    debug_print_expr(*pool, *expr);
    fprintf(stderr, "\n");
#endif

    match_info_vec_deinit(matched_nodes);
    return modified;

fail:
    match_info_vec_deinit(matched_nodes);
    return -1;
}
