#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"
#include "dpsfg-plugin.h"
#include "graph-editor/graph_editor.h"
#include <math.h>

static const int GRID_WIDTH              = 20;
static const int ARROW_RADIUS            = 8;
static const double DEFAULT_NODE_SPACING = GRID_WIDTH * 6;
static int DEFAULT_NODE_RADIUS           = 10;

enum mode
{
    MODE_NORMAL,
    MODE_MOVE,
    MODE_RECONNECT_FROM,
    MODE_RECONNECT_TO,
};

HMAP_DEFINE(extern, node_attr_hmap, int, struct node_attr, 16)
HMAP_DEFINE(extern, edge_attr_hmap, int, struct edge_attr, 16)

struct graph_model
{
    const struct plugin_notify_interface* icb;
    struct plugin_notify_context* cb;
    struct plugin_ctx* plugin_ctx;

    struct csfg_graph* graph;
    struct node_attr_hmap* node_attrs;
    struct edge_attr_hmap* edge_attrs;

    int active_node_id;
    int active_edge_id;
    int marked_node_id;
    int reconnect_node_id;
    int reconnect_edge_id;
    int node_in_id;
    int node_out_id;

    enum mode mode;
};

struct _GraphEditor
{
    GtkBox parent_instance;

    /* drawing area is a child of "overlay". We use overlay to create GtkEntry's
     * on top of the drawing area so the user can enter formulas and give names
     * to nodes */
    GtkWidget* overlay;
    GtkWidget* drawing_area;
    GtkWidget* entry; /* Text entry, when active */

    /* It's difficult to get the mouse coordinates in some callbacks, so we use
     * GtkEventControllerMotion to store them here */
    double mouse_x, mouse_y;

    /* View transform */
    double zoom;
    double pan_x, pan_y;
    double pan_offset_x, pan_offset_y;

    /* Drag state */
    double drag_offset_x, drag_offset_y;

    struct graph_model model;
};

G_DEFINE_DYNAMIC_TYPE(GraphEditor, graph_editor, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
void edge_attr_deinit(struct edge_attr* ea)
{
    str_deinit(ea->expr_str);
}

/* -------------------------------------------------------------------------- */
static int find_farthest_node_in_direction(
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
static int find_closest_node_in_direction(
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
static int find_farthest_edge_in_direction(
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
static int find_closest_edge_in_direction(
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
static int calc_circle(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3)
{
    double dx   = x1 - x3;
    double dy   = y1 - y3;
    double dist = sqrt(dx * dx + dy * dy);

    /* Circumcenter */
    double A   = x1 - x2;
    double B   = y1 - y2;
    double C   = x1 - x3;
    double D   = y1 - y3;
    double E   = ((x1 * x1 - x2 * x2) + (y1 * y1 - y2 * y2)) / 2.0;
    double F   = ((x1 * x1 - x3 * x3) + (y1 * y1 - y3 * y3)) / 2.0;
    double den = A * D - B * C;
    if (fabs(den) < 0.0001)
    {
        *cx = (x1 + x3) / 2.0;
        *cy = (y1 + y3) / 2.0;
        return 0;
    }
    *cx     = (D * E - B * F) / den;
    *cy     = (-C * E + A * F) / den;
    *radius = hypot(*cx - x1, *cy - y1);
    if (*radius > 100 * dist)
        return 0;

    double vx1 = (x2 - x1), vy1 = (y2 - y1);
    double vx2 = (x3 - x1), vy2 = (y3 - y1);
    double cross = (vx1 * vy2 - vy1 * vx2);
    return cross > 0 ? 1 : -1;
}

/* -------------------------------------------------------------------------- */
static void
drag_edge_with_node(struct csfg_graph* g, int n_id, double new_x, double new_y)
{
    const struct csfg_node* n;
    struct csfg_edge* e;
    double cx, cy, radius;
    int n_idx, orientation;

    if (g == NULL)
        return;

    csfg_graph_enumerate_nodes (g, n_idx, n)
        if (n->id == n_id)
            break;
    if (n_idx == csfg_graph_node_count(g))
        return;

    csfg_graph_for_each_edge (g, e)
    {
        const struct csfg_node* n1 = csfg_graph_get_node(g, n_idx);
        const struct csfg_node* n2 =
            e->n_idx_from == n_idx ? csfg_graph_get_node(g, e->n_idx_to)
            : e->n_idx_to == n_idx ? csfg_graph_get_node(g, e->n_idx_from)
                                   : NULL;
        if (n2 == NULL)
            continue;

        orientation = calc_circle(
            &cx, &cy, &radius, n1->x, n1->y, e->x, e->y, n2->x, n2->y);
        if (orientation == 0)
        {
            e->x = (new_x + n2->x) / 2;
            e->y = (new_y + n2->y) / 2;
        }
        else
        {
            e->x += (new_x - n1->x) / 2;
            e->y += (new_y - n1->y) / 2;
        }
    }
}

/* -------------------------------------------------------------------------- */
static int find_node_idx(struct csfg_graph* g, int node_id)
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
static struct csfg_node* find_node(struct csfg_graph* g, int node_id)
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
static int find_edge_idx(struct csfg_graph* g, int edge_id)
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
static struct csfg_edge* find_edge(struct csfg_graph* g, int edge_id)
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
static void select_next_active_node_in_direction(
    struct graph_model* model, double dx, double dy)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    n = find_node(model->graph, model->active_node_id);
    e = find_edge(model->graph, model->active_edge_id);

    if (n != NULL)
        model->active_node_id = find_closest_node_in_direction(
            model->graph, model->active_node_id, dx, dy, n->x, n->y);
    else if (e != NULL)
        model->active_node_id = find_closest_node_in_direction(
            model->graph, model->active_node_id, dx, dy, e->x, e->y);
    else
        model->active_node_id =
            find_farthest_node_in_direction(model->graph, dx, dy);

    if (model->active_node_id > -1)
        model->active_edge_id = -1;
}

/* -------------------------------------------------------------------------- */
static void select_next_active_edge_in_direction(
    struct graph_model* model, double dx, double dy)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    n = find_node(model->graph, model->active_node_id);
    e = find_edge(model->graph, model->active_edge_id);

    if (e != NULL)
        model->active_edge_id = find_closest_edge_in_direction(
            model->graph, model->active_edge_id, dx, dy, e->x, e->y);
    else if (n != NULL)
        model->active_edge_id = find_closest_edge_in_direction(
            model->graph, model->active_edge_id, dx, dy, n->x, n->y);
    else
        model->active_edge_id = find_farthest_edge_in_direction(
            model->graph, model->active_edge_id, dx, dy);

    if (model->active_edge_id > -1)
        model->active_node_id = -1;
}

/* -------------------------------------------------------------------------- */
static void select_next_connected_node_in_direction(
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
static void select_next_connected_edge_in_direction(
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
static int reconnect_edge(
    struct graph_model* model,
    int edge_id,
    int current_node_id,
    int target_node_id)
{
    struct csfg_edge* e  = find_edge(model->graph, edge_id);
    int current_node_idx = find_node_idx(model->graph, current_node_id);
    int target_node_idx  = find_node_idx(model->graph, target_node_id);
    if (e == NULL || current_node_idx < 0 || target_node_idx < 0)
        return current_node_id;

    if (e->n_idx_to == current_node_idx)
    {
        e->n_idx_to = target_node_idx;
        return target_node_id;
    }
    if (e->n_idx_from == current_node_idx)
    {
        e->n_idx_from = target_node_idx;
        return target_node_id;
    }

    return current_node_id;
}

/* -------------------------------------------------------------------------- */
static void
move_active_node_in_direction(struct graph_model* model, double dx, double dy)
{
    struct csfg_node* n = find_node(model->graph, model->active_node_id);
    if (n == NULL)
        return;

    drag_edge_with_node(
        model->graph,
        model->active_node_id,
        n->x + dx * DEFAULT_NODE_SPACING,
        n->y + dy * DEFAULT_NODE_SPACING);
    n->x += dx * DEFAULT_NODE_SPACING;
    n->y += dy * DEFAULT_NODE_SPACING;

    model->icb->graph_layout_changed(model->cb, model->plugin_ctx);
}

/* -------------------------------------------------------------------------- */
static void
move_active_edge_in_direction(struct graph_model* model, double dx, double dy)
{
    struct csfg_edge* e = find_edge(model->graph, model->active_edge_id);
    if (e == NULL)
        return;

    e->x += dx * GRID_WIDTH;
    e->y += dy * GRID_WIDTH;

    model->icb->graph_layout_changed(model->cb, model->plugin_ctx);
}

/* -------------------------------------------------------------------------- */
static int is_near_any_other_node(
    double x,
    double y,
    const struct csfg_graph* g,
    const struct csfg_node* exclude_node)
{
    const struct csfg_node* n;
    CSFG_DEBUG_ASSERT(g != NULL);
    csfg_graph_for_each_node (g, n)
    {
        double dx = x - n->x;
        double dy = y - n->y;
        if (exclude_node != NULL && n == exclude_node)
            continue;
        if (dx * dx + dy * dy < DEFAULT_NODE_SPACING * DEFAULT_NODE_SPACING / 4)
            return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int is_near_any_other_edge(
    double x,
    double y,
    const struct csfg_graph* g,
    const struct csfg_edge* exclude_edge)
{
    const struct csfg_edge* e;
    CSFG_DEBUG_ASSERT(g != NULL);
    csfg_graph_for_each_edge (g, e)
    {
        double dx = x - e->x;
        double dy = y - e->y;
        if (exclude_edge != NULL && e == exclude_edge)
            continue;
        if (dx * dx + dy * dy < DEFAULT_NODE_SPACING * DEFAULT_NODE_SPACING / 4)
            return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static void auto_position_node_grid(
    const struct csfg_graph* g, struct csfg_node* n, int total_node_count)
{
    const double row_end =
        (int)ceil(sqrt(total_node_count)) * DEFAULT_NODE_SPACING;
    CSFG_DEBUG_ASSERT(g != NULL);

    n->x = 0.0;
    n->y = 0.0;
    while (is_near_any_other_node(n->x, n->y, g, n))
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
static void auto_position_node_near(
    const struct csfg_graph* g,
    struct csfg_node* n,
    double near_x,
    double near_y)
{
    CSFG_DEBUG_ASSERT(g != NULL);
    n->x = near_x;
    n->y = near_y;
    while (is_near_any_other_node(n->x, n->y, g, n))
        n->x += DEFAULT_NODE_SPACING;
}

/* -------------------------------------------------------------------------- */
static void auto_position_edge(
    const struct csfg_graph* g,
    struct csfg_edge* e,
    const struct csfg_node* from,
    const struct csfg_node* to)
{
    CSFG_DEBUG_ASSERT(g != NULL);

    e->x = (from->x + to->x) / 2;
    e->y = (from->y + to->y) / 2;

    double spacing_increments = DEFAULT_NODE_SPACING / 2;
    double edge_add_spacing   = spacing_increments;

    while (is_near_any_other_node(e->x, e->y, g, NULL) ||
           is_near_any_other_edge(e->x, e->y, g, e))
    {
        double tmp;
        double nx  = from->x - to->x;
        double ny  = from->y - to->y;
        double den = sqrt(nx * nx + ny * ny);
        tmp        = nx;
        nx         = -ny / den;
        ny         = tmp / den;

        e->x -= nx * edge_add_spacing;
        e->y -= ny * edge_add_spacing;
        edge_add_spacing = edge_add_spacing > 0
                             ? -(edge_add_spacing + spacing_increments)
                             : -edge_add_spacing + spacing_increments;
    }
}

/* -------------------------------------------------------------------------- */
static int
create_new_edge(struct graph_model* model, int n_id_selected, int n_id_active)
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

    csfg_graph_enumerate_nodes (model->graph, n_idx_selected, n_selected)
        if (n_selected->id == n_id_selected)
            break;
    if (n_idx_selected == csfg_graph_node_count(model->graph))
        return -1;

    csfg_graph_enumerate_nodes (model->graph, n_idx_active, n_active)
        if (n_active->id == n_id_active)
            break;
    if (n_idx_active == csfg_graph_node_count(model->graph))
        return -1;

    csfg_expr_pool_init(&pool);
    expr  = csfg_expr_parse(&pool, cstr_view("1"));
    e_idx = csfg_graph_add_edge(
        model->graph, n_idx_selected, n_idx_active, pool, expr);
    e  = csfg_graph_get_edge(model->graph, e_idx);
    ea = edge_attr_hmap_emplace_new(&model->edge_attrs, e->id);

    e->x = (n_active->x + n_selected->x) / 2;
    e->y = (n_active->y + n_selected->y) / 2;
    str_init(&ea->expr_str);
    csfg_expr_to_str(&ea->expr_str, pool, expr);

    auto_position_edge(model->graph, e, n_selected, n_active);

    return e_idx;
}

/* -------------------------------------------------------------------------- */
static int create_new_node(struct graph_model* model, int n_id_prev)
{
    struct csfg_node* n;
    struct node_attr* na;
    const struct csfg_node* n_prev;
    int n_idx;

    if (model->graph == NULL)
        return -1;

    n_idx      = csfg_graph_add_node(model->graph, "V1");
    n          = csfg_graph_get_node(model->graph, n_idx);
    na         = node_attr_hmap_emplace_new(&model->node_attrs, n->id);
    na->radius = DEFAULT_NODE_RADIUS;

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
static int delete_edge(struct graph_model* model, int edge_id)
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
static int delete_node(struct graph_model* model, int node_id)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    int e_idx, n_idx;
    int next_active_node_id = -1;

    if (model->graph == NULL)
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
static int try_select_node(
    double mouse_x,
    double mouse_y,
    const struct csfg_graph* graph,
    const struct node_attr_hmap* attrs)
{
    double dx, dy;
    int idx;
    const struct csfg_node* n;

    csfg_graph_enumerate_nodes (graph, idx, n)
    {
        const struct node_attr* na = node_attr_hmap_find(attrs, n->id);
        if (na == NULL)
            continue;

        dx = mouse_x - n->x;
        dy = mouse_y - n->y;
        if (dx * dx + dy * dy < na->radius * na->radius)
            return n->id;
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
static int
try_select_edge(double mouse_x, double mouse_y, const struct csfg_graph* graph)
{
    double dx, dy;
    int idx;
    const struct csfg_edge* e;

    csfg_graph_enumerate_edges (graph, idx, e)
    {
        dx = mouse_x - e->x;
        dy = mouse_y - e->y;
        if (dx * dx + dy * dy < ARROW_RADIUS * ARROW_RADIUS)
            return e->id;
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
static void
try_select_edge_or_node(struct graph_model* model, double x, double y)
{
    model->active_node_id =
        try_select_node(x, y, model->graph, model->node_attrs);
    model->active_edge_id = try_select_edge(x, y, model->graph);

    /* Edge has priority when clicking with the mouse -- feels better */
    if (model->active_edge_id > -1)
        model->active_node_id = -1;
}

/* -------------------------------------------------------------------------- */
static void notify_graph_changed(struct graph_model* model)
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
void rebuild_graph(struct graph_model* model, int node_in, int node_out)
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
            case HMAP_NEW   : na->radius = DEFAULT_NODE_RADIUS;
            case HMAP_EXISTS: break;
        }

    csfg_graph_for_each_edge (model->graph, e)
        switch (edge_attr_hmap_emplace_or_get(&model->edge_attrs, e->id, &ea))
        {
            case HMAP_OOM: return;
            case HMAP_NEW: {
                str_init(&ea->expr_str);
                csfg_expr_to_str(&ea->expr_str, e->pool, e->expr);
            }
            case HMAP_EXISTS: break;
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
static void finish_editing(GtkEntry* entry, gpointer user_data)
{
    int n_idx;
    struct csfg_node* n;
    struct csfg_edge* e;
    struct edge_attr* ea;
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;

    const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));

    if (model->active_node_id > -1)
    {
        csfg_graph_enumerate_nodes (model->graph, n_idx, n)
            if (n->id == model->active_node_id)
            {
                str_set_cstr(&n->name, text);
                break;
            }
    }
    else if (model->active_edge_id > -1)
    {
        ea = edge_attr_hmap_find(model->edge_attrs, model->active_edge_id);
        str_set_cstr(&ea->expr_str, text);

        csfg_graph_for_each_edge (model->graph, e)
            if (e->id == model->active_edge_id)
            {
                csfg_expr_pool_clear(e->pool);
                e->expr = csfg_expr_parse(&e->pool, cstr_view(text));
            }
    }

    gtk_widget_unparent(editor->entry);
    editor->entry = NULL;

    notify_graph_changed(model);
    gtk_widget_queue_draw(editor->drawing_area);
}
static void start_editing(
    GraphEditor* editor,
    double x,
    double y,
    double radius,
    const char* current_name)
{
    if (editor->entry)
        return; // Already editing

    editor->entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(editor->entry), current_name);

    gtk_widget_set_hexpand(editor->entry, FALSE);
    gtk_widget_set_vexpand(editor->entry, FALSE);
    gtk_widget_set_halign(editor->entry, GTK_ALIGN_START);
    gtk_widget_set_valign(editor->entry, GTK_ALIGN_START);

    int ex = (int)(x * editor->zoom + editor->pan_x);
    int ey = (int)(y * editor->zoom + editor->pan_y);
    gtk_widget_set_margin_start(editor->entry, ex);
    gtk_widget_set_margin_top(editor->entry, ey);
    gtk_widget_set_size_request(editor->entry, radius, radius);

    gtk_overlay_add_overlay(GTK_OVERLAY(editor->overlay), editor->entry);
    gtk_widget_set_visible(editor->entry, TRUE);
    gtk_widget_grab_focus(editor->entry);

    g_signal_connect(
        editor->entry, "activate", G_CALLBACK(finish_editing), editor);
}
static void
on_entry_focus_changed(GObject* obj, GParamSpec* pspec, gpointer user_data)
{
    GtkWidget* entry = GTK_WIDGET(obj);
    (void)pspec;
    if (!gtk_widget_has_focus(entry))
        finish_editing(GTK_ENTRY(entry), user_data);
}
static void start_editing_active_node_or_edge(GraphEditor* editor)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    const struct node_attr* na;
    const struct edge_attr* ea;
    struct graph_model* model = &editor->model;

    if ((n = find_node(model->graph, model->active_node_id)) != NULL)
    {
        na = node_attr_hmap_find(model->node_attrs, n->id);
        if (na != NULL)
            start_editing(editor, n->x, n->y, na->radius, str_cstr(n->name));
    }
    else if ((e = find_edge(model->graph, model->active_edge_id)) != NULL)
    {
        ea = edge_attr_hmap_find(model->edge_attrs, model->active_edge_id);
        if (ea != NULL)
            start_editing(editor, e->x, e->y, 10, str_cstr(ea->expr_str));
    }

    model->mode = MODE_NORMAL;
}

/* -------------------------------------------------------------------------- */
static void
node_direction_action(struct graph_model* model, double dx, double dy)
{
    switch (model->mode)
    {
        case MODE_NORMAL:
            select_next_active_node_in_direction(model, dx, dy);
            break;
        case MODE_MOVE:
            move_active_node_in_direction(model, dx, dy);
            move_active_edge_in_direction(model, dx, dy);
            break;
        case MODE_RECONNECT_FROM:
            select_next_connected_node_in_direction(
                model, model->reconnect_edge_id, dx, dy);
            select_next_connected_edge_in_direction(
                model, model->reconnect_node_id, dx, dy);
            break;
        case MODE_RECONNECT_TO:
            select_next_active_node_in_direction(model, dx, dy);
            model->reconnect_node_id = reconnect_edge(
                model,
                model->reconnect_edge_id,
                model->reconnect_node_id,
                model->active_node_id);
            break;
    }
}
static void
edge_direction_action(struct graph_model* model, double dx, double dy)
{
    switch (model->mode)
    {
        case MODE_NORMAL:
            select_next_active_edge_in_direction(model, dx, dy);
            break;
        case MODE_MOVE:
            move_active_edge_in_direction(model, dx, dy);
            move_active_node_in_direction(model, dx, dy);
            break;
        case MODE_RECONNECT_FROM:
            select_next_connected_node_in_direction(
                model, model->reconnect_node_id, dx, dy);
            select_next_connected_edge_in_direction(
                model, model->reconnect_node_id, dx, dy);
        case MODE_RECONNECT_TO: break;
    }
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_node_left_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    node_direction_action(&editor->model, -1, 0);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_node_left_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_node_left_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_node_right_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    node_direction_action(&editor->model, 1, 0);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_node_right_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_node_right_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_node_up_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    node_direction_action(&editor->model, 0, -1);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_node_up_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_node_up_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_node_down_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    node_direction_action(&editor->model, 0, 1);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_node_down_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_node_down_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_edge_left_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    edge_direction_action(&editor->model, -1, 0);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_edge_left_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_edge_left_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_edge_right_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    edge_direction_action(&editor->model, 1, 0);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_edge_right_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_edge_right_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_edge_up_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    edge_direction_action(&editor->model, 0, -1);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_edge_up_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_edge_up_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_edge_down_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    edge_direction_action(&editor->model, 0, 1);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_edge_down_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_edge_down_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_move_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)widget, (void)unused;
    switch (model->mode)
    {
        case MODE_NORMAL: model->mode = MODE_MOVE; break;
        case MODE_MOVE  : model->mode = MODE_NORMAL; break;

        case MODE_RECONNECT_FROM:
            model->reconnect_node_id = -1;
            model->reconnect_edge_id = -1;
            model->mode              = MODE_MOVE;
            break;
        case MODE_RECONNECT_TO: break;
    }

    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_switch_mode_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_move_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_new_node_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)widget, (void)unused;
    model->active_node_id =
        create_new_node(&editor->model, model->active_node_id);
    model->active_edge_id    = -1;
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;
    notify_graph_changed(model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_new_node_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_new_node_cb(NULL, NULL, user_data);
}

static gboolean shortcut_new_node_and_edge_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    int prev_active_node_id   = model->active_node_id;
    (void)widget, (void)unused;
    model->active_node_id =
        create_new_node(&editor->model, model->active_node_id);
    model->active_edge_id    = -1;
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;
    create_new_edge(&editor->model, prev_active_node_id, model->active_node_id);
    notify_graph_changed(model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_new_node_and_edge_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_new_node_and_edge_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_delete_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)widget, (void)unused;
    editor->model.active_node_id =
        delete_node(model, editor->model.active_node_id);
    editor->model.active_edge_id =
        delete_edge(model, editor->model.active_edge_id);
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;
    notify_graph_changed(model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_delete_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_delete_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_mark_node_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)widget, (void)unused;
    if (model->marked_node_id == model->active_node_id)
        model->marked_node_id = -1;
    else
        model->marked_node_id = model->active_node_id;
    model->mode = MODE_NORMAL;
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_mark_node_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_mark_node_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_new_edge_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)widget, (void)unused;
    create_new_edge(model, model->marked_node_id, model->active_node_id);
    notify_graph_changed(model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_new_edge_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_new_edge_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_set_in_node_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    struct graph_model* model = &editor->model;
    model->node_in_id         = model->active_node_id;
    notify_graph_changed(model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_set_in_node_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_set_in_node_cb(NULL, NULL, user_data);
}

static gboolean shortcut_set_out_node_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)widget, (void)unused;
    model->node_out_id = model->active_node_id;
    notify_graph_changed(model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_set_out_node_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_set_out_node_cb(NULL, NULL, user_data);
}

static gboolean shortcut_edit_node_or_edge_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    (void)widget, (void)unused;
    start_editing_active_node_or_edge(user_data);
    return TRUE;
}
static void button_edit_node_or_edge_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_edit_node_or_edge_cb(NULL, NULL, user_data);
}

static gboolean shortcut_reconnect_edge_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)widget, (void)unused;
    switch (editor->model.mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
            model->reconnect_node_id = -1;
            model->reconnect_edge_id = -1;
            if (model->active_node_id > -1)
                model->reconnect_node_id = model->active_node_id;
            else if (model->active_edge_id > -1)
                model->reconnect_edge_id = model->active_edge_id;
            else
                break;

            model->mode = MODE_RECONNECT_FROM;
            break;

        case MODE_RECONNECT_FROM:
            if (model->reconnect_node_id > -1)
                model->reconnect_edge_id = model->active_edge_id;
            else
                model->reconnect_node_id = model->active_node_id;

            if (model->reconnect_node_id > -1 && model->reconnect_edge_id > -1)
                model->mode = MODE_RECONNECT_TO;
            else
                model->mode = MODE_NORMAL;
            break;

        case MODE_RECONNECT_TO:
            model->reconnect_node_id = -1;
            model->reconnect_edge_id = -1;
            model->mode              = MODE_NORMAL;
            break;
    }
    return TRUE;
}
static void button_reconnect_edge_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_edit_node_or_edge_cb(NULL, NULL, user_data);
}

/* -------------------------------------------------------------------------- */
static void setup_global_shortcuts(GraphEditor* editor, GtkBox* toolbar)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction* action;
    GtkWidget* button;
    GtkShortcut* shortcut;
    int i;

    struct
    {
        guint keyval;
        GdkModifierType modifiers;
        gboolean (*callback)(GtkWidget*, GVariant*, gpointer);
    } shortcuts[] = {
        /* clang-format off */
        {GDK_KEY_h, GDK_NO_MODIFIER_MASK, shortcut_node_left_cb},
        {GDK_KEY_l, GDK_NO_MODIFIER_MASK, shortcut_node_right_cb},
        {GDK_KEY_j, GDK_NO_MODIFIER_MASK, shortcut_node_down_cb},
        {GDK_KEY_k, GDK_NO_MODIFIER_MASK, shortcut_node_up_cb},
        {GDK_KEY_h, GDK_SHIFT_MASK,       shortcut_edge_left_cb},
        {GDK_KEY_l, GDK_SHIFT_MASK,       shortcut_edge_right_cb},
        {GDK_KEY_j, GDK_SHIFT_MASK,       shortcut_edge_down_cb},
        {GDK_KEY_k, GDK_SHIFT_MASK,       shortcut_edge_up_cb},
        {GDK_KEY_m, GDK_NO_MODIFIER_MASK, shortcut_move_cb},
        {GDK_KEY_m, GDK_SHIFT_MASK,       shortcut_move_cb},
        {GDK_KEY_n, GDK_NO_MODIFIER_MASK, shortcut_new_node_cb},
        {GDK_KEY_e, GDK_NO_MODIFIER_MASK, shortcut_new_edge_cb},
        {GDK_KEY_n, GDK_SHIFT_MASK,       shortcut_new_node_and_edge_cb},
        {GDK_KEY_x, GDK_NO_MODIFIER_MASK, shortcut_delete_cb},
        {GDK_KEY_s, GDK_NO_MODIFIER_MASK, shortcut_mark_node_cb},
        {GDK_KEY_i, GDK_NO_MODIFIER_MASK, shortcut_set_in_node_cb},
        {GDK_KEY_o, GDK_NO_MODIFIER_MASK, shortcut_set_out_node_cb},
        {GDK_KEY_i, GDK_SHIFT_MASK,       shortcut_edit_node_or_edge_cb},
        {GDK_KEY_r, GDK_NO_MODIFIER_MASK, shortcut_reconnect_edge_cb},
        /* clang-format on */
    };

    struct
    {
        const char* tooltip;
        const char* icon;
        void (*callback)(GtkButton*, gpointer);
    } buttons[] = {
        /* clang-format off */
        {"New Node\n"                      "Shortcut: n",
         "/ch/thecomet/dpsfg/graph-editor/icons/new-node.svg",
         button_new_node_cb},
        {"New Connected Node\n"            "Shortcut: Shift+n",
         "/ch/thecomet/dpsfg/graph-editor/icons/new-node-with-edge.svg",
         button_new_node_and_edge_cb},
        {"New Edge\n"                      "Shortcut: e",
         "/ch/thecomet/dpsfg/graph-editor/icons/new-edge.svg",
         button_new_edge_cb},
        {"Mark Node (for connections)\n"   "Shortcut: s",
         "/ch/thecomet/dpsfg/graph-editor/icons/mark-node.svg",
         button_mark_node_cb},
        {"Set Input Node\n"                "Shortcut: i",
         "/ch/thecomet/dpsfg/graph-editor/icons/input-node.svg",
         button_set_in_node_cb},
        {"Set Output Node\n"               "Shortcut: o",
         "/ch/thecomet/dpsfg/graph-editor/icons/output-node.svg",
         button_set_out_node_cb},
        {"Delete\n"                        "Shortcut: x",
         "/ch/thecomet/dpsfg/graph-editor/icons/delete.svg",
         button_delete_cb},
        /* clang-format on */
    };

    controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(
        GTK_SHORTCUT_CONTROLLER(controller), GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(editor->drawing_area, controller);

    for (i = 0; i != G_N_ELEMENTS(shortcuts); ++i)
    {
        trigger =
            gtk_keyval_trigger_new(shortcuts[i].keyval, shortcuts[i].modifiers);
        action   = gtk_callback_action_new(shortcuts[i].callback, editor, NULL);
        shortcut = gtk_shortcut_new(trigger, action);
        gtk_shortcut_controller_add_shortcut(
            GTK_SHORTCUT_CONTROLLER(controller), shortcut);
    }

    for (i = 0; i != G_N_ELEMENTS(buttons); ++i)
    {
        GtkWidget* image = gtk_image_new_from_resource(buttons[i].icon);
        button           = gtk_button_new();
        gtk_button_set_child(GTK_BUTTON(button), image);
        gtk_widget_set_tooltip_text(button, buttons[i].tooltip);
        gtk_box_append(toolbar, button);
        g_signal_connect(
            button, "clicked", G_CALLBACK(buttons[i].callback), editor);
    }
}

/* -------------------------------------------------------------------------- */
static void draw_grid_with_size(
    cairo_t* cr,
    double pan_x,
    double pan_y,
    int width,
    int height,
    int grid_width,
    double color)
{
    int from_x = -(int)pan_x / grid_width * grid_width - grid_width;
    int from_y = -(int)pan_y / grid_width * grid_width - grid_width;
    int to_x   = from_x + width + grid_width;
    int to_y   = from_y + height + grid_width;

    cairo_set_source_rgb(cr, color, color, color);
    for (int x = from_x; x < to_x; x += grid_width)
    {
        cairo_move_to(cr, x, from_y);
        cairo_line_to(cr, x, to_y);
    }
    for (int y = from_y; y < to_y; y += grid_width)
    {
        cairo_move_to(cr, from_x, y);
        cairo_line_to(cr, to_x, y);
    }
    cairo_stroke(cr);
}
static void draw_grid(
    cairo_t* cr, double pan_x, double pan_y, int width, int height, double zoom)
{
    pan_x /= zoom;
    pan_y /= zoom;
    width /= zoom;
    height /= zoom;

    draw_grid_with_size(cr, pan_x, pan_y, width, height, GRID_WIDTH, 0.8);
    draw_grid_with_size(cr, pan_x, pan_y, width, height, GRID_WIDTH * 6, 0.7);
}

/* -------------------------------------------------------------------------- */
static void draw_text(
    cairo_t* cr, const char* text, double x, double y, double offset_angle)
{
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text, -1);
    PangoFontDescription* desc = pango_font_description_from_string("Sans 12");
    pango_layout_set_font_description(layout, desc);

    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    double tx = x - tw / 2.0;
    double ty = y - th / 2.0;

    tx += cos(offset_angle) * (tw / 2.0 + th);
    ty += sin(offset_angle) * (th / 2.0 + th);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, tx, ty);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    pango_font_description_free(desc);
}

/* -------------------------------------------------------------------------- */
static void draw_arrow(
    cairo_t* cr, double x, double y, double a, double r, double g, double b)
{
    double ox1 = 16.0, oy1 = 0.0;
    double ox2 = -8.0, oy2 = 8.0;
    double ox3 = -4.0, oy3 = 0.0;
    double ox4 = -8.0, oy4 = -8.0;

    double m00 = cos(a), m01 = -sin(a);
    double m10 = sin(a), m11 = cos(a);

    double x1 = m00 * ox1 + m01 * oy1 + x;
    double y1 = m10 * ox1 + m11 * oy1 + y;
    double x2 = m00 * ox2 + m01 * oy2 + x;
    double y2 = m10 * ox2 + m11 * oy2 + y;
    double x3 = m00 * ox3 + m01 * oy3 + x;
    double y3 = m10 * ox3 + m11 * oy3 + y;
    double x4 = m00 * ox4 + m01 * oy4 + x;
    double y4 = m10 * ox4 + m11 * oy4 + y;

    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_line_to(cr, x3, y3);
    cairo_line_to(cr, x4, y4);
    cairo_close_path(cr);
    cairo_fill(cr);
}

/* -------------------------------------------------------------------------- */
static void draw_move_active_symbol(
    cairo_t* cr, double x, double y, double r, double g, double b)
{
    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, x + 12, y + 12);
    cairo_line_to(cr, x + 8, y + 8);
    cairo_move_to(cr, x - 12, y + 12);
    cairo_line_to(cr, x - 8, y + 8);
    cairo_move_to(cr, x + 12, y - 12);
    cairo_line_to(cr, x + 8, y - 8);
    cairo_move_to(cr, x - 12, y - 12);
    cairo_line_to(cr, x - 8, y - 8);
    cairo_stroke(cr);
}

/* -------------------------------------------------------------------------- */
static void draw_node(
    cairo_t* cr,
    double x,
    double y,
    double radius,
    const char* name,
    double r,
    double g,
    double b,
    int is_selected,
    int is_in_node,
    int is_out_node)
{
    draw_text(cr, name, x, y, M_PI / 2);

    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, x + radius, y);
    cairo_arc(cr, x, y, radius, 0, 2 * M_PI);

    if (is_in_node || is_out_node)
        cairo_stroke(cr);
    else
        cairo_fill(cr);

    if (is_in_node)
    {
        const double radius = DEFAULT_NODE_RADIUS * 0.707;
        cairo_move_to(cr, x + radius, y + radius);
        cairo_line_to(cr, x - radius, y - radius);
        cairo_move_to(cr, x + radius, y - radius);
        cairo_line_to(cr, x - radius, y + radius);
        cairo_stroke(cr);
    }

    if (is_selected)
    {
        cairo_set_source_rgb(cr, r, g, b);
        cairo_arc(cr, x, y, radius * 1.2, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
}

/* -------------------------------------------------------------------------- */
static void draw_edge(
    cairo_t* cr,
    double src_x,
    double src_y,
    double control_x,
    double control_y,
    double dst_x,
    double dst_y,
    const char* expr_str,
    double r,
    double g,
    double b)
{
    int orientation;
    double cx, cy, radius, a1, a2;

    orientation = calc_circle(
        &cx, &cy, &radius, src_x, src_y, control_x, control_y, dst_x, dst_y);
    if (orientation == 0)
    {
        double arrow_angle = atan2(dst_y - src_y, dst_x - src_x);
        double text_angle  = arrow_angle + M_PI / 2;
        if (text_angle < M_PI)
            text_angle += M_PI;
        cairo_set_source_rgb(cr, r, g, b);
        cairo_move_to(cr, src_x, src_y);
        cairo_line_to(cr, dst_x, dst_y);
        cairo_stroke(cr);
        draw_arrow(cr, control_x, control_y, arrow_angle, r, g, b);
        draw_text(cr, expr_str, control_x, control_y, text_angle);
        return;
    }

    a1 = atan2(src_y - cy, src_x - cx);
    a2 = atan2(dst_y - cy, dst_x - cx);
    cairo_new_path(cr);
    cairo_set_source_rgb(cr, r, g, b);
    if (orientation > 0)
        cairo_arc(cr, cx, cy, radius, a1, a2);
    else
        cairo_arc(cr, cx, cy, radius, a2, a1);
    cairo_stroke(cr);

    double text_angle = atan2(control_y - cy, control_x - cx);
    double arrow_angle =
        orientation > 0 ? text_angle + M_PI / 2 : text_angle - M_PI / 2;
    draw_arrow(cr, control_x, control_y, arrow_angle, r, g, b);
    draw_text(cr, expr_str, control_x, control_y, text_angle);
}

/* -------------------------------------------------------------------------- */
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t* cr,
    int width,
    int height,
    gpointer user_data)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    GraphEditor* editor             = user_data;
    const struct graph_model* model = &editor->model;
    (void)area;
    cairo_translate(cr, editor->pan_x, editor->pan_y);
    cairo_scale(cr, editor->zoom, editor->zoom);

    cairo_set_line_width(cr, 0.5 / editor->zoom);
    draw_grid(cr, editor->pan_x, editor->pan_y, width, height, editor->zoom);

    cairo_set_line_width(cr, 2.0 / editor->zoom);
    csfg_graph_for_each_edge (model->graph, e)
    {
        double r = 0.2, g = 0.2, b = 0.2;
        const struct csfg_node *n_src, *n_dst;
        const struct edge_attr* ea;
        const struct node_attr *na_src, *na_dst;
        n_src  = csfg_graph_get_node(model->graph, e->n_idx_from);
        n_dst  = csfg_graph_get_node(model->graph, e->n_idx_to);
        ea     = edge_attr_hmap_find(model->edge_attrs, e->id);
        na_src = node_attr_hmap_find(model->node_attrs, n_src->id);
        na_dst = node_attr_hmap_find(model->node_attrs, n_dst->id);
        if (ea == NULL || na_src == NULL || na_dst == NULL)
            continue;
        if (e->id == model->active_edge_id)
            r = 0.8, g = 0.5;
        draw_edge(
            cr,
            n_src->x,
            n_src->y,
            e->x,
            e->y,
            n_dst->x,
            n_dst->y,
            str_cstr(ea->expr_str),
            r,
            g,
            b);
        if (model->mode == MODE_MOVE && e->id == model->active_edge_id)
            draw_move_active_symbol(cr, e->x, e->y, r, g, b);
    }

    csfg_graph_for_each_node (model->graph, n)
    {
        double r = 0.2, g = 0.2, b = 0.2;
        const struct node_attr* na =
            node_attr_hmap_find(model->node_attrs, n->id);
        if (na == NULL)
            continue;
        if (n->id == model->active_node_id)
            r = 0.8, g = 0.5;
        draw_node(
            cr,
            n->x,
            n->y,
            na->radius,
            str_cstr(n->name),
            r,
            g,
            b,
            n->id == model->marked_node_id,
            n->id == model->node_in_id,
            n->id == model->node_out_id);
        if (model->mode == MODE_MOVE && n->id == model->active_node_id)
            draw_move_active_symbol(cr, n->x, n->y, r, g, b);
    }
}

/* -------------------------------------------------------------------------- */
static void click_begin(
    GtkGestureClick* gesture,
    int n_press,
    double x,
    double y,
    gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)gesture;

    gtk_widget_grab_focus(GTK_WIDGET(editor));

    try_select_edge_or_node(
        model,
        x = (x - editor->pan_x) / editor->zoom,
        y = (y - editor->pan_y) / editor->zoom);

    if (n_press == 2 /* double-click */)
        start_editing_active_node_or_edge(editor);

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void click_end(
    GtkGestureClick* gesture,
    int n_press,
    double x,
    double y,
    gpointer user_data)
{
    (void)gesture, (void)n_press, (void)x, (void)y, (void)user_data;
}

/* -------------------------------------------------------------------------- */
static void
drag_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_data)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;
    (void)gesture;

    try_select_edge_or_node(model, x, y);
    n = find_node(model->graph, model->active_node_id);
    e = find_edge(model->graph, model->active_edge_id);

    if (n != NULL)
    {
        editor->drag_offset_x = round((x - n->x) / 20) * 20;
        editor->drag_offset_y = round((y - n->y) / 20) * 20;
    }
    else if (e != NULL)
    {
        editor->drag_offset_x = round((x - e->x) / 20) * 20;
        editor->drag_offset_y = round((y - e->y) / 20) * 20;
    }
}

/* -------------------------------------------------------------------------- */
static void drag_update(
    GtkGestureDrag* gesture,
    double offset_x,
    double offset_y,
    gpointer user_data)
{
    int snap_size;
    struct csfg_node* n;
    struct csfg_edge* e;
    double start_x, start_y, x, y;
    GdkEvent* event;
    GraphEditor* editor       = user_data;
    struct graph_model* model = &editor->model;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - editor->pan_x) / editor->zoom;
    y = (start_y + offset_y - editor->pan_y) / editor->zoom;

    n = find_node(model->graph, model->active_node_id);
    e = find_edge(model->graph, model->active_edge_id);

    snap_size = 20;
    event =
        gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(gesture));
    if (event)
    {
        GdkModifierType mods = gdk_event_get_modifier_state(event);
        if (mods & GDK_CONTROL_MASK)
            snap_size *= 6;
    }

    if (n != NULL)
    {
        int node_x = round((x - editor->drag_offset_x) / snap_size) * snap_size;
        int node_y = round((y - editor->drag_offset_y) / snap_size) * snap_size;

        drag_edge_with_node(
            model->graph, model->active_node_id, node_x, node_y);

        n->x = node_x;
        n->y = node_y;
    }
    else if (e != NULL)
    {
        e->x = round((x - editor->drag_offset_x) / snap_size) * snap_size;
        e->y = round((y - editor->drag_offset_y) / snap_size) * snap_size;
    }

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void
drag_end(GtkGestureDrag* gesture, double x, double y, gpointer user_data)
{
    (void)gesture, (void)x, (void)y, (void)user_data;
}

/* -------------------------------------------------------------------------- */
static void
pan_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)gesture, (void)x, (void)y;
    editor->pan_offset_x = editor->pan_x;
    editor->pan_offset_y = editor->pan_y;
}

/* -------------------------------------------------------------------------- */
static void pan_update(
    GtkGestureDrag* gesture,
    double offset_x,
    double offset_y,
    gpointer user_data)
{
    double start_x, start_y;
    GraphEditor* editor = user_data;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    editor->pan_x = editor->pan_offset_x + offset_x;
    editor->pan_y = editor->pan_offset_y + offset_y;
    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static gboolean scroll_cb(
    GtkEventControllerScroll* controller,
    double dx,
    double dy,
    gpointer user_data)
{
    double gx, gy, new_zoom;
    GraphEditor* editor = user_data;
    (void)controller, (void)dx;

    new_zoom = dy > 0 ? editor->zoom * 0.9 : editor->zoom * 1.1;
    new_zoom = new_zoom > 4.0 ? 4.0 : new_zoom;
    new_zoom = new_zoom < 1.0 / 4 ? 1.0 / 4 : new_zoom;

    /* Convert mouse coordinates to graph space */
    gx = (editor->mouse_x - editor->pan_x) / editor->zoom;
    gy = (editor->mouse_y - editor->pan_y) / editor->zoom;

    /* Adjust pan so we zoom around the mouse coordinates and not 0,0 */
    editor->pan_x = editor->mouse_x - gx * new_zoom;
    editor->pan_y = editor->mouse_y - gy * new_zoom;
    editor->zoom  = new_zoom;

    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean mouse_motion_cb(
    GtkEventControllerMotion* controller,
    double dx,
    double dy,
    gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)controller;
    editor->mouse_x = dx;
    editor->mouse_y = dy;
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static void graph_editor_init(GraphEditor* self)
{
    struct graph_model* model = &self->model;

    model->graph = NULL;
    node_attr_hmap_init(&model->node_attrs);
    edge_attr_hmap_init(&model->edge_attrs);
    model->node_in_id        = -1;
    model->node_out_id       = -1;
    model->active_node_id    = -1;
    model->active_edge_id    = -1;
    model->marked_node_id    = -1;
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;

    self->overlay      = NULL;
    self->drawing_area = NULL;
    self->entry        = NULL;

    self->zoom          = 1.0;
    self->pan_x         = 500.0;
    self->pan_y         = 500.0;
    self->drag_offset_x = 0.0;
    self->drag_offset_y = 0.0;

    self->mouse_x = 0.0;
    self->mouse_y = 0.0;

    self->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area, TRUE);
    gtk_widget_set_vexpand(self->drawing_area, TRUE);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area), draw_cb, self, NULL);

    GtkGesture* click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(click_begin), self);
    g_signal_connect(click, "released", G_CALLBACK(click_end), self);
    gtk_widget_add_controller(self->drawing_area, GTK_EVENT_CONTROLLER(click));

    GtkGesture* drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), GDK_BUTTON_PRIMARY);
    g_signal_connect(drag, "drag-begin", G_CALLBACK(drag_begin), self);
    g_signal_connect(drag, "drag-update", G_CALLBACK(drag_update), self);
    g_signal_connect(drag, "drag-end", G_CALLBACK(drag_end), self);
    gtk_widget_add_controller(self->drawing_area, GTK_EVENT_CONTROLLER(drag));

    GtkGesture* pan = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(pan), GDK_BUTTON_MIDDLE);
    g_signal_connect(pan, "drag-begin", G_CALLBACK(pan_begin), self);
    g_signal_connect(pan, "drag-update", G_CALLBACK(pan_update), self);
    gtk_widget_add_controller(self->drawing_area, GTK_EVENT_CONTROLLER(pan));

    GtkEventController* zoom =
        gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(zoom, "scroll", G_CALLBACK(scroll_cb), self);
    gtk_widget_add_controller(self->drawing_area, zoom);

    GtkEventController* mouse_motion = gtk_event_controller_motion_new();
    g_signal_connect(mouse_motion, "motion", G_CALLBACK(mouse_motion_cb), self);
    gtk_widget_add_controller(self->drawing_area, mouse_motion);

    self->overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(self->overlay), self->drawing_area);

    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    setup_global_shortcuts(self, GTK_BOX(toolbar));

    gtk_orientable_set_orientation(
        GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(self), toolbar);
    gtk_box_append(GTK_BOX(self), self->overlay);
    gtk_widget_set_focusable(GTK_WIDGET(self), TRUE);
}

/* -------------------------------------------------------------------------- */
static void graph_editor_finalize(GObject* obj)
{
    GraphEditor* self         = PLUGIN_GRAPH_EDITOR(obj);
    struct graph_model* model = &self->model;
    int idx, id;
    struct edge_attr* ea;

    hmap_for_each (model->edge_attrs, idx, id, ea)
        (void)id, edge_attr_deinit(ea);

    edge_attr_hmap_deinit(model->edge_attrs);
    node_attr_hmap_deinit(model->node_attrs);
}

/* -------------------------------------------------------------------------- */
static void graph_editor_class_init(GraphEditorClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize     = graph_editor_finalize;
}

/* -------------------------------------------------------------------------- */
static void graph_editor_class_finalize(GraphEditorClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
void graph_editor_register_type_internal(GTypeModule* type_module)
{
    graph_editor_register_type(type_module);

    /*
     * Have to re-initialize static variables explicitly, because GTK doesn't
     * support types registered by modules to be unloaded, but we do it anyway.
     */
    // gpointer class = g_type_class_peek(PLUGIN_TYPE_GRAPH_EDITOR);
    // if (class)
    //     graph_editor_parent_class = g_type_class_peek_parent(class);
}

/* -------------------------------------------------------------------------- */
GraphEditor* graph_editor_new(
    struct plugin_ctx* plugin_ctx,
    const struct plugin_notify_interface* icb,
    struct plugin_notify_context* cb)
{
    GraphEditor* editor       = g_object_new(PLUGIN_TYPE_GRAPH_EDITOR, NULL);
    struct graph_model* model = &editor->model;
    model->plugin_ctx         = plugin_ctx;
    model->icb                = icb;
    model->cb                 = cb;
    return editor;
}

/* -------------------------------------------------------------------------- */
void graph_editor_set_graph(
    GraphEditor* editor, struct csfg_graph* g, int node_in, int node_out)
{
    editor->model.graph = g;
    graph_editor_rebuild_graph(editor, node_in, node_out);
}

/* -------------------------------------------------------------------------- */
void graph_editor_clear_graph(GraphEditor* editor)
{
    editor->model.graph = NULL;
    graph_editor_rebuild_graph(editor, -1, -1);
}

/* -------------------------------------------------------------------------- */
void graph_editor_rebuild_graph(GraphEditor* editor, int node_in, int node_out)
{
    rebuild_graph(&editor->model, node_in, node_out);
}

/* -------------------------------------------------------------------------- */
void graph_editor_redraw_graph(GraphEditor* editor)
{
    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
struct edge_attr_hmap* graph_editor_take_edge_attributes(GraphEditor* editor)
{
    struct edge_attr_hmap* edge_attrs = editor->model.edge_attrs;
    editor->model.edge_attrs          = NULL;
    return edge_attrs;
}

/* -------------------------------------------------------------------------- */
void graph_editor_set_edge_attributes(
    GraphEditor* editor, struct edge_attr_hmap* edge_attrs)
{
    int i, id;
    struct edge_attr* ea;

    if (editor->model.edge_attrs != NULL)
        hmap_for_each (editor->model.edge_attrs, i, id, ea)
        {
            (void)id, (void)ea;
            edge_attr_deinit(
                edge_attr_hmap_erase_slot(editor->model.edge_attrs, i));
        }
    edge_attr_hmap_deinit(editor->model.edge_attrs);

    editor->model.edge_attrs = edge_attrs;
}

/* -------------------------------------------------------------------------- */
void graph_editor_clear_attrs(GraphEditor* editor)
{
    int i, id;
    struct edge_attr* ea;

    if (editor->model.edge_attrs != NULL)
        hmap_for_each (editor->model.edge_attrs, i, id, ea)
        {
            (void)id, (void)ea;
            edge_attr_deinit(
                edge_attr_hmap_erase_slot(editor->model.edge_attrs, i));
        }
    edge_attr_hmap_clear(editor->model.edge_attrs);
}
