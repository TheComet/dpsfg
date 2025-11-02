#pragma once

#include "csfg/util/strview.h"
#include "csfg/util/vec.h"

struct str;

struct csfg_node
{
    struct str* name;
    int         id;
    unsigned    visited : 1;
};

struct csfg_edge
{
    struct csfg_expr_pool* pool;
    int                    expr;

    int id;
    int from;
    int to;
};

struct csfg_path
{
    const int* edge_idxs;
};

VEC_DECLARE(csfg_node_vec, struct csfg_node, 16)
VEC_DECLARE(csfg_edge_vec, struct csfg_edge, 16)

VEC_DECLARE(csfg_path_vec, int, 16)

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
int csfg_graph_add_edge_parse_expr(
    struct csfg_graph* g, int n_from, int n_to, struct strview text);
void csfg_graph_mark_node_deleted(struct csfg_graph* g, int n);
void csfg_graph_mark_edge_deleted(struct csfg_graph* g, int e);

void csfg_graph_gc(struct csfg_graph* g);

#define csfg_graph_for_each_node(g, var)                                       \
    vec_for_each ((g) ? (g)->nodes : NULL, (var))
#define csfg_graph_for_each_edge(g, var)                                       \
    vec_for_each ((g) ? (g)->edges : NULL, (var))
#define csfg_graph_enumerate_nodes(g, i, var)                                  \
    vec_enumerate ((g) ? (g)->nodes : NULL, (i), (var))
#define csfg_graph_enumerate_edges(g, i, var)                                  \
    vec_enumerate ((g) ? (g)->edges : NULL, (i), (var))
#define csfg_graph_get_node(g, i) ((g) ? vec_get((g)->nodes, (i)) : NULL)
#define csfg_graph_get_edge(g, i) ((g) ? vec_get((g)->edges, (i)) : NULL)
#define csfg_graph_node_count(g)  ((g) ? vec_count((g)->nodes) : 0)
#define csfg_graph_edge_count(g)  ((g) ? vec_count((g)->edges) : 0)

int csfg_graph_find_forward_paths(
    struct csfg_graph*     graph,
    struct csfg_path_vec** paths,
    int                    node_in,
    int                    node_out);

int csfg_graph_find_loops(
    struct csfg_graph* graph, struct csfg_path_vec** paths);

int csfg_graph_paths_are_touching(
    const struct csfg_graph* graph, struct csfg_path p1, struct csfg_path p2);

/*!
 * @brief Given a list of paths (paths can be be either forward paths or loops),
 * fills "nontouching" with all paths that do not touch "check_path".
 * @param[out] nontouching List to fill with nontouching paths
 * @param[in] paths List of paths to check against check_path.
 * @param[in] check_path A single path in the graph.
 */
int csfg_graph_find_nontouching(
    const struct csfg_graph*    graph,
    struct csfg_path_vec**      nontouching,
    const struct csfg_path_vec* paths,
    const struct csfg_path      check_path);

/*!
 * @brief Computes the graph's transfer function as an expression.
 * @return Expression root into "pool", or -1 if an error occurred.
 */
int csfg_graph_mason(
    const struct csfg_graph*    graph,
    struct csfg_expr_pool**     pool,
    const struct csfg_path_vec* paths,
    const struct csfg_path_vec* loops);

static struct csfg_path csfg_paths_begin(const struct csfg_path_vec* paths)
{
    struct csfg_path path;
    path.edge_idxs = vec_begin(paths);
    return path;
}

static struct csfg_path csfg_paths_next(struct csfg_path path)
{
    while (*path.edge_idxs != -1)
        path.edge_idxs++;
    path.edge_idxs++;
    return path;
}

static int csfg_path_count(const struct csfg_path_vec* paths)
{
    int        count = 0;
    const int* edge_idx;
    vec_for_each (paths, edge_idx)
        if (*edge_idx == -1)
            count++;
    return count;
}

static struct csfg_path
csfg_paths_get(const struct csfg_path_vec* paths, int idx)
{
    struct csfg_path path = csfg_paths_begin(paths);
    while (idx--)
        path = csfg_paths_next(path);
    return path;
}

#define csfg_paths_for_each(paths, path)                                       \
    for (path = csfg_paths_begin(paths); path.edge_idxs != vec_end(paths);     \
         path = csfg_paths_next(path))
#define csfg_paths_enumerate(paths, idx, path)                                 \
    for (path = csfg_paths_begin(paths), idx = 0;                              \
         path.edge_idxs != vec_end(paths);                                     \
         path = csfg_paths_next(path), ++idx)
