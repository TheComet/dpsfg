#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/hmap.h"
#include "csfg/util/str.h"
#include "dpsfg/plugin.h"
#include "graph-editor/graph_editor.h"

static const int    GRID_WIDTH = 20;
static const int    ARROW_RADIUS = 8;
static const double DEFAULT_NODE_SPACING = GRID_WIDTH * 6;
static int          DEFAULT_NODE_RADIUS = 10;

enum mode
{
    MODE_NORMAL,
    MODE_MOVE
};

struct node_attr
{
    double radius;
};

struct edge_attr
{
    struct str* expr_str;
};

HMAP_DECLARE(static, node_attr_hmap, int, struct node_attr, 16)
HMAP_DEFINE(static, node_attr_hmap, int, struct node_attr, 16)

HMAP_DECLARE(static, edge_attr_hmap, int, struct edge_attr, 16)
HMAP_DEFINE(static, edge_attr_hmap, int, struct edge_attr, 16)

struct _GraphEditor
{
    GtkBox parent_instance;

    /* drawing area is a child of "overlay". We use overlay to create GtkEntry's
     * on top of the drawing area so the user can enter formulas and give names
     * to nodes */
    GtkWidget* overlay;
    GtkWidget* drawing_area;
    GtkWidget* entry; /* Text entry, when active */

    const struct plugin_notify_interface* icb;
    struct dpsfg_plugin_callbacks*        cb;
    struct plugin_ctx*                    plugin_ctx;

    struct csfg_graph*     graph;
    struct node_attr_hmap* node_attrs;
    struct edge_attr_hmap* edge_attrs;

    int active_node_id;
    int active_edge_id;
    int selected_node_id;
    int node_in_id;
    int node_out_id;

    // View transform
    double zoom;
    double pan_x, pan_y;
    double pan_offset_x, pan_offset_y;

    // Drag state
    double drag_offset_x, drag_offset_y;

    /* It's difficult to get the mouse coordinates in some callbacks, so we use
     * GtkEventControllerMotion to store them here */
    double mouse_x, mouse_y;

    enum mode mode;
};

G_DEFINE_DYNAMIC_TYPE(GraphEditor, graph_editor, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static int
find_farthest_node_in_direction(const GraphEditor* editor, double dx, double dy)
{
    int                     idx, new_active_node;
    const struct csfg_node* n;
    double                  x, y;

    new_active_node = -1;
    csfg_graph_enumerate_nodes (editor->graph, idx, n)
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
    const GraphEditor* editor,
    double             dirx,
    double             diry,
    double             current_x,
    double             current_y)
{
    int                     idx, new_active_node;
    const struct csfg_node* n;
    double                  dx, dy;
    double                  new_dist, dist = 100000. * 100000.;

    new_active_node = editor->active_node_id;
    csfg_graph_enumerate_nodes (editor->graph, idx, n)
    {
        if (n->id == editor->active_node_id)
            continue;

        dx = current_x - n->x;
        dy = current_y - n->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
        {
            if (dirx > 0 && n->x > current_x)
                dist = new_dist, new_active_node = n->id;
            if (dirx < 0 && n->x < current_x)
                dist = new_dist, new_active_node = n->id;
            if (diry > 0 && n->y > current_y)
                dist = new_dist, new_active_node = n->id;
            if (diry < 0 && n->y < current_y)
                dist = new_dist, new_active_node = n->id;
        }
    }

    return new_active_node;
}

/* -------------------------------------------------------------------------- */
static int
find_farthest_edge_in_direction(const GraphEditor* editor, double dx, double dy)
{
    int                     idx, new_active_edge;
    const struct csfg_edge* e;
    double                  x, y;

    new_active_edge = -1;
    csfg_graph_enumerate_edges (editor->graph, idx, e)
    {
        if (new_active_edge == -1)
        {
            x = e->x, y = e->y;
            new_active_edge = e->id;
            continue;
        }

        if (dx > 0.0 && e->x > x)
            x = e->x, new_active_edge = e->id;
        if (dx < 0.0 && e->x < x)
            x = e->x, new_active_edge = e->id;
        if (dy > 0.0 && e->y > y)
            y = e->y, new_active_edge = e->id;
        if (dy < 0.0 && e->y < y)
            y = e->y, new_active_edge = e->id;
    }

    return new_active_edge;
}

/* -------------------------------------------------------------------------- */
static int find_closest_edge_in_direction(
    const GraphEditor* editor,
    double             dirx,
    double             diry,
    double             current_x,
    double             current_y)
{
    int                     idx, new_active_edge;
    const struct csfg_edge* e;
    double                  dx, dy;
    double                  new_dist, dist = 100000. * 100000.;

    new_active_edge = editor->active_edge_id;
    csfg_graph_enumerate_edges (editor->graph, idx, e)
    {
        if (e->id == editor->active_edge_id)
            continue;

        dx = current_x - e->x;
        dy = current_y - e->y;
        new_dist = dx * dx + dy * dy;
        if (new_dist < dist)
        {
            if (dirx > 0 && e->x > current_x)
                dist = new_dist, new_active_edge = e->id;
            if (dirx < 0 && e->x < current_x)
                dist = new_dist, new_active_edge = e->id;
            if (diry > 0 && e->y > current_y)
                dist = new_dist, new_active_edge = e->id;
            if (diry < 0 && e->y < current_y)
                dist = new_dist, new_active_edge = e->id;
        }
    }

    return new_active_edge;
}

/* -------------------------------------------------------------------------- */
static int calc_circle(
    double* cx,
    double* cy,
    double* radius,
    double  x1,
    double  y1,
    double  x2,
    double  y2,
    double  x3,
    double  y3)
{
    double dx = x1 - x3;
    double dy = y1 - y3;
    double dist = sqrt(dx * dx + dy * dy);

    /* Circumcenter */
    double A = x1 - x2;
    double B = y1 - y2;
    double C = x1 - x3;
    double D = y1 - y3;
    double E = ((x1 * x1 - x2 * x2) + (y1 * y1 - y2 * y2)) / 2.0;
    double F = ((x1 * x1 - x3 * x3) + (y1 * y1 - y3 * y3)) / 2.0;
    double den = A * D - B * C;
    if (fabs(den) < 0.0001)
    {
        *cx = (x1 + x3) / 2.0;
        *cy = (y1 + y3) / 2.0;
        return 0;
    }
    *cx = (D * E - B * F) / den;
    *cy = (-C * E + A * F) / den;
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
    struct csfg_edge*       e;
    double                  cx, cy, radius;
    int                     n_idx, orientation;

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
static struct csfg_node* find_node(struct csfg_graph* g, int node_id)
{
    struct csfg_node* n;
    csfg_graph_for_each_node (g, n)
        if (n->id == node_id)
            return n;
    return NULL;
}

/* -------------------------------------------------------------------------- */
static struct csfg_edge* find_edge(struct csfg_graph* g, int edge_id)
{
    struct csfg_edge* e;
    csfg_graph_for_each_edge (g, e)
        if (e->id == edge_id)
            return e;
    return NULL;
}

/* -------------------------------------------------------------------------- */
static void
next_active_node_in_direction(GraphEditor* editor, double dx, double dy)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    n = find_node(editor->graph, editor->active_node_id);
    e = find_edge(editor->graph, editor->active_edge_id);

    if (n != NULL)
        editor->active_node_id =
            find_closest_node_in_direction(editor, dx, dy, n->x, n->y);
    else if (e != NULL)
        editor->active_node_id =
            find_closest_node_in_direction(editor, dx, dy, e->x, e->y);
    else
        editor->active_node_id =
            find_farthest_node_in_direction(editor, dx, dy);

    if (editor->active_node_id > -1)
        editor->active_edge_id = -1;
}

/* -------------------------------------------------------------------------- */
static void
next_active_edge_in_direction(GraphEditor* editor, double dx, double dy)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    n = find_node(editor->graph, editor->active_node_id);
    e = find_edge(editor->graph, editor->active_edge_id);

    if (e != NULL)
        editor->active_edge_id =
            find_closest_edge_in_direction(editor, dx, dy, e->x, e->y);
    else if (n != NULL)
        editor->active_edge_id =
            find_closest_edge_in_direction(editor, dx, dy, n->x, n->y);
    else
        editor->active_edge_id =
            find_farthest_edge_in_direction(editor, dx, dy);

    if (editor->active_edge_id > -1)
        editor->active_node_id = -1;
}

/* -------------------------------------------------------------------------- */
static void
move_active_node_in_direction(GraphEditor* editor, double dx, double dy)
{
    struct csfg_node* n = find_node(editor->graph, editor->active_node_id);
    if (n == NULL)
        return;

    drag_edge_with_node(
        editor->graph,
        editor->active_node_id,
        n->x + dx * DEFAULT_NODE_SPACING,
        n->y + dy * DEFAULT_NODE_SPACING);
    n->x += dx * DEFAULT_NODE_SPACING;
    n->y += dy * DEFAULT_NODE_SPACING;

    editor->icb->graph_layout_changed(editor->cb, editor->plugin_ctx);

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void
move_active_edge_in_direction(GraphEditor* editor, double dx, double dy)
{
    struct csfg_edge* e = find_edge(editor->graph, editor->active_edge_id);
    if (e == NULL)
        return;

    e->x += dx * GRID_WIDTH;
    e->y += dy * GRID_WIDTH;

    editor->icb->graph_layout_changed(editor->cb, editor->plugin_ctx);

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_edge_left_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_edge_in_direction(editor, -1, 0); break;
        case MODE_MOVE:
            move_active_edge_in_direction(editor, -1, 0);
            move_active_node_in_direction(editor, -1, 0);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static gboolean
shortcut_edge_right_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_edge_in_direction(editor, 1, 0); break;
        case MODE_MOVE:
            move_active_edge_in_direction(editor, 1, 0);
            move_active_node_in_direction(editor, 1, 0);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static gboolean
shortcut_edge_up_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_edge_in_direction(editor, 0, -1); break;
        case MODE_MOVE:
            move_active_edge_in_direction(editor, 0, -1);
            move_active_node_in_direction(editor, 0, -1);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static gboolean
shortcut_edge_down_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_edge_in_direction(editor, 0, 1); break;
        case MODE_MOVE:
            move_active_edge_in_direction(editor, 0, 1);
            move_active_node_in_direction(editor, 0, 1);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_node_left_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_node_in_direction(editor, -1, 0); break;
        case MODE_MOVE:
            move_active_node_in_direction(editor, -1, 0);
            move_active_edge_in_direction(editor, -1, 0);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static gboolean
shortcut_node_right_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_node_in_direction(editor, 1, 0); break;
        case MODE_MOVE:
            move_active_node_in_direction(editor, 1, 0);
            move_active_edge_in_direction(editor, 1, 0);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static gboolean
shortcut_node_up_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_node_in_direction(editor, 0, -1); break;
        case MODE_MOVE:
            move_active_node_in_direction(editor, 0, -1);
            move_active_edge_in_direction(editor, 0, -1);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static gboolean
shortcut_node_down_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: next_active_node_in_direction(editor, 0, 1); break;
        case MODE_MOVE:
            move_active_node_in_direction(editor, 0, 1);
            move_active_edge_in_direction(editor, 0, 1);
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_switch_mode_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    switch (editor->mode)
    {
        case MODE_NORMAL: editor->mode = MODE_MOVE; break;
        case MODE_MOVE: editor->mode = MODE_NORMAL; break;
    }

    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static int is_near_any_other_node(
    double                   x,
    double                   y,
    const struct csfg_graph* g,
    const struct csfg_node*  exclude_node)
{
    const struct csfg_node* n;
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
    double                   x,
    double                   y,
    const struct csfg_graph* g,
    const struct csfg_edge*  exclude_edge)
{
    const struct csfg_edge* e;
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
    struct csfg_node*        n,
    double                   near_x,
    double                   near_y)
{
    n->x = near_x;
    n->y = near_y;
    while (is_near_any_other_node(n->x, n->y, g, n))
        n->x += DEFAULT_NODE_SPACING;
}

/* -------------------------------------------------------------------------- */
static void auto_position_edge(
    const struct csfg_graph* g,
    struct csfg_edge*        e,
    const struct csfg_node*  from,
    const struct csfg_node*  to)
{
    e->x = (from->x + to->x) / 2;
    e->y = (from->y + to->y) / 2;

    double spacing_increments = DEFAULT_NODE_SPACING / 2;
    double edge_add_spacing = spacing_increments;

    while (is_near_any_other_node(e->x, e->y, g, NULL) ||
           is_near_any_other_edge(e->x, e->y, g, e))
    {
        double tmp;
        double nx = from->x - to->x;
        double ny = from->y - to->y;
        double den = sqrt(nx * nx + ny * ny);
        tmp = nx;
        nx = -ny / den;
        ny = tmp / den;

        e->x -= nx * edge_add_spacing;
        e->y -= ny * edge_add_spacing;
        edge_add_spacing = edge_add_spacing > 0
                               ? -(edge_add_spacing + spacing_increments)
                               : -edge_add_spacing + spacing_increments;
    }
}

/* -------------------------------------------------------------------------- */
static int
create_new_edge(GraphEditor* editor, int n_id_selected, int n_id_active)
{
    struct csfg_expr_pool*  pool;
    struct edge_attr*       ea;
    struct csfg_edge*       e;
    const struct csfg_node* n_selected;
    const struct csfg_node* n_active;
    int                     n_idx_selected, n_idx_active, expr, e_idx;

    if (n_id_selected == -1 || n_id_active == -1)
        return -1;

    csfg_graph_enumerate_nodes (editor->graph, n_idx_selected, n_selected)
        if (n_selected->id == n_id_selected)
            break;
    if (n_idx_selected == csfg_graph_node_count(editor->graph))
        return -1;

    csfg_graph_enumerate_nodes (editor->graph, n_idx_active, n_active)
        if (n_active->id == n_id_active)
            break;
    if (n_idx_active == csfg_graph_node_count(editor->graph))
        return -1;

    csfg_expr_pool_init(&pool);
    expr = csfg_expr_parse(&pool, cstr_view("1"));
    e_idx = csfg_graph_add_edge(
        editor->graph, n_idx_selected, n_idx_active, pool, expr);
    e = csfg_graph_get_edge(editor->graph, e_idx);
    ea = edge_attr_hmap_emplace_new(&editor->edge_attrs, e->id);

    e->x = (n_active->x + n_selected->x) / 2;
    e->y = (n_active->y + n_selected->y) / 2;
    str_init(&ea->expr_str);
    csfg_expr_to_str(&ea->expr_str, pool, expr);

    auto_position_edge(editor->graph, e, n_selected, n_active);

    return e_idx;
}

/* -------------------------------------------------------------------------- */
static int create_new_node(GraphEditor* editor, int n_id_prev)
{
    struct csfg_node*       n;
    struct node_attr*       na;
    const struct csfg_node* n_prev;
    int                     n_idx;

    n_idx = csfg_graph_add_node(editor->graph, "V1");
    n = csfg_graph_get_node(editor->graph, n_idx);
    na = node_attr_hmap_emplace_new(&editor->node_attrs, n->id);
    na->radius = DEFAULT_NODE_RADIUS;

    if (n_id_prev > -1)
    {
        n_prev = find_node(editor->graph, n_id_prev);
        auto_position_node_near(editor->graph, n, n_prev->x, n_prev->y);
    }
    else
        auto_position_node_near(editor->graph, n, 0.0, 0.0);

    return n->id;
}

/* -------------------------------------------------------------------------- */
static void notify_graph_changed(GraphEditor* editor)
{
    int                     node_in_idx, node_out_idx;
    const struct csfg_node* n;

    csfg_graph_enumerate_nodes (editor->graph, node_in_idx, n)
        if (n->id == editor->node_in_id)
            break;
    csfg_graph_enumerate_nodes (editor->graph, node_out_idx, n)
        if (n->id == editor->node_out_id)
            break;

    if (node_in_idx == csfg_graph_node_count(editor->graph))
        node_in_idx = -1;
    if (node_out_idx == csfg_graph_node_count(editor->graph))
        node_out_idx = -1;

    editor->icb->graph_structure_changed(
        editor->cb, editor->plugin_ctx, node_in_idx, node_out_idx);
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_new_node_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    editor->active_node_id = create_new_node(editor, editor->active_node_id);
    editor->active_edge_id = -1;
    editor->mode = MODE_NORMAL;
    notify_graph_changed(editor);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean shortcut_new_node_and_edge_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    int          prev_active_node_id = editor->active_node_id;
    (void)widget, (void)unused;
    editor->active_node_id = create_new_node(editor, editor->active_node_id);
    editor->mode = MODE_NORMAL;
    create_new_edge(editor, prev_active_node_id, editor->active_node_id);
    notify_graph_changed(editor);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static int delete_edge(GraphEditor* editor, int edge_id)
{
    int               e_idx;
    struct csfg_edge* e;
    struct edge_attr* ea;

    csfg_graph_enumerate_edges (editor->graph, e_idx, e)
        if (e->id == edge_id)
            break;
    if (e_idx == csfg_graph_edge_count(editor->graph))
        return -1;

    ea = edge_attr_hmap_erase(editor->edge_attrs, edge_id);
    str_deinit(ea->expr_str);
    csfg_graph_mark_edge_deleted(editor->graph, e_idx);
    csfg_graph_gc(editor->graph);

    return -1;
}

/* -------------------------------------------------------------------------- */
static int delete_node(GraphEditor* editor, int node_id)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    int               e_idx, n_idx;
    int               next_active_node_id = -1;

    /* Prefer following edges pointing to us */
    csfg_graph_enumerate_edges (editor->graph, e_idx, e)
        if (csfg_graph_get_node(editor->graph, e->n_idx_to)->id == node_id)
        {
            next_active_node_id =
                csfg_graph_get_node(editor->graph, e->n_idx_from)->id;
            break;
        }
    if (next_active_node_id == -1)
        csfg_graph_enumerate_edges (editor->graph, e_idx, e)
            if (csfg_graph_get_node(editor->graph, e->n_idx_from)->id ==
                node_id)
            {
                next_active_node_id =
                    csfg_graph_get_node(editor->graph, e->n_idx_to)->id;
                break;
            }

    csfg_graph_enumerate_edges (editor->graph, e_idx, e)
    {
        if (csfg_graph_get_node(editor->graph, e->n_idx_from)->id == node_id ||
            csfg_graph_get_node(editor->graph, e->n_idx_to)->id == node_id)
        {
            struct edge_attr* ea;
            ea = edge_attr_hmap_erase(editor->edge_attrs, e->id);
            str_deinit(ea->expr_str);
            csfg_graph_mark_edge_deleted(editor->graph, e_idx);
        }
    }

    csfg_graph_enumerate_nodes (editor->graph, n_idx, n)
        if (n->id == node_id)
            break;
    if (n_idx != csfg_graph_node_count(editor->graph))
    {
        node_attr_hmap_erase(editor->node_attrs, node_id);
        csfg_graph_mark_node_deleted(editor->graph, n_idx);
        csfg_graph_gc(editor->graph);
    }

    /* Fall back to searching for the right-most node in the graph */
    if (next_active_node_id == -1)
        next_active_node_id = find_farthest_node_in_direction(editor, 1, 0);

    return next_active_node_id;
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_delete_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;

    editor->active_node_id = delete_node(editor, editor->active_node_id);
    editor->active_edge_id = delete_edge(editor, editor->active_edge_id);

    notify_graph_changed(editor);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_select_node_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    if (editor->selected_node_id == editor->active_node_id)
        editor->selected_node_id = -1;
    else
        editor->selected_node_id = editor->active_node_id;
    editor->mode = MODE_NORMAL;
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_new_edge_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    create_new_edge(editor, editor->selected_node_id, editor->active_node_id);
    notify_graph_changed(editor);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean
shortcut_set_in_node_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;

    editor->node_in_id = editor->active_node_id;

    notify_graph_changed(editor);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean shortcut_set_out_node_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;

    editor->node_out_id = editor->active_node_id;

    notify_graph_changed(editor);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static void finish_editing(GtkEntry* e, gpointer user_pointer);
static void
on_entry_focus_changed(GObject* obj, GParamSpec* pspec, gpointer user_data)
{
    GtkWidget* entry = GTK_WIDGET(obj);
    (void)pspec;
    if (!gtk_widget_has_focus(entry))
        finish_editing(GTK_ENTRY(entry), user_data);
}

/* -------------------------------------------------------------------------- */
static void start_editing(
    GraphEditor* editor,
    double       x,
    double       y,
    double       radius,
    const char*  current_name)
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

/* -------------------------------------------------------------------------- */
static void start_editing_active_node_or_edge(GraphEditor* editor)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    const struct node_attr* na;
    const struct edge_attr* ea;

    n = find_node(editor->graph, editor->active_node_id);
    e = find_edge(editor->graph, editor->active_edge_id);

    if ((n = find_node(editor->graph, editor->active_node_id)) != NULL)
    {
        na = node_attr_hmap_find(editor->node_attrs, n->id);
        if (na != NULL)
            start_editing(editor, n->x, n->y, na->radius, str_cstr(n->name));
    }
    else if ((e = find_edge(editor->graph, editor->active_edge_id)) != NULL)
    {
        ea = edge_attr_hmap_find(editor->edge_attrs, editor->active_edge_id);
        if (ea != NULL)
            start_editing(editor, e->x, e->y, 10, str_cstr(ea->expr_str));
    }

    editor->mode = MODE_NORMAL;
}

/* -------------------------------------------------------------------------- */
static gboolean shortcut_edit_node_or_edge_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    (void)widget, (void)unused;
    start_editing_active_node_or_edge(user_data);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static void setup_global_shortcuts(GraphEditor* editor)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction*  action;
    GtkShortcut*        shortcut;
    int                 i;

    struct
    {
        guint           keyval;
        GdkModifierType modifiers;
        gboolean (*callback)(GtkWidget*, GVariant*, gpointer);
    } table[] = {
        /* clang-format off */
        {GDK_KEY_h, GDK_NO_MODIFIER_MASK, shortcut_node_left_cb},
        {GDK_KEY_l, GDK_NO_MODIFIER_MASK, shortcut_node_right_cb},
        {GDK_KEY_j, GDK_NO_MODIFIER_MASK, shortcut_node_down_cb},
        {GDK_KEY_k, GDK_NO_MODIFIER_MASK, shortcut_node_up_cb},
        {GDK_KEY_h, GDK_SHIFT_MASK,       shortcut_edge_left_cb},
        {GDK_KEY_l, GDK_SHIFT_MASK,       shortcut_edge_right_cb},
        {GDK_KEY_j, GDK_SHIFT_MASK,       shortcut_edge_down_cb},
        {GDK_KEY_k, GDK_SHIFT_MASK,       shortcut_edge_up_cb},
        {GDK_KEY_m, GDK_NO_MODIFIER_MASK, shortcut_switch_mode_cb},
        {GDK_KEY_m, GDK_SHIFT_MASK,       shortcut_switch_mode_cb},
        {GDK_KEY_n, GDK_NO_MODIFIER_MASK, shortcut_new_node_cb},
        {GDK_KEY_n, GDK_SHIFT_MASK,       shortcut_new_node_and_edge_cb},
        {GDK_KEY_x, GDK_NO_MODIFIER_MASK, shortcut_delete_cb},
        {GDK_KEY_s, GDK_NO_MODIFIER_MASK, shortcut_select_node_cb},
        {GDK_KEY_e, GDK_NO_MODIFIER_MASK, shortcut_new_edge_cb},
        {GDK_KEY_i, GDK_NO_MODIFIER_MASK, shortcut_set_in_node_cb},
        {GDK_KEY_o, GDK_NO_MODIFIER_MASK, shortcut_set_out_node_cb},
        {GDK_KEY_i, GDK_SHIFT_MASK,       shortcut_edit_node_or_edge_cb},
        /* clang-format on */
    };

    controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(
        GTK_SHORTCUT_CONTROLLER(controller), GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(editor->drawing_area, controller);

    for (i = 0; i != G_N_ELEMENTS(table); ++i)
    {
        trigger = gtk_keyval_trigger_new(table[i].keyval, table[i].modifiers);
        action = gtk_callback_action_new(table[i].callback, editor, NULL);
        shortcut = gtk_shortcut_new(trigger, action);
        gtk_shortcut_controller_add_shortcut(
            GTK_SHORTCUT_CONTROLLER(controller), shortcut);
    }
}

/* -------------------------------------------------------------------------- */
static void draw_grid(
    cairo_t* cr, double pan_x, double pan_y, int width, int height, double zoom)
{
    pan_x /= zoom;
    pan_y /= zoom;
    width /= zoom;
    height /= zoom;

    int from_x = -(int)pan_x / GRID_WIDTH * GRID_WIDTH - GRID_WIDTH;
    int from_y = -(int)pan_y / GRID_WIDTH * GRID_WIDTH - GRID_WIDTH;
    int to_x = from_x + width + GRID_WIDTH;
    int to_y = from_y + height + GRID_WIDTH;

    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    for (int x = from_x; x < to_x; x += 20)
    {
        cairo_move_to(cr, x, from_y);
        cairo_line_to(cr, x, to_y);
    }
    for (int y = from_y; y < to_y; y += 20)
    {
        cairo_move_to(cr, from_x, y);
        cairo_line_to(cr, to_x, y);
    }
    cairo_stroke(cr);
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
    cairo_t*    cr,
    double      x,
    double      y,
    double      radius,
    const char* name,
    double      r,
    double      g,
    double      b,
    int         is_selected,
    int         is_in_node,
    int         is_out_node)
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
    cairo_t*    cr,
    double      src_x,
    double      src_y,
    double      control_x,
    double      control_y,
    double      dst_x,
    double      dst_y,
    const char* expr_str,
    double      r,
    double      g,
    double      b)
{
    int    orientation;
    double cx, cy, radius, a1, a2;

    orientation = calc_circle(
        &cx, &cy, &radius, src_x, src_y, control_x, control_y, dst_x, dst_y);
    if (orientation == 0)
    {
        double arrow_angle = atan2(dst_y - src_y, dst_x - src_x);
        double text_angle = arrow_angle + M_PI / 2;
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
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    GraphEditor*            editor = user_pointer;
    (void)area;
    cairo_translate(cr, editor->pan_x, editor->pan_y);
    cairo_scale(cr, editor->zoom, editor->zoom);

    cairo_set_line_width(cr, 0.5 / editor->zoom);
    draw_grid(cr, editor->pan_x, editor->pan_y, width, height, editor->zoom);

    cairo_set_line_width(cr, 2.0 / editor->zoom);
    csfg_graph_for_each_edge (editor->graph, e)
    {
        double                  r = 0.2, g = 0.2, b = 0.2;
        const struct csfg_node *n_src, *n_dst;
        const struct edge_attr* ea;
        const struct node_attr *na_src, *na_dst;
        n_src = csfg_graph_get_node(editor->graph, e->n_idx_from);
        n_dst = csfg_graph_get_node(editor->graph, e->n_idx_to);
        ea = edge_attr_hmap_find(editor->edge_attrs, e->id);
        na_src = node_attr_hmap_find(editor->node_attrs, n_src->id);
        na_dst = node_attr_hmap_find(editor->node_attrs, n_dst->id);
        if (ea == NULL || na_src == NULL || na_dst == NULL)
            continue;
        if (e->id == editor->active_edge_id)
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
        if (editor->mode == MODE_MOVE && e->id == editor->active_edge_id)
            draw_move_active_symbol(cr, e->x, e->y, r, g, b);
    }

    csfg_graph_for_each_node (editor->graph, n)
    {
        double                  r = 0.2, g = 0.2, b = 0.2;
        const struct node_attr* na =
            node_attr_hmap_find(editor->node_attrs, n->id);
        if (na == NULL)
            continue;
        if (n->id == editor->active_node_id)
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
            n->id == editor->selected_node_id,
            n->id == editor->node_in_id,
            n->id == editor->node_out_id);
        if (editor->mode == MODE_MOVE && n->id == editor->active_node_id)
            draw_move_active_symbol(cr, n->x, n->y, r, g, b);
    }
}

/* -------------------------------------------------------------------------- */
static int try_select_node(
    double                       mouse_x,
    double                       mouse_y,
    const struct csfg_graph*     graph,
    const struct node_attr_hmap* attrs)
{
    double                  dx, dy;
    int                     idx;
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
    double                  dx, dy;
    int                     idx;
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
static void finish_editing(GtkEntry* entry, gpointer user_pointer)
{
    int               n_idx;
    struct csfg_node* n;
    struct csfg_edge* e;
    struct edge_attr* ea;
    GraphEditor*      editor = user_pointer;

    const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));

    if (editor->active_node_id > -1)
    {
        csfg_graph_enumerate_nodes (editor->graph, n_idx, n)
            if (n->id == editor->active_node_id)
            {
                str_set_cstr(&n->name, text);
                break;
            }
    }
    else if (editor->active_edge_id > -1)
    {
        ea = edge_attr_hmap_find(editor->edge_attrs, editor->active_edge_id);
        str_set_cstr(&ea->expr_str, text);

        csfg_graph_for_each_edge (editor->graph, e)
            if (e->id == editor->active_edge_id)
            {
                csfg_expr_pool_clear(e->pool);
                e->expr = csfg_expr_parse(&e->pool, cstr_view(text));
            }
    }

    gtk_widget_unparent(editor->entry);
    editor->entry = NULL;

    notify_graph_changed(editor);
    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void try_select_edge_or_node(GraphEditor* editor, double x, double y)
{
    editor->active_node_id =
        try_select_node(x, y, editor->graph, editor->node_attrs);
    editor->active_edge_id = try_select_edge(x, y, editor->graph);

    /* Edge has priority when clicking with the mouse -- feels better */
    if (editor->active_edge_id > -1)
        editor->active_node_id = -1;
}

/* -------------------------------------------------------------------------- */
static void click_begin(
    GtkGestureClick* gesture,
    int              n_press,
    double           x,
    double           y,
    gpointer         user_pointer)
{
    GraphEditor* editor = user_pointer;
    (void)gesture;

    gtk_widget_grab_focus(GTK_WIDGET(editor));

    try_select_edge_or_node(
        editor,
        x = (x - editor->pan_x) / editor->zoom,
        y = (y - editor->pan_y) / editor->zoom);

    if (n_press == 2 /* double-click */)
        start_editing_active_node_or_edge(editor);

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void click_end(
    GtkGestureClick* gesture,
    int              n_press,
    double           x,
    double           y,
    gpointer         user_pointer)
{
}

/* -------------------------------------------------------------------------- */
static void
drag_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    GraphEditor* editor = user_pointer;
    (void)gesture;

    try_select_edge_or_node(editor, x, y);
    n = find_node(editor->graph, editor->active_node_id);
    e = find_edge(editor->graph, editor->active_edge_id);

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
    double          offset_x,
    double          offset_y,
    gpointer        user_pointer)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    double            start_x, start_y, x, y;
    GraphEditor*      editor = user_pointer;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - editor->pan_x) / editor->zoom;
    y = (start_y + offset_y - editor->pan_y) / editor->zoom;

    n = find_node(editor->graph, editor->active_node_id);
    e = find_edge(editor->graph, editor->active_edge_id);

    if (n != NULL)
    {
        int node_x = round((x - editor->drag_offset_x) / 20) * 20;
        int node_y = round((y - editor->drag_offset_y) / 20) * 20;

        drag_edge_with_node(
            editor->graph, editor->active_node_id, node_x, node_y);

        n->x = node_x;
        n->y = node_y;
    }
    else if (e != NULL)
    {
        e->x = round((x - editor->drag_offset_x) / 20) * 20;
        e->y = round((y - editor->drag_offset_y) / 20) * 20;
    }

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void
drag_end(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
}

/* -------------------------------------------------------------------------- */
static void
pan_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;
    (void)gesture, (void)x, (void)y;
    editor->pan_offset_x = editor->pan_x;
    editor->pan_offset_y = editor->pan_y;
}

/* -------------------------------------------------------------------------- */
static void pan_update(
    GtkGestureDrag* gesture,
    double          offset_x,
    double          offset_y,
    gpointer        user_pointer)
{
    double       start_x, start_y;
    GraphEditor* editor = user_pointer;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    editor->pan_x = editor->pan_offset_x + offset_x;
    editor->pan_y = editor->pan_offset_y + offset_y;
    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static gboolean scroll_cb(
    GtkEventControllerScroll* controller,
    double                    dx,
    double                    dy,
    gpointer                  user_pointer)
{
    double       gx, gy, new_zoom;
    GraphEditor* editor = user_pointer;
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
    editor->zoom = new_zoom;

    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static gboolean mouse_motion_cb(
    GtkEventControllerMotion* controller,
    double                    dx,
    double                    dy,
    gpointer                  user_pointer)
{
    GraphEditor* editor = user_pointer;
    (void)controller;
    editor->mouse_x = dx;
    editor->mouse_y = dy;
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static void graph_editor_init(GraphEditor* self)
{
    self->overlay = NULL;
    self->drawing_area = NULL;
    self->entry = NULL;

    self->graph = NULL;
    node_attr_hmap_init(&self->node_attrs);
    edge_attr_hmap_init(&self->edge_attrs);
    self->node_in_id = -1;
    self->node_out_id = -1;

    self->zoom = 1.0;
    self->pan_x = 500.0;
    self->pan_y = 500.0;
    self->active_node_id = -1;
    self->active_edge_id = -1;
    self->selected_node_id = -1;
    self->drag_offset_x = 0.0;
    self->drag_offset_y = 0.0;

    self->mouse_x = 0.0;
    self->mouse_y = 0.0;

    self->mode = MODE_NORMAL;

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

    gtk_box_append(GTK_BOX(self), self->overlay);

    setup_global_shortcuts(self);
    gtk_widget_set_focusable(GTK_WIDGET(self), TRUE);
}

/* -------------------------------------------------------------------------- */
static void graph_editor_finalize(GObject* obj)
{
    GraphEditor*      self = PLUGIN_GRAPH_EDITOR(obj);
    int               idx, id;
    struct edge_attr* ea;

    hmap_for_each (self->edge_attrs, idx, id, ea)
        (void)idx, (void)id, str_deinit(ea->expr_str);
    edge_attr_hmap_deinit(self->edge_attrs);

    node_attr_hmap_deinit(self->node_attrs);
}

/* -------------------------------------------------------------------------- */
static void graph_editor_class_init(GraphEditorClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize = graph_editor_finalize;
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
    struct plugin_ctx*                    plugin_ctx,
    const struct plugin_notify_interface* icb,
    struct dpsfg_plugin_callbacks*        cb)
{
    GraphEditor* editor = g_object_new(PLUGIN_TYPE_GRAPH_EDITOR, NULL);
    editor->plugin_ctx = plugin_ctx;
    editor->icb = icb;
    editor->cb = cb;
    return editor;
}

/* -------------------------------------------------------------------------- */
void graph_editor_set_graph(
    GraphEditor* editor, struct csfg_graph* g, int node_in, int node_out)
{
    editor->graph = g;
    graph_editor_rebuild_graph(editor, node_in, node_out);
}

/* -------------------------------------------------------------------------- */
void graph_editor_clear_graph(GraphEditor* editor)
{
    editor->graph = NULL;
    graph_editor_rebuild_graph(editor, -1, -1);
}

/* -------------------------------------------------------------------------- */
void graph_editor_rebuild_graph(GraphEditor* editor, int node_in, int node_out)
{
    int               i, id;
    struct csfg_node* n;
    struct csfg_edge* e;
    struct node_attr* na;
    struct edge_attr* ea;

    if (editor->graph == NULL)
        return;

    editor->node_in_id = -1;
    editor->node_out_id = -1;
    csfg_graph_enumerate_nodes (editor->graph, i, n)
    {
        if (i == node_in)
            editor->node_in_id = n->id;
        if (i == node_out)
            editor->node_out_id = n->id;
    }

    csfg_graph_enumerate_nodes (editor->graph, i, n)
        switch (node_attr_hmap_emplace_or_get(&editor->node_attrs, n->id, &na))
        {
            case HMAP_OOM: return;
            case HMAP_NEW: na->radius = DEFAULT_NODE_RADIUS;
            case HMAP_EXISTS: break;
        }

    csfg_graph_for_each_edge (editor->graph, e)
        switch (edge_attr_hmap_emplace_or_get(&editor->edge_attrs, e->id, &ea))
        {
            case HMAP_OOM: return;
            case HMAP_NEW: {
                str_init(&ea->expr_str);
                csfg_expr_to_str(&ea->expr_str, e->pool, e->expr);
            }
            case HMAP_EXISTS: break;
        }

    hmap_for_each (editor->node_attrs, i, id, na)
        if (find_node(editor->graph, id) == NULL)
            node_attr_hmap_erase_slot(editor->node_attrs, i);

    hmap_for_each (editor->edge_attrs, i, id, ea)
        if (find_edge(editor->graph, id) == NULL)
            edge_attr_hmap_erase_slot(editor->edge_attrs, i);
}

/* -------------------------------------------------------------------------- */
void graph_editor_redraw_graph(GraphEditor* editor)
{
    gtk_widget_queue_draw(editor->drawing_area);
}
