#include "csfg/graph/graph.h"
#include "graph-editor/attr.h"
#include "graph-editor/constants.h"
#include "graph-editor/geometry.h"
#include "graph-editor/graph_helpers.h"
#include <math.h>

/* -------------------------------------------------------------------------- */
int find_node_idx(struct csfg_graph* g, int node_id)
{
    struct csfg_node* n;
    int idx;
    if (g == NULL)
        return -1;
    csfg_graph_enumerate_nodes (g, idx, n)
        if (n->id == node_id)
            return idx;
    return -1;
}

/* -------------------------------------------------------------------------- */
struct csfg_node* find_node(struct csfg_graph* g, int node_id)
{
    struct csfg_node* n;
    if (g == NULL)
        return NULL;
    csfg_graph_for_each_node (g, n)
        if (n->id == node_id)
            return n;
    return NULL;
}

/* -------------------------------------------------------------------------- */
int find_edge_idx(struct csfg_graph* g, int edge_id)
{
    struct csfg_edge* e;
    int idx;
    if (g == NULL)
        return -1;
    csfg_graph_enumerate_edges (g, idx, e)
        if (e->id == edge_id)
            return idx;
    return -1;
}

/* -------------------------------------------------------------------------- */
struct csfg_edge* find_edge(struct csfg_graph* g, int edge_id)
{
    struct csfg_edge* e;
    if (g == NULL)
        return NULL;
    csfg_graph_for_each_edge (g, e)
        if (e->id == edge_id)
            return e;
    return NULL;
}

/* -------------------------------------------------------------------------- */
int find_farthest_node_in_direction(
    const struct csfg_graph* graph, double dx, double dy)
{
    int idx, new_active_node;
    const struct csfg_node* n;
    double x, y;

    if (graph == NULL)
        return -1;

    new_active_node = -1;
    csfg_graph_enumerate_nodes (graph, idx, n)
    {
        if (new_active_node == -1)
        {
            x = n->x, y = n->y;
            new_active_node = n->id;
            continue;
        }

        if (dx > 0.0 && n->x > x)
            x = n->x, new_active_node = n->id;
        if (dx < 0.0 && n->x < x)
            x = n->x, new_active_node = n->id;
        if (dy > 0.0 && n->y > y)
            y = n->y, new_active_node = n->id;
        if (dy < 0.0 && n->y < y)
            y = n->y, new_active_node = n->id;
    }

    return new_active_node;
}

/* -------------------------------------------------------------------------- */
int find_nearest_node_in_direction(
    const struct csfg_graph* graph,
    int active_node_id,
    double dirx,
    double diry,
    double current_x,
    double current_y)
{
    int idx;
    const struct csfg_node* n;
    double dx, dy;
    double new_dist, dist = 100000. * 100000.;

    if (graph == NULL)
        return -1;

    csfg_graph_enumerate_nodes (graph, idx, n)
    {
        if (n->id == active_node_id)
            continue;

        dx       = current_x - n->x;
        dy       = current_y - n->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
        {
            if (dirx > 0 && n->x > current_x)
                dist = new_dist, active_node_id = n->id;
            if (dirx < 0 && n->x < current_x)
                dist = new_dist, active_node_id = n->id;
            if (diry > 0 && n->y > current_y)
                dist = new_dist, active_node_id = n->id;
            if (diry < 0 && n->y < current_y)
                dist = new_dist, active_node_id = n->id;
        }
    }

    return active_node_id;
}

/* -------------------------------------------------------------------------- */
int find_farthest_edge_in_direction(
    const struct csfg_graph* graph, int active_edge_id, double dx, double dy)
{
    int idx;
    const struct csfg_edge* e;
    double x, y;

    if (graph == NULL)
        return -1;

    csfg_graph_enumerate_edges (graph, idx, e)
    {
        if (active_edge_id == -1)
        {
            x = e->x, y = e->y;
            active_edge_id = e->id;
            continue;
        }

        if (dx > 0.0 && e->x > x)
            x = e->x, active_edge_id = e->id;
        if (dx < 0.0 && e->x < x)
            x = e->x, active_edge_id = e->id;
        if (dy > 0.0 && e->y > y)
            y = e->y, active_edge_id = e->id;
        if (dy < 0.0 && e->y < y)
            y = e->y, active_edge_id = e->id;
    }

    return active_edge_id;
}

/* -------------------------------------------------------------------------- */
int find_nearest_edge_in_direction(
    const struct csfg_graph* graph,
    int active_edge_id,
    double dirx,
    double diry,
    double current_x,
    double current_y)
{
    int idx;
    const struct csfg_edge* e;
    double dx, dy;
    double new_dist, dist = 100000. * 100000.;

    if (graph == NULL)
        return -1;

    csfg_graph_enumerate_edges (graph, idx, e)
    {
        if (e->id == active_edge_id)
            continue;

        dx       = current_x - e->x;
        dy       = current_y - e->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
        {
            if (dirx > 0 && e->x > current_x)
                dist = new_dist, active_edge_id = e->id;
            if (dirx < 0 && e->x < current_x)
                dist = new_dist, active_edge_id = e->id;
            if (diry > 0 && e->y > current_y)
                dist = new_dist, active_edge_id = e->id;
            if (diry < 0 && e->y < current_y)
                dist = new_dist, active_edge_id = e->id;
        }
    }

    return active_edge_id;
}

/* -------------------------------------------------------------------------- */
int is_near_any_other_node(
    const struct csfg_graph* g,
    const struct csfg_node* exclude_node,
    int x,
    int y)
{
    const struct csfg_node* n;
    CSFG_DEBUG_ASSERT(g != NULL);
    csfg_graph_for_each_node (g, n)
    {
        int dx = x - n->x;
        int dy = y - n->y;
        if (exclude_node != NULL && n == exclude_node)
            continue;
        if (dx * dx + dy * dy < DEFAULT_NODE_SPACING * DEFAULT_NODE_SPACING / 4)
            return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int is_near_any_other_edge(
    const struct csfg_graph* g,
    const struct csfg_edge* exclude_edge,
    int x,
    int y)
{
    const struct csfg_edge* e;
    CSFG_DEBUG_ASSERT(g != NULL);
    csfg_graph_for_each_edge (g, e)
    {
        int dx = x - e->x;
        int dy = y - e->y;
        if (exclude_edge != NULL && e == exclude_edge)
            continue;
        if (dx * dx + dy * dy < DEFAULT_NODE_SPACING * DEFAULT_NODE_SPACING / 4)
            return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int node_is_connected_to_edge(struct csfg_graph* g, int n_id, int e_id)
{
    const struct csfg_edge* e;
    int n_idx;

    if (g == NULL)
        return 0;

    e = find_edge(g, e_id);
    if (e == NULL)
        return 0;

    n_idx = find_node_idx(g, n_id);
    return e->n_idx_from == n_idx || e->n_idx_to == n_idx;
}

/* -------------------------------------------------------------------------- */
int find_single_node_edge(const struct csfg_graph* g, int n_id)
{
    const struct csfg_edge* e;
    int connected_edges = 0, last_edge_id = -1;

    if (g == NULL)
        return -1;

    csfg_graph_for_each_edge (g, e)
        if (csfg_graph_get_node(g, e->n_idx_from)->id == n_id ||
            csfg_graph_get_node(g, e->n_idx_to)->id == n_id)
        {
            connected_edges++;
            last_edge_id = e->id;
        }

    if (connected_edges == 1)
        return last_edge_id;

    return -1;
}

/* -------------------------------------------------------------------------- */
void auto_position_node_grid(
    const struct csfg_graph* g, struct csfg_node* n, int total_node_count)
{
    const int row_end =
        (int)ceil(sqrt(total_node_count)) * DEFAULT_NODE_SPACING;
    CSFG_DEBUG_ASSERT(g != NULL);

    n->x = 0.0;
    n->y = 0.0;
    while (is_near_any_other_node(g, n, n->x, n->y))
    {
        n->x += DEFAULT_NODE_SPACING;
        if (n->x >= row_end)
        {
            n->x = 0.0;
            n->y += DEFAULT_NODE_SPACING;
        }
    }
}

/* -------------------------------------------------------------------------- */
void auto_position_node_near(
    const struct csfg_graph* g, struct csfg_node* n, int near_x, int near_y)
{
    CSFG_DEBUG_ASSERT(g != NULL);
    n->x = near_x;
    n->y = near_y;
    while (is_near_any_other_node(g, n, n->x, n->y))
        n->x += DEFAULT_NODE_SPACING;
}

/* -------------------------------------------------------------------------- */
void auto_position_edge(
    const struct csfg_graph* g,
    struct csfg_edge* e,
    const struct csfg_node* from,
    const struct csfg_node* to)
{
    CSFG_DEBUG_ASSERT(g != NULL);

    e->x = (from->x + to->x) / 2;
    e->y = (from->y + to->y) / 2;

    int spacing_increments = DEFAULT_NODE_SPACING / 2;
    int edge_add_spacing   = spacing_increments;

    while (is_near_any_other_node(g, NULL, e->x, e->y) ||
           is_near_any_other_edge(g, e, e->x, e->y))
    {
        int tmp;
        int nx  = from->x - to->x;
        int ny  = from->y - to->y;
        int den = sqrt(nx * nx + ny * ny);
        if (den != 0)
        {
            tmp = nx;
            nx  = -ny / den;
            ny  = tmp / den;

            e->x -= nx * edge_add_spacing;
            e->y -= ny * edge_add_spacing;
            edge_add_spacing = edge_add_spacing > 0
                                 ? -(edge_add_spacing + spacing_increments)
                                 : -edge_add_spacing + spacing_increments;
        }
        else
        {
            e->x = from->x;
            e->y -= edge_add_spacing;
        }
    }
}

/* -------------------------------------------------------------------------- */
void drag_edge_connected_to_node(
    struct csfg_graph* g,
    struct csfg_edge* e,
    int n1_idx,
    int n1_prev_x,
    int n1_prev_y,
    int is_reconnect_operation)
{
    const struct csfg_node *n1, *n2;
    double cx, cy, radius;

    n1 = csfg_graph_get_node(g, n1_idx);
    n2 = e->n_idx_from == n1_idx ? csfg_graph_get_node(g, e->n_idx_to)
       : e->n_idx_to == n1_idx   ? csfg_graph_get_node(g, e->n_idx_from)
                                 : NULL;
    if (n2 == NULL)
        return;

    if (!is_reconnect_operation && e->n_idx_from == e->n_idx_to)
    {
        /* We want to move the edge the same distance if that edge is connected
         * to itself, but only if the loop was not created from reconnecting. If
         * a reconnection happened, then we still want to run the usual "divide
         * by 2" logic to move the edge half the distance so it looks right. */
        e->x += n1->x - n1_prev_x;
        e->y += n1->y - n1_prev_y;
    }
    else if (calc_circle(
                 &cx,
                 &cy,
                 &radius,
                 n1_prev_x,
                 n1_prev_y,
                 e->x,
                 e->y,
                 n2->x,
                 n2->y))
    {
        /* This runs when the edge forms an arc, or if a loop is created from
         * reconnecting */
        e->x += (n1->x - n1_prev_x) / 2;
        e->y += (n1->y - n1_prev_y) / 2;
    }
    else
    {
        /* This runs when there is no arc, in which case we position the edge at
         * the center between the two nodes */
        e->x = (n1->x + n2->x) / 2;
        e->y = (n1->y + n2->y) / 2;
    }
}

/* -------------------------------------------------------------------------- */
void drag_all_edges_connected_to_node(
    struct csfg_graph* g,
    int n1_id,
    int n1_prev_x,
    int n1_prev_y,
    int is_reconnect_operation)
{
    struct csfg_edge* e;
    int n_idx;

    n_idx = find_node_idx(g, n1_id);
    if (n_idx < 0)
        return;

    csfg_graph_for_each_edge (g, e)
        drag_edge_connected_to_node(
            g, e, n_idx, n1_prev_x, n1_prev_y, is_reconnect_operation);
}

/* -------------------------------------------------------------------------- */
void bump_edge(struct csfg_graph* g, struct csfg_edge* e)
{
    struct csfg_node *n1, *n2;
    if (is_near_any_other_node(g, NULL, e->x, e->y) ||
        is_near_any_other_edge(g, e, e->x, e->y))
    {
        n1 = csfg_graph_get_node(g, e->n_idx_from);
        n2 = csfg_graph_get_node(g, e->n_idx_to);
        auto_position_edge(g, e, n1, n2);
    }
}

/* -------------------------------------------------------------------------- */
void bump_all_edges_connected_to_node(struct csfg_graph* g, int n_id)
{
    struct csfg_edge* e;
    int n_idx;

    n_idx = find_node_idx(g, n_id);
    if (n_idx < 0)
        return;

    csfg_graph_for_each_edge (g, e)
        if (e->n_idx_from == n_idx || e->n_idx_to == n_idx)
            bump_edge(g, e);
}

/* -------------------------------------------------------------------------- */
int try_select_node(
    const struct csfg_graph* g,
    const struct node_attr_hmap* attrs,
    int mouse_x,
    int mouse_y)
{
    int dx, dy, idx;
    const struct csfg_node* n;
    const struct node_attr* na;

    if (g == NULL)
        return -1;

    csfg_graph_enumerate_nodes (g, idx, n)
    {
        na = node_attr_hmap_find(attrs, n->id);
        dx = mouse_x - n->x;
        dy = mouse_y - n->y;
        if (dx * dx + dy * dy < na->radius * na->radius)
            return n->id;
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
int try_select_edge(const struct csfg_graph* g, int mouse_x, int mouse_y)
{
    int dx, dy, idx;
    const struct csfg_edge* e;

    if (g == NULL)
        return -1;

    csfg_graph_enumerate_edges (g, idx, e)
    {
        dx = mouse_x - e->x;
        dy = mouse_y - e->y;
        if (dx * dx + dy * dy < ARROW_RADIUS * ARROW_RADIUS)
            return e->id;
    }

    return -1;
}
