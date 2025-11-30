#include "csfg/graph/graph.h"

/* -------------------------------------------------------------------------- */
int csfg_graph_paths_are_touching(
    const struct csfg_graph* graph, struct csfg_path p1, struct csfg_path p2)
{
    const int *             e1_idx, *e2_idx;
    const struct csfg_edge *e1, *e2;

    for (e1_idx = p1.edge_idxs; *e1_idx != -1; ++e1_idx)
        for (e2_idx = p2.edge_idxs; *e2_idx != -1; ++e2_idx)
        {
            e1 = vec_get(graph->edges, *e1_idx);
            e2 = vec_get(graph->edges, *e2_idx);
            if (e1->n_idx_from == e2->n_idx_from ||
                e1->n_idx_to == e2->n_idx_to)
            {
                return 1;
            }
        }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_graph_find_nontouching(
    const struct csfg_graph*    graph,
    struct csfg_path_vec**      nontouching,
    const struct csfg_path_vec* paths,
    const struct csfg_path      check_path)
{
    struct csfg_path path;
    const int*       edge_idx;

    csfg_paths_for_each (paths, path)
    {
        if (csfg_graph_paths_are_touching(graph, path, check_path))
            continue;

        for (edge_idx = path.edge_idxs; *edge_idx != -1; ++edge_idx)
            if (csfg_path_vec_push(nontouching, *edge_idx) != 0)
                return -1;
        if (csfg_path_vec_push(nontouching, -1) != 0)
            return -1;
    }

    return 0;
}
