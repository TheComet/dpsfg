#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/bm.h"

/* -------------------------------------------------------------------------- */
static int touch_cache_size(int N)
{
    return N * (N - 1) / 2;
}
static int touch_cache_idx(int i, int j, int N)
{
    return i * (2 * N - i - 1) / 2 + (j - i - 1);
}

/* -------------------------------------------------------------------------- */
static int path_gain(
    const struct csfg_graph* graph,
    struct csfg_expr_pool**  pool,
    struct csfg_path         path)
{
    const int* edge_idx;
    int        expr = -1;

    for (edge_idx = path.edge_idxs; *edge_idx != -1; ++edge_idx)
    {
        const struct csfg_edge* edge = vec_get(graph->edges, *edge_idx);
        int factor = csfg_expr_dup_recurse_from(pool, edge->pool, edge->expr);
        if (expr == -1)
            expr = factor;
        else
            expr = csfg_expr_mul(pool, expr, factor);
    }

    return expr;
}

/* -------------------------------------------------------------------------- */
static int determinant(
    const struct csfg_graph*    graph,
    struct csfg_expr_pool**     pool,
    const struct csfg_path_vec* loops)
{
    int              i, j, k;
    int              det;
    int*             lcomb;
    int              loop_count;
    struct csfg_path path, p1, p2;
    struct bm*       touch_cache;

    loop_count = csfg_paths_count(loops);

    det = csfg_expr_lit(pool, 1.0);
    if (loop_count == 0)
        return det;

    /* For k=0 and k=1 the expressions are trivial and can be computed manually
     * as follows:
     *   1 - (L1 + L2 + ... + Li) where Li is the loop gain at index i.
     */
    csfg_paths_for_each (loops, path)
        det = csfg_expr_sub(pool, det, path_gain(graph, pool, path));
    if (loop_count < 2)
        return det;

    /*
     * Check which combination of loops touch each other, and cache results
     * into a "touching table" or tt for short, because
     * sfgsym_paths_are_touching() can be fairly expensive to call over and over
     * again.
     *
     * The table needs n(n-1)/2 bytes of memory instead of n^2 because we only
     * need unique combinations.
     */
    bm_init(&touch_cache);
    if (bm_realloc(&touch_cache, touch_cache_size(loop_count)) != 0)
        goto alloc_touch_cache_failed;

    p1 = csfg_paths_begin(loops);
    bm_reset_all(touch_cache);
    for (i = 0; i != loop_count - 1; ++i, p1 = csfg_paths_next(p1))
    {
        p2 = csfg_paths_next(p1);
        for (j = i + 1; j != loop_count; ++j, p2 = csfg_paths_next(p2))
            if (csfg_graph_paths_are_touching(graph, p1, p2))
                bm_set(touch_cache, touch_cache_idx(i, j, loop_count));
    }

    /*
     * Create array for holding the current combination of loops. In the worst
     * case all loops from 0 to N-1 will need to be referenced.
     */
    lcomb = mem_alloc(sizeof(int) * loop_count);
    if (lcomb == NULL)
        goto alloc_counters_failed;

    for (k = 2; k != loop_count + 1; ++k)
    {
        int found_nontouching_loops = 0;

        /* The initial combination of loops is 0, 1, ..., k */
        for (i = 0; i != k; ++i)
            lcomb[k - i - 1] = i;

        while (1)
        {
            int combined_gain_expr;

            /* We add or subtract the result of loop combinations depending on
             * k being odd or even */
            int (*combine_expr_func)(struct csfg_expr_pool**, int, int) =
                (k % 2 == 0 ? csfg_expr_add : csfg_expr_sub);

            /* Check if any loops in the current combination touch each other */
            for (i = 0; i != k - 1; ++i)
                for (j = i + 1; j != k; ++j)
                {
                    const int l1 = lcomb[j];
                    const int l2 = lcomb[i];
                    const int tc_idx = touch_cache_idx(l1, l2, loop_count);
                    if (bm_test(touch_cache, tc_idx))
                        goto loops_touch;
                }
            found_nontouching_loops = 1;

            /* The current combination of loops do not touch each other, so
             * multiply their gains together. */
            combined_gain_expr =
                path_gain(graph, pool, csfg_paths_get(loops, lcomb[0]));
            for (i = 1; i != k; ++i)
            {
                combined_gain_expr = csfg_expr_mul(
                    pool,
                    combined_gain_expr,
                    path_gain(graph, pool, csfg_paths_get(loops, lcomb[i])));
            }

            /* Now, add or subtract it to/from the final expression */
            det = combine_expr_func(pool, det, combined_gain_expr);

        loops_touch:
            lcomb[0]++;
            for (i = 0; i != k; ++i)
            {
                if (lcomb[i] >= loop_count - i)
                {
                    if (i == k - 1)
                        goto no_more_combinations;

                    lcomb[i + 1]++;
                    for (j = i; j >= 0; --j)
                        lcomb[j] = lcomb[j + 1] + 1;
                    continue;
                }

                break;
            }
        }

        /*
         * Can exit early if for the current k no non-touching loops were found,
         * because that necessarily means that for larger k, there won't be
         * any non-touching loops either.
         */
    no_more_combinations:
        if (!found_nontouching_loops)
            break;
    }

    mem_free(lcomb);
    bm_deinit(touch_cache);
    return det;

    mem_free(lcomb);
alloc_counters_failed:
    bm_deinit(touch_cache);
alloc_touch_cache_failed:
    return -1;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_mason(
    const struct csfg_graph*    graph,
    struct csfg_expr_pool**     pool,
    const struct csfg_path_vec* paths,
    const struct csfg_path_vec* loops)
{
    int                   gain_expr, expr;
    struct csfg_path      path;
    struct csfg_path_vec* nontouching_loops;
    csfg_path_vec_init(&nontouching_loops);
    csfg_expr_pool_clear(*pool);

    expr = -1;
    csfg_paths_for_each (paths, path)
    {
        csfg_path_vec_clear(nontouching_loops);
        if (csfg_graph_find_nontouching(
                graph, &nontouching_loops, loops, path) != 0)
        {
            goto fail;
        }

        gain_expr = csfg_expr_mul(
            pool,
            path_gain(graph, pool, path),
            determinant(graph, pool, nontouching_loops));
        if (expr == -1)
            expr = gain_expr;
        else
            expr = csfg_expr_add(pool, expr, gain_expr);

        if (expr == -1)
            goto fail;
    }

    csfg_path_vec_deinit(nontouching_loops);
    return csfg_expr_div(pool, expr, determinant(graph, pool, loops));

fail:
    csfg_path_vec_deinit(nontouching_loops);
    return -1;
}
