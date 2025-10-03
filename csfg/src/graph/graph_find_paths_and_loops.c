#include "csfg/graph/graph.h"

/* -------------------------------------------------------------------------- */
static int find_paths_recurse(
    struct csfg_path_vec** paths,
    struct csfg_path_vec** stack,
    struct csfg_graph*     graph,
    int                    edge_idx,
    int                    node_out)
{
    struct csfg_edge* edge = csfg_graph_get_edge(graph, edge_idx);
    struct csfg_node* node = csfg_graph_get_node(graph, edge->to);
    if (node->visited)
        return 0;

    node->visited = 1;
    if (csfg_path_vec_push(stack, edge_idx) != 0)
        return -1;

    if (edge->to == node_out)
    {
        const int* idx;
        vec_for_each (*stack, idx)
            if (csfg_path_vec_push(paths, *idx) != 0)
                return -1;
        if (csfg_path_vec_push(paths, -1) != 0)
            return -1;
    }
    else
    {
        int current_node_idx = edge->to;
        csfg_graph_enumerate_edges (graph, edge_idx, edge)
        {
            if (edge->from == current_node_idx)
                if (find_paths_recurse(
                        paths, stack, graph, edge_idx, node_out) != 0)
                    return -1;
        }
    }

    csfg_path_vec_pop(*stack);
    node->visited = 0;

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_find_forward_paths(
    struct csfg_graph*     graph,
    struct csfg_path_vec** paths,
    int                    node_in,
    int                    node_out)
{
    struct csfg_path_vec* stack;
    struct csfg_node*     node;
    struct csfg_edge*     edge;
    int                   edge_idx;

    csfg_path_vec_init(&stack);
    csfg_graph_for_each_node (graph, node)
        node->visited = 0;

    csfg_graph_enumerate_edges (graph, edge_idx, edge)
    {
        if (edge->from != node_in)
            continue;
        if (find_paths_recurse(paths, &stack, graph, edge_idx, node_out) != 0)
            goto find_paths_failed;
    }

    csfg_path_vec_deinit(stack);
    return 0;

find_paths_failed:
    csfg_path_vec_deinit(stack);
    return -1;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_find_loops(
    struct csfg_graph* graph, struct csfg_path_vec** paths)
{
    struct csfg_path_vec* stack;
    struct csfg_node*     node;
    struct csfg_edge*     edge;
    int                   edge_idx, node_idx;

    csfg_path_vec_init(&stack);
    csfg_graph_for_each_node (graph, node)
        node->visited = 0;

    csfg_graph_enumerate_nodes (graph, node_idx, node)
    {
        csfg_graph_enumerate_edges (graph, edge_idx, edge)
        {
            if (edge->from == node_idx)
                if (find_paths_recurse(
                        paths, &stack, graph, edge_idx, node_idx) != 0)
                    goto find_paths_failed;
        }
        node->visited = 1;
    }

    csfg_path_vec_deinit(stack);
    return 0;

find_paths_failed:
    csfg_path_vec_deinit(stack);
    return -1;
}
