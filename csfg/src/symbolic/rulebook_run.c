#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>

#define DEBUG_PRINTF 1

VEC_DEFINE(csfg_ruleset_vec, struct csfg_ruleset, 16)
HMAP_DEFINE_STR(extern, csfg_ruleset_hmap, int, 16)

VEC_DECLARE(node_idx_vec, int, 16)
VEC_DEFINE(node_idx_vec, int, 16)

enum match_result
{
    MATCH_OOM           = -1,
    MATCH_NONE          = 0,
    MATCH_FOUND         = 1,
    MATCH_NEXT_ROTATION = 2
};

struct match_info
{
    int var_idx;
    int ruleset_node;
    int target_node;
};

VEC_DECLARE(match_info_vec, struct match_info, 8)
VEC_DEFINE(match_info_vec, struct match_info, 8)

/* -------------------------------------------------------------------------- */
#if DEBUG_PRINTF == 1
#    include <stdio.h>
/* clang-format off */
static int  debug_depth = 0;
static void debug_depth_inc(void)   { debug_depth += 1;}
static void debug_depth_dec(void)   { debug_depth -= 1;}
static void debug_depth_reset(void) { debug_depth  = 0;}
/* clang-format on */
static void debug_print_run_ruleset(
    const struct csfg_expr_pool* pool,
    const struct csfg_ruleset* ruleset,
    int ruleset_idx,
    int is_rerun)
{
    struct str* str;

    int i = debug_depth;
    while (i--)
        fprintf(stderr, " ");

    if (ruleset->expr_search < 0 || ruleset->expr_replace < 0)
    {
        fprintf(stderr, "%03d: Ruleset", ruleset_idx);
        if (is_rerun)
            fprintf(stderr, " (Re-run)");
        fprintf(stderr, "\n");
        return;
    }

    str_init(&str);
    csfg_expr_to_str(&str, pool, ruleset->expr_search);
    fprintf(stderr, "%03d: Trying \"%s\" --> ", ruleset_idx, str_cstr(str));
    str_clear(str);
    csfg_expr_to_str(&str, pool, ruleset->expr_replace);
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
static void debug_print_zip(const struct csfg_expr_pool* pool, int n)
{
    int i = debug_depth;
    while (i--)
        fprintf(stderr, " ");
    fprintf(stderr, "> Zip    : ");
    debug_print_expr(pool, n);
    fprintf(stderr, "\n");
}
static void debug_print_unzip(const struct csfg_expr_pool* pool, int n)
{
    int i = debug_depth;
    while (i--)
        fprintf(stderr, " ");
    fprintf(stderr, "> UnZip  : ");
    debug_print_expr(pool, n);
    fprintf(stderr, "\n");
}
static void debug_print_rotate(const struct csfg_expr_pool* pool, int n)
{
    int i = debug_depth;
    while (i--)
        fprintf(stderr, " ");
    fprintf(stderr, "> Rotate : ");
    debug_print_expr(pool, n);
    fprintf(stderr, "\n");
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
#    define debug_print_run_ruleset(pool, group, group_idx, is_rerun)
#    define debug_print_expr(pool, n)
#    define debug_print_zip(pool, n)
#    define debug_print_rotate(pool, n)
#    define debug_print_replace_subtree_before(pool, n)
#    define debug_print_replace_subtree_after(pool, n)
#endif

/* -------------------------------------------------------------------------- */
static int floats_equal(double f1, double f2, double epsilon)
{
    return fabs(f1 - f2) < epsilon;
}

/* -------------------------------------------------------------------------- */
static int remove_zero_summands(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int left  = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_ADD)
            continue;

        if ((*pool)->nodes[left].type == CSFG_EXPR_LIT &&
            floats_equal((*pool)->nodes[left].value.lit, 0.0, 0.0000001))
        {
            csfg_expr_collapse_into_parent(*pool, right, n);
            modified = 1;
        }
        else if (
            (*pool)->nodes[right].type == CSFG_EXPR_LIT &&
            floats_equal((*pool)->nodes[right].value.lit, 0.0, 0.0000001))
        {
            csfg_expr_collapse_into_parent(*pool, left, n);
            modified = 1;
        }
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
static int remove_one_products(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int c;
        if ((*pool)->nodes[n].type != CSFG_EXPR_MUL)
            continue;

        for (c = 0; c != 2; ++c)
        {
            double value;
            int child   = (*pool)->nodes[n].child[c];
            int sibling = (*pool)->nodes[n].child[1 - c];
            if ((*pool)->nodes[child].type != CSFG_EXPR_LIT)
                continue;

            value = (*pool)->nodes[child].value.lit;
            if (floats_equal(value, 1.0, 0.0000001))
            {
                csfg_expr_collapse_into_parent(*pool, sibling, n);
                modified = 1;
                break;
            }
            else if (floats_equal(value, -1.0, 0.0000001))
            {
                csfg_expr_mark_deleted_shallow(*pool, child);
                csfg_expr_set_neg(pool, n, sibling);
                modified = 1;
                break;
            }
        }
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
static int match_info_add(
    struct match_info_vec** matched_nodes,
    int var_idx,
    int search_expr,
    int target_expr)
{
    struct match_info* match_info = match_info_vec_emplace(matched_nodes);
    if (match_info == NULL)
        return -1;
    match_info->var_idx      = var_idx;
    match_info->ruleset_node = search_expr;
    match_info->target_node  = target_expr;
    return 0;
}

/* -------------------------------------------------------------------------- */
static enum match_result
combine_match_results(enum match_result r1, enum match_result r2)
{
    if (r1 == MATCH_OOM || r2 == MATCH_OOM)
        return MATCH_OOM;

    if (r1 == MATCH_NONE || r2 == MATCH_NONE)
        return MATCH_NONE;

    if (r1 == MATCH_NEXT_ROTATION || r2 == MATCH_NEXT_ROTATION)
        return MATCH_NEXT_ROTATION;

    return MATCH_FOUND;
}

/* -------------------------------------------------------------------------- */
static enum match_result match_subtree(
    struct match_info_vec** matched_nodes,
    const struct csfg_expr_pool* target_pool,
    int target_expr,
    const struct csfg_expr_pool* ruleset_pool,
    int search_expr)
{
    const struct csfg_expr_node* target_node = &target_pool->nodes[target_expr];
    const struct csfg_expr_node* search_node =
        &ruleset_pool->nodes[search_expr];
    enum csfg_expr_type search_node_type = search_node->type;

    switch (search_node_type)
    {
        case CSFG_EXPR_GC: break;

        case CSFG_EXPR_VAR: {
            int i;
            struct match_info* match_info;

            if (match_info_add(
                    matched_nodes,
                    search_node->value.var_idx,
                    search_expr,
                    target_expr) != 0)
                return MATCH_OOM;

            /* We can key on var_idx here instead of having to compare strings,
             * because csfg_expr_pool::var_names guarantees uniqueness. Both
             * structs point into the same varnames strlist */

            /* If the search pattern uses the same variable again, then we must
             * check if the last subtree is the same as this subtree. */
            vec_enumerate (*matched_nodes, i, match_info)
                if (match_info->var_idx == search_node->value.var_idx)
                    if (!csfg_expr_equal(
                            target_pool,
                            target_expr,
                            target_pool,
                            match_info->target_node))
                    {
                        return MATCH_NEXT_ROTATION;
                    }

            return MATCH_FOUND;
        }

        case CSFG_EXPR_LIT: {
            if (target_node->type != search_node->type ||
                target_node->value.lit != search_node->value.lit)
                return MATCH_NONE;
            return MATCH_FOUND;
        }

        case CSFG_EXPR_INF: {
            if (target_pool->nodes[target_expr].type !=
                ruleset_pool->nodes[search_expr].type)
                return MATCH_NONE;
            return MATCH_FOUND;
        }

        case CSFG_EXPR_NEG: {
            if (target_pool->nodes[target_expr].type !=
                ruleset_pool->nodes[search_expr].type)
                return MATCH_NONE;
            return match_subtree(
                matched_nodes,
                target_pool,
                target_node->child[0],
                ruleset_pool,
                search_node->child[0]);
        }

        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW: {
            enum match_result left_result, right_result;

            if (target_pool->nodes[target_expr].type !=
                ruleset_pool->nodes[search_expr].type)
                return MATCH_NONE;

            left_result = match_subtree(
                matched_nodes,
                target_pool,
                target_node->child[0],
                ruleset_pool,
                search_node->child[0]);
            right_result = match_subtree(
                matched_nodes,
                target_pool,
                target_node->child[1],
                ruleset_pool,
                search_node->child[1]);
            return combine_match_results(left_result, right_result);
        }
    }

    CSFG_DEBUG_ASSERT(0);
    return MATCH_NONE;
}

/* -------------------------------------------------------------------------- */
static int zip_related_chains(
    struct csfg_expr_pool** target_pool,
    const struct match_info_vec* matched_nodes)
{
    int i1, i2, chain_len = 0;
    int count = vec_count(matched_nodes);
    for (i1 = 0; i1 != count; ++i1)
        for (i2 = i1 + 1; i2 < count; ++i2)
            if (vec_get(matched_nodes, i1)->var_idx ==
                vec_get(matched_nodes, i2)->var_idx)
            {
                int chain1 = csfg_expr_find_top_of_chain(
                    *target_pool,
                    csfg_expr_find_parent(
                        *target_pool, vec_get(matched_nodes, i1)->target_node));
                int chain2 = csfg_expr_find_top_of_chain(
                    *target_pool,
                    csfg_expr_find_parent(
                        *target_pool, vec_get(matched_nodes, i2)->target_node));
                int new_chain_len =
                    csfg_expr_zip_chains(target_pool, chain1, chain2);
                if (new_chain_len < 0)
                    return -1;
                if (chain_len < new_chain_len)
                    chain_len = new_chain_len;
            }
    return chain_len;
}
static void unzip(struct csfg_expr_pool** target_pool, int* target_expr)
{
    csfg_rules_run(
        target_pool, remove_zero_summands, remove_one_products, NULL);
    *target_expr = csfg_expr_gc(*target_pool, *target_expr);
}

/* -------------------------------------------------------------------------- */
static void rotate_related_chains(
    struct csfg_expr_pool* target_pool,
    const struct match_info_vec* matched_nodes)
{
    int i1, i2;
    int count = vec_count(matched_nodes);
    for (i1 = 0; i1 != count; ++i1)
        for (i2 = i1 + 1; i2 < count; ++i2)
            if (vec_get(matched_nodes, i1)->var_idx ==
                vec_get(matched_nodes, i2)->var_idx)
            {
                int chain1 = csfg_expr_find_top_of_chain(
                    target_pool,
                    csfg_expr_find_parent(
                        target_pool, vec_get(matched_nodes, i1)->target_node));
                int chain2 = csfg_expr_find_top_of_chain(
                    target_pool,
                    csfg_expr_find_parent(
                        target_pool, vec_get(matched_nodes, i2)->target_node));
                csfg_expr_rotate_chain(target_pool, chain1);
                csfg_expr_rotate_chain(target_pool, chain2);
            }
}

/* -------------------------------------------------------------------------- */
static int dup_replace_tree(
    struct csfg_expr_pool** target_pool,
    const struct csfg_expr_pool* ruleset_pool,
    int replace_expr,
    const struct match_info_vec* matched_nodes)
{
    const struct csfg_expr_node* replace_node =
        &ruleset_pool->nodes[replace_expr];

    switch ((enum csfg_expr_type)replace_node->type)
    {
        case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); break;
        case CSFG_EXPR_LIT:
            return csfg_expr_lit(target_pool, replace_node->value.lit);
        case CSFG_EXPR_INF: return csfg_expr_inf(target_pool);
        case CSFG_EXPR_NEG:
            return csfg_expr_neg(
                target_pool,
                dup_replace_tree(
                    target_pool,
                    ruleset_pool,
                    replace_node->child[0],
                    matched_nodes));
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            return csfg_expr_binop(
                target_pool,
                replace_node->type,
                dup_replace_tree(
                    target_pool,
                    ruleset_pool,
                    replace_node->child[0],
                    matched_nodes),
                dup_replace_tree(
                    target_pool,
                    ruleset_pool,
                    replace_node->child[1],
                    matched_nodes));
        case CSFG_EXPR_VAR: {
            const struct match_info* match_info;
            vec_for_each (matched_nodes, match_info)
            {
                int replace_var = replace_node->value.var_idx;
                int match_var   = match_info->var_idx;
                if (strview_eq(
                        strlist_view(ruleset_pool->var_names, replace_var),
                        strlist_view(ruleset_pool->var_names, match_var)))
                {
                    return csfg_expr_dup_recurse(
                        target_pool, match_info->target_node);
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
    int n, i, modified;
    do
    {
        modified = 0;
        for (n = 0; n != csfg_expr_pool_count(pool); ++n)
            for (i = 0; i != 2; ++i)
            {
                int child   = pool->nodes[n].child[i];
                int sibling = pool->nodes[n].child[1 - i];
                if (child > -1 && pool->nodes[n].type != CSFG_EXPR_GC &&
                    pool->nodes[child].type == CSFG_EXPR_GC)
                {
                    if (sibling > -1)
                    {
                        pool->nodes[n] = pool->nodes[sibling];
                        csfg_expr_mark_deleted_shallow(pool, sibling);
                    }
                    else
                        csfg_expr_mark_deleted_shallow(pool, n);
                    modified = 1;
                }
            }
    } while (modified);
}

/* -------------------------------------------------------------------------- */
static int replace_subtree(
    struct csfg_expr_pool** target_pool,
    int target_expr,
    const struct csfg_expr_pool* replace_pool,
    int replace_expr,
    const struct match_info_vec* matched_nodes)
{
    int replace_expr_dup = dup_replace_tree(
        target_pool, replace_pool, replace_expr, matched_nodes);
    if (replace_expr_dup < 0)
        return -1;

    csfg_expr_mark_deleted_recursive(*target_pool, target_expr);
    (*target_pool)->nodes[target_expr] =
        (*target_pool)->nodes[replace_expr_dup];
    csfg_expr_mark_deleted_shallow(*target_pool, replace_expr_dup);

    return 0;
}

/* -------------------------------------------------------------------------- */
static int run_ruleset(
    struct csfg_expr_pool** target_pool,
    int* expr,
    struct match_info_vec** matched_nodes,
    const struct csfg_expr_pool* ruleset_pool,
    const struct csfg_ruleset* ruleset)
{
    int n, rotation, modified = 0;

    if (ruleset->extern_run != NULL)
    {
        switch (ruleset->extern_run(target_pool))
        {
            case -1: return -1;
            case 0 : break;
            case 1 : modified = 1; break;
        }
        *expr = csfg_expr_gc(*target_pool, *expr);
    }
    /* rulesets can be empty containers for child rulesets */
    else if (ruleset->expr_search > -1 && ruleset->expr_replace > -1)
    {
        for (n = 0; n < vec_count(*target_pool); ++n)
        {
            rotation = -1;
        next_rotation:
            match_info_vec_clear(*matched_nodes);
            switch (match_subtree(
                matched_nodes,
                *target_pool,
                n,
                ruleset_pool,
                ruleset->expr_search))
            {
                case MATCH_OOM: return -1;
                case MATCH_NONE:
                    /* This rotates the chains back to their original state, so
                     * we can preserve ordering for the next match */
                    if (rotation > 0)
                    {
                        while (rotation-- > 0)
                            rotate_related_chains(*target_pool, *matched_nodes);
                        unzip(target_pool, expr);
                        debug_print_unzip(*target_pool, *expr);
                    }
                    continue;
                case MATCH_FOUND: break;
                case MATCH_NEXT_ROTATION:
                    if (rotation == -1)
                    {
                        rotation =
                            zip_related_chains(target_pool, *matched_nodes);
                        if (rotation < 0)
                            return -1;
                        debug_print_zip(*target_pool, *expr);
                    }
                    if (rotation-- > 0)
                    {
                        rotate_related_chains(*target_pool, *matched_nodes);
                        debug_print_rotate(*target_pool, *expr);
                        goto next_rotation;
                    }
                    unzip(target_pool, expr);
                    debug_print_unzip(*target_pool, *expr);
                    continue;
            }

            debug_print_replace_subtree_before(*target_pool, *expr);
            if (replace_subtree(
                    target_pool,
                    n,
                    ruleset_pool,
                    ruleset->expr_replace,
                    *matched_nodes) != 0)
                return -1;
            debug_print_replace_subtree_after(*target_pool, *expr);

            if (rotation > 0)
            {
                csfg_rules_run(
                    target_pool,
                    remove_zero_summands,
                    remove_one_products,
                    NULL);
            }
            csfg_expr_canonicalize(*target_pool, *expr);
            *expr = csfg_expr_gc(*target_pool, *expr);
            if (rotation > 0)
                debug_print_unzip(*target_pool, *expr);

            modified = 1;
        }
    }

    return ruleset->ignore ? 0 : modified;
}

/* -------------------------------------------------------------------------- */
static int process_ruleset_recurse(
    struct csfg_expr_pool** pool,
    int* expr,
    struct match_info_vec** matched_nodes,
    const struct csfg_rulebook* book,
    int ruleset_idx)
{
    const struct csfg_ruleset* ruleset;
    int modified = 0;

    debug_depth_inc();

    for (; ruleset_idx != -1; ruleset_idx = ruleset->next)
    {
        ruleset = vec_get(book->rulesets, ruleset_idx);

        debug_print_run_ruleset(book->pool, ruleset, ruleset_idx, 0);
        switch (run_ruleset(pool, expr, matched_nodes, book->pool, ruleset))
        {
            case -1: return -1;
            case 0 : break;
            case 1:
                if (!ruleset->ignore)
                    modified = 1;
                break;
        }

        while (1)
        {
            switch (process_ruleset_recurse(
                pool, expr, matched_nodes, book, ruleset->child))
            {
                case -1: return -1;
                case 0 : break;
                case 1:
                    if (!ruleset->ignore)
                        modified = 1;

                    /* Don't need to re-run group if there is only one child,
                     * because the child will have already re-run itself  */
                    if (vec_get(book->rulesets, ruleset->child)->next == -1)
                        break;

                    debug_print_run_ruleset(
                        book->pool, ruleset, ruleset_idx, 1);
                    continue;
            }
            break;
        }
    }

    debug_depth_dec();

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_rulebook_run(
    const struct csfg_rulebook* book,
    const char* name,
    struct csfg_expr_pool** pool,
    int* expr)
{
    struct match_info_vec* matched_nodes;
    int* ruleset_idx;
    int modified;

    CSFG_DEBUG_ASSERT(expr != NULL);
    CSFG_DEBUG_ASSERT(csfg_expr_is_canonicalized(*pool, *expr));

    match_info_vec_init(&matched_nodes);
    debug_depth_reset();

    ruleset_idx = csfg_ruleset_hmap_find(book->ruleset_map, cstr_view(name));
    if (ruleset_idx == NULL)
        goto fail;

    modified = 0;
    while (1)
    {
#if DEBUG_PRINTF == 1
        fprintf(stderr, "Input: ");
        debug_print_expr(*pool, *expr);
        fprintf(stderr, "\n");
#endif
        switch (process_ruleset_recurse(
            pool, expr, &matched_nodes, book, *ruleset_idx))
        {
            case -1: goto fail;
            case 0 : break;
            case 1 : modified = 1; continue;
        }
        break;
    }

    csfg_rules_run(pool, remove_zero_summands, remove_one_products, NULL);
    *expr = csfg_expr_gc(*pool, *expr);

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
