#pragma once

#include "csfg/util/vec.h"

struct str;

struct csfg_node
{
    struct str* name;
    int         id;
};

struct csfg_edge
{
    struct csfg_expr_pool* pool;
    int                    expr;

    int id;
    int from;
    int to;
};

VEC_DECLARE(csfg_node_vec, struct csfg_node, 16)
VEC_DECLARE(csfg_edge_vec, struct csfg_edge, 16)

struct csfg_graph
{
    struct csfg_node_vec* nodes;
    struct csfg_edge_vec* edges;
    int                   id_counter;
};

void csfg_graph_init(struct csfg_graph* g);
void csfg_graph_deinit(struct csfg_graph* g);

int csfg_graph_add_node(struct csfg_graph* g, const char* name);
int csfg_graph_add_edge(
    struct csfg_graph*     g,
    int                    n_from,
    int                    n_to,
    struct csfg_expr_pool* pool,
    int                    expr);

#define csfg_graph_for_each_node(g, var)                                       \
    if (g)                                                                     \
        vec_for_each ((g)->nodes, (var))
#define csfg_graph_for_each_edge(g, var)                                       \
    if (g)                                                                     \
        vec_for_each ((g)->edges, (var))
#define csfg_graph_enumerate_nodes(g, i, var)                                  \
    if (g)                                                                     \
        vec_enumerate ((g)->nodes, (i), (var))
#define csfg_graph_enumerate_edges(g, i, var)                                  \
    if (g)                                                                     \
        vec_enumerate ((g)->edges, (i), (var))
#define csfg_graph_get_node(g, i) ((g) ? vec_get((g)->nodes, (i)) : NULL)
#define csfg_graph_get_edge(g, i) ((g) ? vec_get((g)->edges, (i)) : NULL)
#define csfg_graph_node_count(g)  ((g) ? vec_count((g)->nodes) : 0)
#define csfg_graph_edge_count(g)  ((g) ? vec_count((g)->edges) : 0)
