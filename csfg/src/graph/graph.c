#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"

VEC_DEFINE(csfg_node_vec, struct csfg_node, 16)
VEC_DEFINE(csfg_edge_vec, struct csfg_edge, 16)

VEC_DEFINE(csfg_path_vec, int, 16)

/* -------------------------------------------------------------------------- */
static int node_init(struct csfg_node* n, int id, const char* name)
{
    n->id = id;
    str_init(&n->name);
    return str_set_cstr(&n->name, name);
}

/* -------------------------------------------------------------------------- */
static void edge_init(
    struct csfg_edge*      e,
    int                    id,
    int                    n_from,
    int                    n_to,
    struct csfg_expr_pool* pool,
    int                    expr)
{
    e->id = id;
    e->from = n_from;
    e->to = n_to;
    e->pool = pool;
    e->expr = expr;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_init(struct csfg_graph* g)
{
    csfg_node_vec_init(&g->nodes);
    csfg_edge_vec_init(&g->edges);
    g->id_counter = 0;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_deinit(struct csfg_graph* g)
{
    struct csfg_node* n;
    struct csfg_edge* e;

    vec_for_each (g->nodes, n)
        str_deinit(n->name);
    csfg_node_vec_deinit(g->nodes);

    vec_for_each (g->edges, e)
        csfg_expr_pool_deinit(e->pool);
    csfg_edge_vec_deinit(g->edges);
}

/* -------------------------------------------------------------------------- */
int csfg_graph_add_node(struct csfg_graph* g, const char* name)
{
    int               n_idx = vec_count(g->nodes);
    struct csfg_node* n = csfg_node_vec_emplace(&g->nodes);
    if (n == NULL)
        return -1;

    if (node_init(n, g->id_counter++, name) != 0)
        return -1;

    return n_idx;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_add_edge(
    struct csfg_graph*     g,
    int                    n_from,
    int                    n_to,
    struct csfg_expr_pool* pool,
    int                    expr)
{
    int               e_idx = vec_count(g->edges);
    struct csfg_edge* e = csfg_edge_vec_emplace(&g->edges);
    if (e == NULL)
        return -1;

    edge_init(e, g->id_counter++, n_from, n_to, pool, expr);
    return e_idx;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_add_edge_parse_expr(
    struct csfg_graph* g, int n_from, int n_to, struct strview text)
{
    int                    expr, edge;
    struct csfg_expr_pool* pool;

    csfg_expr_pool_init(&pool);
    expr = csfg_expr_parse(&pool, text);
    if (expr < 0)
        goto parse_failed;

    edge = csfg_graph_add_edge(g, n_from, n_to, pool, expr);
    if (edge < 0)
        goto parse_failed;

    return edge;

parse_failed:
    csfg_expr_pool_deinit(pool);
    return -1;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_mark_node_deleted(struct csfg_graph* g, int n)
{
    vec_get(g->nodes, n)->id = -1;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_mark_edge_deleted(struct csfg_graph* g, int e_idx)
{
    vec_get(g->edges, e_idx)->id = -1;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_gc(struct csfg_graph* g)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    int               n_idx, e_idx;

    csfg_graph_enumerate_edges (g, e_idx, e)
    {
        int end = csfg_graph_edge_count(g) - 1;
        if (e->id > -1)
            continue;

        csfg_expr_pool_deinit(e->pool);
        csfg_edge_vec_swap_values(g->edges, e_idx, end);
        csfg_edge_vec_pop(g->edges);
        --e_idx;
    }

    csfg_graph_enumerate_nodes (g, n_idx, n)
    {
        int end = csfg_graph_node_count(g) - 1;
        if (n->id > -1)
            continue;

        csfg_graph_for_each_edge (g, e)
        {
            if (e->from == end)
                e->from = n_idx;
            if (e->to == end)
                e->to = n_idx;
        }

        str_deinit(n->name);
        csfg_node_vec_swap_values(g->nodes, n_idx, end);
        csfg_node_vec_pop(g->nodes);
        --n_idx;
    }
}
