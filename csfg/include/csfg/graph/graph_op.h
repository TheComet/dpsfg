#pragma once

struct csfg_expr_pool;
struct csfg_graph;

/*!
 * @brief Computes the graph's transfer function as an expression.
 * @note This is quite an expensive operation.
 * @param[in] node_in/node_out These specify the forward path through the graph.
 */
int csfg_graph_op_calc_expr(
    struct csfg_expr_pool**  pool,
    const struct csfg_graph* graph,
    int                      node_in,
    int                      node_out);
