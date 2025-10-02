#pragma once

struct csfg_expr_pool;
struct csfg_graph;
struct csfg_edge_idx_vec;

struct csfg_graph_path
{
    const int* edge_idxs;
};

int csfg_graph_find_forward_paths(
    struct csfg_graph*         graph,
    struct csfg_edge_idx_vec** paths,
    int                        node_in,
    int                        node_out);

int csfg_graph_find_loops(
    struct csfg_graph* graph, struct csfg_edge_idx_vec** paths);

/*!
 * \brief Given a list of paths (paths can be be either forward paths or loops),
 * fills "nontouching" with all paths that do not touch "check_path".
 * \param[out] nontouching List to fill with nontouching paths
 * \param[in] paths List of paths to check against check_path.
 * \param[in] check_path A single path in the graph.
 */
int csfg_graph_find_nontouching(
    const struct csfg_graph*        graph,
    struct csfg_edge_idx_vec**      nontouching,
    const struct csfg_edge_idx_vec* paths,
    const struct csfg_graph_path*   check_path);

/*!
 * @brief Computes the graph's transfer function as an expression.
 * @note This is quite an expensive operation.
 * @param[in] node_in/node_out These specify the forward path through the graph.
 */
int csfg_graph_op_calc_expr(
    const struct csfg_graph* graph,
    struct csfg_expr_pool**  pool,
    int                      node_in,
    int                      node_out);
