#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* -------------------------------------------------------------------------- */
/* Searches the subtree "n" recursively until it matches the node "search". The
 * returned node can either be the node that matched, or if the matching subtree
 * already has a constant product, returns the product operator where one of the
 * operands is the matching node. */
static int find_same_expr(const struct csfg_expr_pool* pool, int n, int search)
{
    if (pool->nodes[n].type == CSFG_EXPR_ADD)
    {
        int left = pool->nodes[n].child[0];
        int right = pool->nodes[n].child[1];
        n = find_same_expr(pool, left, search);
        if (n > -1)
            return n;
        n = find_same_expr(pool, right, search);
        if (n > -1)
            return n;
        return -1;
    }

    if (pool->nodes[n].type == CSFG_EXPR_MUL &&
        pool->nodes[pool->nodes[n].child[1]].type == CSFG_EXPR_LIT)
    {
        int prod1 = pool->nodes[n].child[0];
        int prod2 = pool->nodes[n].child[0];
        if (search != prod1 && csfg_expr_equal(pool, search, pool, prod1))
            return n;
        if (search != prod2 && csfg_expr_equal(pool, search, pool, prod2))
            return n;
        return -1;
    }

    if (search != n && csfg_expr_equal(pool, search, pool, n))
        return n;

    return -1;
}

/* -------------------------------------------------------------------------- */
static int process_chain(struct csfg_expr_pool** pool, int n, int top)
{
    for (; top > -1 && (*pool)->nodes[top].type == CSFG_EXPR_ADD;
         top = csfg_expr_find_parent((*pool), top))
    {
        int match = find_same_expr(
            (*pool),
            top,
            /* If the sum is multiplied by a product, want to search only for
               the sum and not the whole product */
            (*pool)->nodes[n].type == CSFG_EXPR_MUL
                ? (*pool)->nodes[n].child[0]
                : n);
        if (match == -1)
            continue;

        if ((*pool)->nodes[n].type == CSFG_EXPR_MUL)
        {
            int prod1 = (*pool)->nodes[n].child[1];
            int sum1 = csfg_expr_dup_single(pool, prod1);
            int sum2 =
                (*pool)->nodes[match].type == CSFG_EXPR_MUL
                    ? csfg_expr_dup_single(pool, (*pool)->nodes[match].child[1])
                    : csfg_expr_lit(pool, 1.0);
            if (csfg_expr_set_add(pool, prod1, sum1, sum2) == -1)
                return -1;
        }
        else
        {
            int exp =
                (*pool)->nodes[match].type == CSFG_EXPR_MUL
                    ? csfg_expr_add(
                          pool,
                          csfg_expr_dup_single(pool, (*pool)->nodes[match].child[1]),
                          csfg_expr_lit(pool, 1.0))
                    : csfg_expr_lit(pool, 2.0);
            if (csfg_expr_set_pow(pool, n, csfg_expr_dup_single(pool, n), exp) == -1)
                return -1;
        }
        csfg_expr_collapse_sibling_into_parent(*pool, match);
        return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int simplify_sums(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int left = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_ADD)
            continue;

        switch (process_chain(pool, left, n))
        {
            case -1: return -1;
            case 0: break;
            case 1: modified = 1; break;
        }
        switch (process_chain(pool, right, n))
        {
            case -1: return -1;
            case 0: break;
            case 1: modified = 1; break;
        }
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_simplify_sums(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, simplify_sums);
}

#if 0
#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include <assert.h>

/* -------------------------------------------------------------------------- */
static int find_same_expr(const struct csfg_expr_pool* pool, int n, int search)
{
    if (pool->nodes[search].type == CSFG_EXPR_LIT)
        return -1;

    if (pool->nodes[n].type == CSFG_EXPR_OP_ADD ||
        pool->nodes[n].type == CSFG_EXPR_OP_MUL)
    {
        int left = pool->nodes[n].child[0];
        int right = pool->nodes[n].child[1];
        n = find_same_expr(pool, left, search);
        if (n > -1)
            return n;
        n = find_same_expr(pool, right, search);
        if (n > -1)
            return n;
        return -1;
    }

    if (search != n && csfg_expr_equal(pool, search, pool, n))
        return n;

    return -1;
}

/* -------------------------------------------------------------------------- */
static int process_chain(struct csfg_expr_pool** pool, int n, int top)
{
    int search_sibling, match, match_parent, sum;

    /* "n" is the subtree we are searching a match for, and can either be a
     * multiplication, or a summand. If "n" is a multiplication, then we try to
     * find a match for either the left or right operand, so we can +1 on the
     * existing product. If "n" is instead a summand, then we don't have an
     * existing product and are searching for a direct match for "n". */
    if ((*pool)->nodes[n].type == CSFG_EXPR_OP_MUL)
    {
        int search = (*pool)->nodes[n].child[0];
        search_sibling = (*pool)->nodes[n].child[1];
        match = find_same_expr((*pool), top, search);
        if (match == -1)
        {
            search = (*pool)->nodes[n].child[1];
            search_sibling = (*pool)->nodes[n].child[0];
            match = find_same_expr((*pool), top, search);
        }
    }
    else
    {
        int search = n;
        match = find_same_expr((*pool), top, search);
    }
    if (match == -1)
        return 0;

    if ((*pool)->nodes[n].type != CSFG_EXPR_OP_MUL)
    {
        search_sibling = csfg_expr_lit(pool, 1.0);
        csfg_expr_set_mul(pool, n, csfg_expr_copy(pool, n), search_sibling);
    }

    match_parent = csfg_expr_find_parent(*pool, match);
    assert(match_parent > -1);
    if ((*pool)->nodes[match_parent].type == CSFG_EXPR_OP_MUL)
    {
        match_parent = csfg_expr_find_top_of_chain(*pool, match_parent);
        csfg_expr_collapse_sibling_into_parent(*pool, match);
        sum = csfg_expr_collapse_sibling_into_parent_steal_orphan(
            *pool, match_parent);
        csfg_expr_set_add(
            pool, search_sibling, csfg_expr_copy(pool, search_sibling), sum);
    }
    else
    {
        csfg_expr_collapse_sibling_into_parent(*pool, match);
        sum = csfg_expr_lit(pool, 1.0);
        csfg_expr_set_add(
            pool, search_sibling, csfg_expr_copy(pool, search_sibling), sum);
    }

    return 1;
}

/* -------------------------------------------------------------------------- */
static int simplify_sums(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int top;
        int left = (*pool)->nodes[n].child[0];
        int right = (*pool)->nodes[n].child[1];
        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_ADD)
            continue;

        top = csfg_expr_find_top_of_chain(*pool, n);
        switch (process_chain(pool, left, top))
        {
            case -1: return -1;
            case 0: break;
            case 1:
                modified = 1;
                CSFG_DEBUG_ASSERT(csfg_expr_integrity_check(*pool) == 0);
                break;
        }
        switch (process_chain(pool, right, top))
        {
            case -1: return -1;
            case 0: break;
            case 1:
                modified = 1;
                CSFG_DEBUG_ASSERT(csfg_expr_integrity_check(*pool) == 0);
                break;
        }
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_simplify_sums(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, simplify_sums);
}
#endif
