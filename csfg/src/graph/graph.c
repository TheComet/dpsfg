#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"

VEC_DEFINE(csfg_node_vec, struct csfg_node, 16)
VEC_DEFINE(csfg_edge_vec, struct csfg_edge, 16)

/* -------------------------------------------------------------------------- */
static int id_exists(const struct csfg_graph* g, uint16_t id)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    csfg_graph_for_each_node (g, n)
        if (n->id == id)
            return 1;
    csfg_graph_for_each_edge (g, e)
        if (e->id == id)
            return 1;
    return 0;
}
static uint16_t new_id(struct csfg_graph* g)
{
    uint16_t id = g->id_counter++;
    if (g->id_counter == CSFG_GRAPH_GC_ID)
        g->id_counter++;
    while (id_exists(g, id))
    {
        id++;
        if (id == CSFG_GRAPH_GC_ID)
            id++;
    }
    return id;
}

/* -------------------------------------------------------------------------- */
static int node_init(struct csfg_node* n, int id, const char* name)
{
    n->id = id;
    n->x  = 0;
    n->y  = 0;
    str_init(&n->name);
    return str_set_cstr(&n->name, name);
}

/* -------------------------------------------------------------------------- */
static void edge_init(
    struct csfg_edge* e,
    int id,
    int n_idx_from,
    int n_idx_to,
    struct csfg_expr_pool* pool,
    int expr)
{
    e->id         = id;
    e->n_idx_from = n_idx_from;
    e->n_idx_to   = n_idx_to;
    e->x          = 0;
    e->y          = 0;
    e->pool       = pool;
    e->expr       = expr;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_init(struct csfg_graph* g)
{
    csfg_node_vec_init(&g->nodes);
    csfg_edge_vec_init(&g->edges);
}

/* -------------------------------------------------------------------------- */
void csfg_graph_deinit(struct csfg_graph* g)
{
    csfg_graph_clear(g);
    csfg_node_vec_deinit(g->nodes);
    csfg_edge_vec_deinit(g->edges);
}

/* -------------------------------------------------------------------------- */
void csfg_graph_clear(struct csfg_graph* g)
{
    struct csfg_node* n;
    struct csfg_edge* e;

    vec_for_each (g->nodes, n)
        str_deinit(n->name);
    csfg_node_vec_clear(g->nodes);

    vec_for_each (g->edges, e)
        csfg_expr_pool_deinit(e->pool);
    csfg_edge_vec_clear(g->edges);
}

/* -------------------------------------------------------------------------- */
int csfg_graph_add_node(struct csfg_graph* g, const char* name)
{
    int n_idx           = vec_count(g->nodes);
    struct csfg_node* n = csfg_node_vec_emplace(&g->nodes);
    if (n == NULL)
        return -1;

    if (node_init(n, new_id(g), name) != 0)
        return -1;

    return n_idx;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_add_edge(
    struct csfg_graph* g,
    int n_idx_from,
    int n_idx_to,
    struct csfg_expr_pool* pool,
    int expr)
{
    int e_idx           = vec_count(g->edges);
    struct csfg_edge* e = csfg_edge_vec_emplace(&g->edges);
    if (e == NULL)
        return -1;

    edge_init(e, new_id(g), n_idx_from, n_idx_to, pool, expr);
    return e_idx;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_add_edge_parse_expr(
    struct csfg_graph* g, int n_idx_from, int n_idx_to, struct strview text)
{
    int expr, edge;
    struct csfg_expr_pool* pool;

    csfg_expr_pool_init(&pool);
    expr = csfg_expr_parse(&pool, text);
    if (expr < 0)
        goto parse_failed;

    edge = csfg_graph_add_edge(g, n_idx_from, n_idx_to, pool, expr);
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
    vec_get(g->nodes, n)->id = CSFG_GRAPH_GC_ID;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_mark_edge_deleted(struct csfg_graph* g, int e_idx)
{
    vec_get(g->edges, e_idx)->id = CSFG_GRAPH_GC_ID;
}

/* -------------------------------------------------------------------------- */
void csfg_graph_gc(struct csfg_graph* g)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    int n_idx, e_idx;

    csfg_graph_enumerate_edges (g, e_idx, e)
    {
        int end = csfg_graph_edge_count(g) - 1;
        if (e->id != CSFG_GRAPH_GC_ID)
            continue;

        csfg_expr_pool_deinit(e->pool);
        csfg_edge_vec_swap_values(g->edges, e_idx, end);
        csfg_edge_vec_pop(g->edges);
        --e_idx;
    }

    csfg_graph_enumerate_nodes (g, n_idx, n)
    {
        int end = csfg_graph_node_count(g) - 1;
        if (n->id != CSFG_GRAPH_GC_ID)
            continue;

        csfg_graph_for_each_edge (g, e)
        {
            if (e->n_idx_from == end)
                e->n_idx_from = n_idx;
            if (e->n_idx_to == end)
                e->n_idx_to = n_idx;
        }

        str_deinit(n->name);
        csfg_node_vec_swap_values(g->nodes, n_idx, end);
        csfg_node_vec_pop(g->nodes);
        --n_idx;
    }
}
