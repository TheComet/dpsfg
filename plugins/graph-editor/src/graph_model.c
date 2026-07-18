#include "csfg/graph/graph.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"
#include "dpsfg-plugin.h"
#include "graph-editor/attr.h"
#include "graph-editor/constants.h"
#include "graph-editor/drawing.h"
#include "graph-editor/graph_helpers.h"
#include "graph-editor/graph_model.h"
#include "graph-editor/undo.h"

/* -------------------------------------------------------------------------- */
void select_next_active_node_in_direction(
    struct graph_model* model, double dx, double dy)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    n = find_node(model->graph, model->active_node_id);
    e = find_edge(model->graph, model->active_edge_id);

    if (n != NULL)
        model->active_node_id = find_nearest_node_in_direction(
            model->graph, model->active_node_id, dx, dy, n->x, n->y);
    else if (e != NULL)
        model->active_node_id = find_nearest_node_in_direction(
            model->graph, model->active_node_id, dx, dy, e->x, e->y);
    else
        model->active_node_id =
            find_farthest_node_in_direction(model->graph, dx, dy);

    if (model->active_node_id > -1)
        model->active_edge_id = -1;
}

/* -------------------------------------------------------------------------- */
void select_next_active_edge_in_direction(
    struct graph_model* model, double dx, double dy)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    n = find_node(model->graph, model->active_node_id);
    e = find_edge(model->graph, model->active_edge_id);

    if (e != NULL)
        model->active_edge_id = find_nearest_edge_in_direction(
            model->graph, model->active_edge_id, dx, dy, e->x, e->y);
    else if (n != NULL)
        model->active_edge_id = find_nearest_edge_in_direction(
            model->graph, model->active_edge_id, dx, dy, n->x, n->y);
    else
        model->active_edge_id = find_farthest_edge_in_direction(
            model->graph, model->active_edge_id, dx, dy);

    if (model->active_edge_id > -1)
        model->active_node_id = -1;
}

/* -------------------------------------------------------------------------- */
void select_next_connected_node_in_direction(
    struct graph_model* model, int edge_id, double dirx, double diry)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    int idx, nodes[2];
    double dx, dy, current_x, current_y;
    double new_dist, dist = 100000. * 100000.;

    e = find_edge(model->graph, edge_id);
    if (e == NULL)
        return;

    n = find_node(model->graph, model->active_node_id);
    if (n != NULL)
        current_x = n->x, current_y = n->y;
    else
        current_x = e->x, current_y = e->y;

    nodes[0] = e->n_idx_from;
    nodes[1] = e->n_idx_to;
    for (idx = 0; idx != 2; ++idx)
    {
        n        = csfg_graph_get_node(model->graph, nodes[idx]);
        dx       = current_x - n->x;
        dy       = current_y - n->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
        {
            if (dirx > 0 && n->x > current_x)
                dist = new_dist, model->active_node_id = n->id;
            if (dirx < 0 && n->x < current_x)
                dist = new_dist, model->active_node_id = n->id;
            if (diry > 0 && n->y > current_y)
                dist = new_dist, model->active_node_id = n->id;
            if (diry < 0 && n->y < current_y)
                dist = new_dist, model->active_node_id = n->id;
        }
    }

    model->active_edge_id = -1;
}

/* -------------------------------------------------------------------------- */
void select_next_connected_edge_in_direction(
    struct graph_model* model, int node_id, double dirx, double diry)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    int idx;
    double dx, dy, current_x, current_y;
    double new_dist, dist = 100000. * 100000.;

    n = find_node(model->graph, node_id);
    if (n == NULL)
        return;

    e = find_edge(model->graph, model->active_edge_id);
    if (e != NULL)
        current_x = e->x, current_y = e->y;
    else
        current_x = n->x, current_y = n->y;

    csfg_graph_enumerate_edges (model->graph, idx, e)
    {
        if (csfg_graph_get_node(model->graph, e->n_idx_from)->id != n->id &&
            csfg_graph_get_node(model->graph, e->n_idx_to)->id != n->id)
            continue;

        dx       = current_x - e->x;
        dy       = current_y - e->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
        {
            if (dirx > 0 && e->x > current_x)
                dist = new_dist, model->active_edge_id = e->id;
            if (dirx < 0 && e->x < current_x)
                dist = new_dist, model->active_edge_id = e->id;
            if (diry > 0 && e->y > current_y)
                dist = new_dist, model->active_edge_id = e->id;
            if (diry < 0 && e->y < current_y)
                dist = new_dist, model->active_edge_id = e->id;
        }
    }

    model->active_node_id = -1;
}

/* -------------------------------------------------------------------------- */
void select_nearest_connected_node(struct graph_model* model, int edge_id)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    int idx, nodes[2];
    double dx, dy, current_x, current_y;
    double new_dist, dist = 100000. * 100000.;

    e = find_edge(model->graph, edge_id);
    if (e == NULL)
        return;

    n = find_node(model->graph, model->active_node_id);
    if (n != NULL)
        current_x = n->x, current_y = n->y;
    else
        current_x = e->x, current_y = e->y;

    nodes[0] = e->n_idx_from;
    nodes[1] = e->n_idx_to;
    for (idx = 0; idx != 2; ++idx)
    {
        n        = csfg_graph_get_node(model->graph, nodes[idx]);
        dx       = current_x - n->x;
        dy       = current_y - n->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
            dist = new_dist, model->active_node_id = n->id;
    }

    model->active_edge_id = -1;
}

/* -------------------------------------------------------------------------- */
void select_nearest_connected_edge(struct graph_model* model, int node_id)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    int idx;
    double dx, dy, current_x, current_y;
    double new_dist, dist = 100000. * 100000.;

    n = find_node(model->graph, node_id);
    if (n == NULL)
        return;

    e = find_edge(model->graph, model->active_edge_id);
    if (e != NULL)
        current_x = e->x, current_y = e->y;
    else
        current_x = n->x, current_y = n->y;

    csfg_graph_enumerate_edges (model->graph, idx, e)
    {
        if (csfg_graph_get_node(model->graph, e->n_idx_from)->id != n->id &&
            csfg_graph_get_node(model->graph, e->n_idx_to)->id != n->id)
            continue;

        dx       = current_x - e->x;
        dy       = current_y - e->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
            dist = new_dist, model->active_edge_id = e->id;
    }

    model->active_node_id = -1;
}

/* -------------------------------------------------------------------------- */
int reconnect_edge(
    struct graph_model* model,
    int edge_id,
    int current_node_id,
    int target_node_id)
{
    struct csfg_node* n1;
    struct csfg_edge* e  = find_edge(model->graph, edge_id);
    int current_node_idx = find_node_idx(model->graph, current_node_id);
    int target_node_idx  = find_node_idx(model->graph, target_node_id);
    if (e == NULL || current_node_idx < 0 || target_node_idx < 0)
        return current_node_id;
    n1 = csfg_graph_get_node(model->graph, current_node_idx);

    if (e->n_idx_to == current_node_idx)
    {
        e->n_idx_to = target_node_idx;
        drag_edge_connected_to_node(
            model->graph, e, e->n_idx_to, n1->x, n1->y, 1);
        bump_edge(model->graph, e);
        current_node_id = target_node_id;
    }
    else if (e->n_idx_from == current_node_idx)
    {
        e->n_idx_from = target_node_idx;
        drag_edge_connected_to_node(
            model->graph, e, e->n_idx_from, n1->x, n1->y, 1);
        bump_edge(model->graph, e);
        current_node_id = target_node_id;
    }

    return current_node_id;
}

/* -------------------------------------------------------------------------- */
void move_active_node_in_direction(
    struct graph_model* model, double dx, double dy)
{
    double n1_prev_x, n1_prev_y;
    struct csfg_node* n1;

    n1 = find_node(model->graph, model->active_node_id);
    if (n1 == NULL)
        return;

    n1_prev_x = n1->x;
    n1_prev_y = n1->y;
    n1->x += dx * DEFAULT_NODE_SPACING;
    n1->y += dy * DEFAULT_NODE_SPACING;
    drag_all_edges_connected_to_node(
        model->graph, model->active_node_id, n1_prev_x, n1_prev_y, 0);
    bump_all_edges_connected_to_node(model->graph, model->active_node_id);

    model->icb->graph_layout_changed(model->cb, model->plugin_ctx);
}

/* -------------------------------------------------------------------------- */
void move_active_edge_in_direction(
    struct graph_model* model, double dx, double dy)
{
    struct csfg_edge* e = find_edge(model->graph, model->active_edge_id);
    if (e == NULL)
        return;

    e->x += dx * GRID;
    e->y += dy * GRID;

    model->icb->graph_layout_changed(model->cb, model->plugin_ctx);
}

/* -------------------------------------------------------------------------- */
int create_new_edge(
    struct graph_model* model, int n_id_selected, int n_id_active)
{
    struct csfg_expr_pool* pool;
    struct edge_attr* ea;
    struct csfg_edge* e;
    const struct csfg_node* n_selected;
    const struct csfg_node* n_active;
    int n_idx_selected, n_idx_active, expr, e_idx;

    if (n_id_selected == -1 || n_id_active == -1)
        return -1;
    if (model->graph == NULL)
        return -1;

    if ((n_idx_selected = find_node_idx(model->graph, n_id_selected)) < 0)
        return -1;
    if ((n_idx_active = find_node_idx(model->graph, n_id_active)) < 0)
        return -1;
    n_selected = csfg_graph_get_node(model->graph, n_idx_selected);
    n_active   = csfg_graph_get_node(model->graph, n_idx_active);

    csfg_expr_pool_init(&pool);
    expr = csfg_expr_parse(&pool, cstr_view("1"));
    if (expr < 0)
        goto parse_failed;

    e_idx = csfg_graph_add_edge(
        model->graph, n_idx_selected, n_idx_active, pool, expr);
    if (e_idx < 0)
        goto add_edge_failed;

    e    = csfg_graph_get_edge(model->graph, e_idx);
    e->x = (n_active->x + n_selected->x) / 2;
    e->y = (n_active->y + n_selected->y) / 2;
    auto_position_edge(model->graph, e, n_selected, n_active);

    switch (edge_attr_hmap_emplace_or_get(&model->edge_attrs, e->id, &ea))
    {
        case HMAP_OOM   : goto add_edge_attr_failed;
        case HMAP_NEW   : edge_attr_init(ea, model->graph_color); /* fallthrough */
        case HMAP_EXISTS: csfg_expr_to_str(&ea->expr_str, pool, expr);
    }

    return e_idx;

add_edge_attr_failed:
    csfg_graph_mark_edge_deleted(model->graph, e_idx);
    csfg_graph_gc(model->graph);
add_edge_failed:
parse_failed:
    csfg_expr_pool_deinit(pool);
    return -1;
}

/* -------------------------------------------------------------------------- */
static int is_node_number_in_use(
    const struct graph_model* model, char unit, int node_number)
{
    struct csfg_node* n;
    if (model->graph == NULL)
        return 0;
    csfg_graph_for_each_node (model->graph, n)
    {
        struct strview sv = str_view(n->name);
        if (sv.len > 1 && sv.data[sv.off] == unit)
        {
            int number = strview_to_integer(strview(sv.data, 1, sv.len - 1));
            if (number == node_number)
                return 1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
static int find_unused_node_number(const struct graph_model* model, char unit)
{
    int node_number = 1;
    while (is_node_number_in_use(model, unit, node_number))
        node_number++;
    return node_number;
}

/* -------------------------------------------------------------------------- */
int create_new_node(struct graph_model* model, int n_id_prev)
{
    struct str* name;
    struct csfg_node* n;
    struct node_attr* na;
    const struct csfg_node* n_prev;
    int n_idx;

    if (model->graph == NULL)
        return -1;

    str_init(&name);
    if (str_fmt(&name, "V%d", find_unused_node_number(model, 'V')) != 0)
    {
        str_deinit(name);
        return -1;
    }

    n_idx = csfg_graph_add_node_steal_name(model->graph, name);
    if (n_idx < 0)
    {
        str_deinit(name);
        return -1;
    }

    n = csfg_graph_get_node(model->graph, n_idx);
    switch (node_attr_hmap_emplace_or_get(&model->node_attrs, n->id, &na))
    {
        case HMAP_OOM   : return -1;
        case HMAP_EXISTS: return -1;
        case HMAP_NEW   : node_attr_init(na, model->graph_color);
    }

    if (n_id_prev > -1)
    {
        n_prev = find_node(model->graph, n_id_prev);
        CSFG_DEBUG_ASSERT(n_prev != NULL);
        auto_position_node_near(model->graph, n, n_prev->x, n_prev->y);
    }
    else
        auto_position_node_near(model->graph, n, 0.0, 0.0);

    return n->id;
}

/* -------------------------------------------------------------------------- */
int delete_edge(struct graph_model* model, int edge_id)
{
    int e_idx;
    struct csfg_edge* e;

    if (model->graph == NULL)
        return -1;

    csfg_graph_enumerate_edges (model->graph, e_idx, e)
        if (e->id == edge_id)
            break;
    if (e_idx == csfg_graph_edge_count(model->graph))
        return -1;

    edge_attr_deinit(edge_attr_hmap_erase(model->edge_attrs, edge_id));
    csfg_graph_mark_edge_deleted(model->graph, e_idx);
    csfg_graph_gc(model->graph);

    return -1;
}

/* -------------------------------------------------------------------------- */
int delete_node(struct graph_model* model, int node_id)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    int e_idx, n_idx;
    int next_active_node_id = -1;

    if (model->graph == NULL)
        return -1;
    if (node_id < 0)
        return -1;

    /* Prefer following edges pointing to us */
    csfg_graph_enumerate_edges (model->graph, e_idx, e)
        if (csfg_graph_get_node(model->graph, e->n_idx_to)->id == node_id)
        {
            next_active_node_id =
                csfg_graph_get_node(model->graph, e->n_idx_from)->id;
            break;
        }
    if (next_active_node_id == -1)
        csfg_graph_enumerate_edges (model->graph, e_idx, e)
            if (csfg_graph_get_node(model->graph, e->n_idx_from)->id == node_id)
            {
                next_active_node_id =
                    csfg_graph_get_node(model->graph, e->n_idx_to)->id;
                break;
            }

    csfg_graph_enumerate_edges (model->graph, e_idx, e)
    {
        if (csfg_graph_get_node(model->graph, e->n_idx_from)->id == node_id ||
            csfg_graph_get_node(model->graph, e->n_idx_to)->id == node_id)
        {
            edge_attr_deinit(edge_attr_hmap_erase(model->edge_attrs, e->id));
            csfg_graph_mark_edge_deleted(model->graph, e_idx);
        }
    }

    csfg_graph_enumerate_nodes (model->graph, n_idx, n)
        if (n->id == node_id)
            break;
    if (n_idx != csfg_graph_node_count(model->graph))
    {
        node_attr_hmap_erase(model->node_attrs, node_id);
        csfg_graph_mark_node_deleted(model->graph, n_idx);
        csfg_graph_gc(model->graph);
    }

    /* Fall back to searching for the right-most node in the graph */
    if (next_active_node_id == -1)
        next_active_node_id =
            find_farthest_node_in_direction(model->graph, 1, 0);

    return next_active_node_id;
}

/* -------------------------------------------------------------------------- */
void delete_multi_selection_objects(struct graph_model* model)
{
    struct csfg_node *n1, *n2;
    struct csfg_edge* e;
    int i, n_idx, e_idx, need_graph_gc = 0;

    if (model->graph == NULL)
        return;

    csfg_graph_enumerate_edges (model->graph, e_idx, e)
    {
        n1 = csfg_graph_get_node(model->graph, e->n_idx_from);
        n2 = csfg_graph_get_node(model->graph, e->n_idx_to);
        if (edge_attr_hmap_find(model->edge_attrs, e->id)->selected ||
            node_attr_hmap_find(model->node_attrs, n1->id)->selected ||
            node_attr_hmap_find(model->node_attrs, n2->id)->selected)
        {
            edge_attr_deinit(edge_attr_hmap_erase(model->edge_attrs, e->id));
            csfg_graph_mark_edge_deleted(model->graph, e_idx);
            need_graph_gc = 1;
        }
    }

    csfg_graph_enumerate_nodes (model->graph, n_idx, n1)
        if (node_attr_hmap_find(model->node_attrs, n1->id)->selected)
        {
            node_attr_hmap_erase(model->node_attrs, n1->id);
            csfg_graph_mark_node_deleted(model->graph, n_idx);
            need_graph_gc = 1;
        }

    for (i = 0; i != vec_count(model->drawing);)
    {
        if (vec_get(model->drawing, i)->selected)
        {
            line_deinit(vec_get(model->drawing, i));
            line_vec_erase(model->drawing, i);
        }
        else
            i++;
    }

    if (need_graph_gc)
        csfg_graph_gc(model->graph);
}

/* -------------------------------------------------------------------------- */
void multi_select_nodes_and_lines(
    struct graph_model* model, int x1, int y1, int x2, int y2, int append)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    struct node_attr* na;
    struct line* line;
    struct point* point;
    double tmp;

    if (model->graph == NULL)
        return;

    if (x1 > x2)
        tmp = x1, x1 = x2, x2 = tmp;
    if (y1 > y2)
        tmp = y1, y1 = y2, y2 = tmp;

    csfg_graph_for_each_node (model->graph, n)
    {
        na = node_attr_hmap_find(model->node_attrs, n->id);
        if (x1 <= n->x + na->radius && x2 >= n->x - na->radius &&
            y1 <= n->y + na->radius && y2 >= n->y - na->radius)
        {
            na->selected = append;
        }
    }

    csfg_graph_for_each_edge (model->graph, e)
        if (x1 <= e->x + GRID / 2.0 && x2 >= e->x - GRID / 2.0 &&
            y1 <= e->y + GRID / 2.0 && y2 >= e->y - GRID / 2.0)
        {
            edge_attr_hmap_find(model->edge_attrs, e->id)->selected = append;
        }

    vec_for_each (model->drawing, line)
        vec_for_each (line->points, point)
            if (x1 <= point->x + GRID && x2 >= point->x - GRID &&
                y1 <= point->y + GRID && y2 >= point->y - GRID)
            {
                line->selected = append;
                break;
            }
}

/* -------------------------------------------------------------------------- */
void multi_deselect_all(struct graph_model* model)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    struct line* line;

    if (model->graph == NULL)
        return;

    csfg_graph_for_each_node (model->graph, n)
        node_attr_hmap_find(model->node_attrs, n->id)->selected = 0;
    csfg_graph_for_each_edge (model->graph, e)
        edge_attr_hmap_find(model->edge_attrs, e->id)->selected = 0;

    vec_for_each (model->drawing, line)
        line->selected = 0;
}

/* -------------------------------------------------------------------------- */
static int any_line_is_selected(const struct graph_model* model)
{
    const struct line* line;
    vec_for_each (model->drawing, line)
        if (line->selected)
            return 1;
    return 0;
}

/* -------------------------------------------------------------------------- */
void flip_active_edge(struct graph_model* model)
{
    int tmp;
    struct csfg_edge* e;

    e = find_edge(model->graph, model->active_edge_id);
    if (e == NULL)
        return;

    tmp           = e->n_idx_to;
    e->n_idx_to   = e->n_idx_from;
    e->n_idx_from = tmp;
}

/* -------------------------------------------------------------------------- */
void recolor_selected_objects(struct graph_model* model, struct color color)
{
    struct csfg_node *n1, *n2;
    struct csfg_edge* e;
    struct node_attr *na1, *na2;
    struct edge_attr* ea;
    struct line* line;

    if (model->graph == NULL)
        return;

    csfg_graph_for_each_node (model->graph, n1)
    {
        na1 = node_attr_hmap_find(model->node_attrs, n1->id);
        if (n1->id == model->active_node_id || na1->selected)
            na1->color = color;
    }

    csfg_graph_for_each_edge (model->graph, e)
    {
        n1  = csfg_graph_get_node(model->graph, e->n_idx_from);
        n2  = csfg_graph_get_node(model->graph, e->n_idx_to);
        na1 = node_attr_hmap_find(model->node_attrs, n1->id);
        na2 = node_attr_hmap_find(model->node_attrs, n2->id);
        ea  = edge_attr_hmap_find(model->edge_attrs, e->id);

        if (e->id == model->active_edge_id || ea->selected)
            ea->color = color;

        if (n1->id == model->active_node_id || na1->selected)
            na1->color = color;
        if (n2->id == model->active_node_id || na2->selected)
            na2->color = color;

        if ((n1->id == model->active_node_id || na1->selected) &&
            (n2->id == model->active_node_id || na2->selected))
        {
            ea->color = color;
        }
    }

    vec_for_each (model->drawing, line)
        if (line->selected)
            line->color = color;
}

/* -------------------------------------------------------------------------- */
void notify_graph_changed(struct graph_model* model)
{
    int node_in_idx, node_out_idx;
    const struct csfg_node* n;

    if (model->graph == NULL)
        return;

    csfg_graph_enumerate_nodes (model->graph, node_in_idx, n)
        if (n->id == model->node_in_id)
            break;
    csfg_graph_enumerate_nodes (model->graph, node_out_idx, n)
        if (n->id == model->node_out_id)
            break;

    if (node_in_idx == csfg_graph_node_count(model->graph))
        node_in_idx = -1;
    if (node_out_idx == csfg_graph_node_count(model->graph))
        node_out_idx = -1;

    model->icb->graph_structure_changed(
        model->cb, model->plugin_ctx, node_in_idx, node_out_idx);
}

/* -------------------------------------------------------------------------- */
void graph_model_rebuild_graph(
    struct graph_model* model, int node_in, int node_out)
{
    int i, id;
    struct csfg_node* n;
    struct csfg_edge* e;
    struct node_attr* na;
    struct edge_attr* ea;

    if (model->graph == NULL)
        return;

    model->node_in_id  = -1;
    model->node_out_id = -1;
    csfg_graph_enumerate_nodes (model->graph, i, n)
    {
        if (i == node_in)
            model->node_in_id = n->id;
        if (i == node_out)
            model->node_out_id = n->id;
    }

    csfg_graph_enumerate_nodes (model->graph, i, n)
        switch (node_attr_hmap_emplace_or_get(&model->node_attrs, n->id, &na))
        {
            case HMAP_OOM   : return;
            case HMAP_NEW   : node_attr_init(na, model->graph_color);
            case HMAP_EXISTS: break;
        }

    csfg_graph_for_each_edge (model->graph, e)
        switch (edge_attr_hmap_emplace_or_get(&model->edge_attrs, e->id, &ea))
        {
            case HMAP_OOM: return;
            case HMAP_NEW:
                edge_attr_init(ea, model->graph_color); /* fallthrough */
            case HMAP_EXISTS: csfg_expr_to_str(&ea->expr_str, e->pool, e->expr);
        }

    hmap_for_each (model->node_attrs, i, id, na)
        if (find_node(model->graph, id) == NULL)
            node_attr_hmap_erase_slot(model->node_attrs, i);
    hmap_for_each (model->edge_attrs, i, id, ea)
        if (find_edge(model->graph, id) == NULL)
            edge_attr_deinit(edge_attr_hmap_erase_slot(model->edge_attrs, i));

    if (find_node(model->graph, model->active_node_id) == NULL)
        model->active_node_id = -1;
    if (find_edge(model->graph, model->active_edge_id) == NULL)
        model->active_edge_id = -1;
}

/* -------------------------------------------------------------------------- */
void graph_model_init(
    struct graph_model* model,
    struct plugin_ctx* plugin_ctx,
    const struct plugin_notify_interface* icb,
    struct plugin_notify_context* cb)
{
    model->icb        = icb;
    model->cb         = cb;
    model->plugin_ctx = plugin_ctx;

    model->graph = NULL;
    node_attr_hmap_init(&model->node_attrs);
    edge_attr_hmap_init(&model->edge_attrs);

    undo_stack_vec_init(&model->undo_stack);
    model->undo_stack_ptr = -1;

    line_vec_init(&model->drawing);

    model->graph_color = hex_rgb(0x333333);
    model->draw_color  = hex_rgb(0x333333);

    model->node_in_id        = -1;
    model->node_out_id       = -1;
    model->active_node_id    = -1;
    model->active_edge_id    = -1;
    model->marked_node_id    = -1;
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;

    model->mode = MODE_NORMAL;
}

/* -------------------------------------------------------------------------- */
void graph_model_deinit(struct graph_model* model)
{
    int idx, id;
    struct edge_attr* ea;
    struct serializer** ser;
    struct line* line;

    vec_for_each (model->drawing, line)
        line_deinit(line);
    line_vec_deinit(model->drawing);

    hmap_for_each (model->edge_attrs, idx, id, ea)
        (void)id, edge_attr_deinit(ea);
    edge_attr_hmap_deinit(model->edge_attrs);
    node_attr_hmap_deinit(model->node_attrs);

    vec_for_each (model->undo_stack, ser)
        serializer_deinit(*ser);
    undo_stack_vec_deinit(model->undo_stack);
}

/* -------------------------------------------------------------------------- */
void graph_model_set_graph(
    struct graph_model* model, struct csfg_graph* g, int node_in, int node_out)
{
    model->graph = g;
    graph_model_rebuild_graph(model, node_in, node_out);
}

/* -------------------------------------------------------------------------- */
void graph_model_clear_graph(struct graph_model* model)
{
    model->graph = NULL;
    graph_model_rebuild_graph(model, -1, -1);
}
