#include "csfg/graph/graph.h"
#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"
#include "csfg/util/vec.h"
#include "dpsfg-plugin.h"
#include "graph-editor/color_picker.h"
#include "graph-editor/graph_editor.h"
#include <librsvg/rsvg.h>
#include <math.h>

static const int GRID                    = 20;
static const int ARROW_RADIUS            = 8;
static const double DEFAULT_NODE_SPACING = GRID * 6;
static const int DEFAULT_NODE_RADIUS     = 10;

#define COLOR_BUTTONS_LIST                                                     \
    X(1, "1", 0x333333)                                                        \
    X(2, "2", 0x0b723b)                                                        \
    X(3, "3", 0x870a00)                                                        \
    X(4, "4", 0x2964ce)                                                        \
    X(5, "5", 0xc740f8)                                                        \
    X(6, "6", 0xff3022)

enum color_button
{
#define X(idx, shortcut, default_color) COLOR_BUTTON##idx,
    COLOR_BUTTONS_LIST
#undef X
        COLOR_BUTTONS_COUNT
};

VEC_DECLARE(undo_stack_vec, struct serializer*, 32)
VEC_DEFINE(undo_stack_vec, struct serializer*, 32)

HMAP_DEFINE(extern, node_attr_hmap, int, struct node_attr, 16)
HMAP_DEFINE(extern, edge_attr_hmap, int, struct edge_attr, 16)

struct _GraphEditor
{
    GtkBox parent_instance;

    /* drawing area is a child of "overlay". We use overlay to create GtkEntry's
     * on top of the drawing area so the user can enter formulas and give names
     * to nodes */
    GtkWidget* overlay;
    GtkWidget* drawing_area;
    GtkWidget* entry; /* Text entry, when active */
    RsvgHandle* seven_steps;

    GtkToggleButton* color_buttons[COLOR_BUTTONS_COUNT];

    /* It's difficult to get the mouse coordinates in some callbacks, so we use
     * GtkEventControllerMotion to store them here */
    double mouse_x, mouse_y;

    /* View transform */
    double zoom;
    double pan_x, pan_y;
    double pan_offset_x, pan_offset_y;

    /* Drag state */
    double drag_offset_x, drag_offset_y;

    struct graph_model* model;

    unsigned show_shortcuts   : 1;
    unsigned show_seven_steps : 1;
};

G_DEFINE_DYNAMIC_TYPE(GraphEditor, graph_editor, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static struct color rgb(uint8_t r, uint8_t g, uint8_t b)
{
    struct color color;
    color.r = r;
    color.g = g;
    color.b = b;
    return color;
}
static GdkRGBA hex_gdk(uint32_t rgb)
{
    GdkRGBA color;
    color.red   = ((rgb >> 16) & 0xFF) / 255.0;
    color.green = ((rgb >> 8) & 0xFF) / 255.0;
    color.blue  = ((rgb >> 0) & 0xFF) / 255.0;
    color.alpha = 1.0;
    return color;
}
static struct color gdk_to_color(GdkRGBA gdk)
{
    struct color color;
    color.r = gdk.red * 255;
    color.g = gdk.green * 255;
    color.b = gdk.blue * 255;
    return color;
}

/* -------------------------------------------------------------------------- */
static struct color highlight_color(struct color color)
{
    (void)color;
    return rgb(204, 128, 0);
}

/* -------------------------------------------------------------------------- */
static void edge_attr_init(struct edge_attr* ea, struct color color)
{
    ea->color = color;
    str_init(&ea->expr_str);
}
static void edge_attr_deinit(struct edge_attr* ea)
{
    str_deinit(ea->expr_str);
}

/* -------------------------------------------------------------------------- */
static void node_attr_init(struct node_attr* na, struct color color)
{
    na->radius = DEFAULT_NODE_RADIUS;
    na->color  = color;
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

    model->graph_color = rgb(50, 50, 50);
    model->draw_color  = rgb(80, 80, 80);

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

    hmap_for_each (model->edge_attrs, idx, id, ea)
        (void)id, edge_attr_deinit(ea);
    edge_attr_hmap_deinit(model->edge_attrs);
    node_attr_hmap_deinit(model->node_attrs);

    vec_for_each (model->undo_stack, ser)
        serializer_deinit(*ser);
    undo_stack_vec_deinit(model->undo_stack);
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
static int push_undo_state(struct graph_model* model)
{
    struct serializer** ser;

    /* destroy future */
    while (vec_count(model->undo_stack) - 1 > model->undo_stack_ptr)
        serializer_deinit(*undo_stack_vec_pop(model->undo_stack));

    ser = undo_stack_vec_emplace(&model->undo_stack);
    if (ser == NULL)
        return -1;
    serializer_init(ser);

    if (csfg_io_graph_save(
            ser,
            model->graph,
            find_node_idx(model->graph, model->node_in_id),
            find_node_idx(model->graph, model->node_out_id)) != 0 ||
        graph_model_save_attrs(model, ser) != 0)
    {
        serializer_deinit(*undo_stack_vec_pop(model->undo_stack));
        return -1;
    }

    model->undo_stack_ptr++;
    return 0;
}
static int undo(struct graph_model* model)
{
    int node_in, node_out;
    struct serializer* ser;
    struct deserializer des;

    if (model->undo_stack_ptr > 0)
    {
        ser = *vec_get(model->undo_stack, model->undo_stack_ptr - 1);
        des = deserializer(vec_data(ser), vec_count(ser));
        if (csfg_io_graph_load(&des, model->graph, &node_in, &node_out) != 0)
            return -1;
        graph_model_rebuild_graph(model, node_in, node_out);
        if (graph_model_load_attrs(model, &des) != 0)
            return -1;
        model->undo_stack_ptr--;
    }

    return 0;
}
static int redo(struct graph_model* model)
{
    int node_in, node_out;
    struct serializer* ser;
    struct deserializer des;

    if (model->undo_stack_ptr + 1 < vec_count(model->undo_stack))
    {
        ser = *vec_get(model->undo_stack, model->undo_stack_ptr + 1);
        des = deserializer(vec_data(ser), vec_count(ser));
        if (csfg_io_graph_load(&des, model->graph, &node_in, &node_out) != 0)
            return -1;
        graph_model_rebuild_graph(model, node_in, node_out);
        if (graph_model_load_attrs(model, &des) != 0)
            return -1;
        model->undo_stack_ptr++;
    }

    return 0;
}
void graph_model_reinit_undo_stack(struct graph_model* model)
{
    while (vec_count(model->undo_stack) - 1 > model->undo_stack_ptr)
        serializer_deinit(*undo_stack_vec_pop(model->undo_stack));
    push_undo_state(model);
}

/* -------------------------------------------------------------------------- */
int graph_model_save_attrs(struct graph_model* model, struct serializer** ser)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    const struct node_attr* na;
    const struct edge_attr* ea;
    int err = 0;

    if (model->graph == NULL)
        return -1;

    err += serialize_li16(ser, csfg_graph_node_count(model->graph));
    csfg_graph_for_each_node (model->graph, n)
    {
        na = node_attr_hmap_find(model->node_attrs, n->id);
        err += serialize_li32(ser, n->id);
        err += serialize_u8(ser, na->radius);
        err += serialize_u8(ser, na->color.r);
        err += serialize_u8(ser, na->color.g);
        err += serialize_u8(ser, na->color.b);
    }

    err += serialize_li16(ser, csfg_graph_edge_count(model->graph));
    csfg_graph_for_each_edge (model->graph, e)
    {
        ea = edge_attr_hmap_find(model->edge_attrs, e->id);
        err += serialize_li32(ser, e->id);
        err += serialize_cstr(ser, str_cstr(ea->expr_str));
        err += serialize_u8(ser, ea->color.r);
        err += serialize_u8(ser, ea->color.g);
        err += serialize_u8(ser, ea->color.b);
    }
    return err;
}

/* -------------------------------------------------------------------------- */
int graph_model_load_attrs(struct graph_model* model, struct deserializer* des)
{
    int count, node_id, edge_id;
    struct node_attr* na;
    struct edge_attr* ea;

    count = deserialize_li16(des);
    if (deserializer_err(des))
        return -1;
    while (count-- > 0)
    {
        node_id = deserialize_li32(des);
        switch (node_attr_hmap_emplace_or_get(&model->node_attrs, node_id, &na))
        {
            case HMAP_OOM   : return -1;
            case HMAP_NEW   : node_attr_init(na, rgb(0, 0, 0));
            case HMAP_EXISTS: break;
        }
        na->radius  = deserialize_u8(des);
        na->color.r = deserialize_u8(des);
        na->color.g = deserialize_u8(des);
        na->color.b = deserialize_u8(des);
    }

    count = deserialize_li16(des);
    if (deserializer_err(des))
        return -1;
    while (count-- > 0)
    {
        edge_id = deserialize_li32(des);
        switch (edge_attr_hmap_emplace_or_get(&model->edge_attrs, edge_id, &ea))
        {
            case HMAP_OOM   : return -1;
            case HMAP_NEW   : edge_attr_init(ea, rgb(0, 0, 0));
            case HMAP_EXISTS: break;
        }
        if (str_set_cstr(&ea->expr_str, deserialize_cstr(des)) != 0)
            return -1;
        ea->color.r = deserialize_u8(des);
        ea->color.g = deserialize_u8(des);
        ea->color.b = deserialize_u8(des);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
void graph_model_clear_attrs(struct graph_model* model)
{
    int i, id;
    struct edge_attr* ea;
    hmap_for_each (model->edge_attrs, i, id, ea)
    {
        (void)id, (void)ea;
        edge_attr_deinit(edge_attr_hmap_erase_slot(model->edge_attrs, i));
    }
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
static int find_nearest_node_in_direction(
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
static int find_nearest_edge_in_direction(
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
static int calc_circle_two_points(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2)
{
    double dx = x1 - x2;
    double dy = y1 - y2;
    *cx       = (x1 + x2) / 2;
    *cy       = (y1 + y2) / 2;
    *radius   = sqrt(dx * dx + dy * dy) / 2;
    return 1;
}
static int calc_circle_two_points_overlapping(
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
    if (x1 == x2 && y1 == y2 && (x1 != x3 || y1 != y3))
        return calc_circle_two_points(cx, cy, radius, x1, y1, x3, y3);
    if (x1 == x3 && y1 == y3 && (x1 != x2 || y1 != y2))
        return calc_circle_two_points(cx, cy, radius, x1, y1, x2, y2);
    if (x2 == x3 && y2 == y3 && (x1 != x2 || y1 != y2))
        return calc_circle_two_points(cx, cy, radius, x1, y1, x2, y2);
    return 0;
}
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
        return 0;

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
static int node_is_connected_to_edge(struct csfg_graph* g, int n_id, int e_id)
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
static int find_single_node_edge(const struct csfg_graph* g, int n_id)
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
        if (den != 0.0)
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
static void drag_edge_connected_to_node(
    struct csfg_graph* g,
    struct csfg_edge* e,
    int n1_idx,
    double n1_prev_x,
    double n1_prev_y,
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
    else if (
        calc_circle(
            &cx, &cy, &radius, n1_prev_x, n1_prev_y, e->x, e->y, n2->x, n2->y))
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

    if (is_near_any_other_node(e->x, e->y, g, NULL) ||
        is_near_any_other_edge(e->x, e->y, g, e))
    {
        auto_position_edge(g, e, n1, n2);
    }
}

/* -------------------------------------------------------------------------- */
static void drag_all_edges_connected_to_node(
    struct csfg_graph* g,
    int n1_id,
    double n1_prev_x,
    double n1_prev_y,
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
static void select_next_active_node_in_direction(
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
static void select_next_active_edge_in_direction(
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
static void
select_nearest_connected_node(struct graph_model* model, int edge_id)
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
static void
select_nearest_connected_edge(struct graph_model* model, int node_id)
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
static int reconnect_edge(
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
        current_node_id = target_node_id;
    }
    else if (e->n_idx_from == current_node_idx)
    {
        e->n_idx_from = target_node_idx;
        drag_edge_connected_to_node(
            model->graph, e, e->n_idx_from, n1->x, n1->y, 1);
        current_node_id = target_node_id;
    }

    return current_node_id;
}

/* -------------------------------------------------------------------------- */
static void
move_active_node_in_direction(struct graph_model* model, double dx, double dy)
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

    model->icb->graph_layout_changed(model->cb, model->plugin_ctx);
}

/* -------------------------------------------------------------------------- */
static void
move_active_edge_in_direction(struct graph_model* model, double dx, double dy)
{
    struct csfg_edge* e = find_edge(model->graph, model->active_edge_id);
    if (e == NULL)
        return;

    e->x += dx * GRID;
    e->y += dy * GRID;

    model->icb->graph_layout_changed(model->cb, model->plugin_ctx);
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
static int create_new_node(struct graph_model* model, int n_id_prev)
{
    struct csfg_node* n;
    struct node_attr* na;
    const struct csfg_node* n_prev;
    int n_idx;

    if (model->graph == NULL)
        return -1;

    n_idx = csfg_graph_add_node(model->graph, "V1");
    n     = csfg_graph_get_node(model->graph, n_idx);
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

    if (graph == NULL)
        return -1;

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

    if (graph == NULL)
        return -1;

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
static void flip_active_edge(struct graph_model* model)
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
static void finish_editing(GtkEntry* entry, gpointer user_data)
{
    int n_idx;
    struct csfg_node* n;
    struct csfg_edge* e;
    struct edge_attr* ea;
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;

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
    push_undo_state(model);
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
    struct graph_model* model = editor->model;

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
            push_undo_state(model);
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
            notify_graph_changed(model);
            push_undo_state(model);
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
            push_undo_state(model);
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
static void button_show_shortcuts_cb(GtkButton* button, gpointer user_data)
{
    GraphEditor* editor = user_data;
    editor->show_shortcuts =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE;
    gtk_widget_queue_draw(editor->drawing_area);
}
static void button_show_7steps_cb(GtkButton* button, gpointer user_data)
{
    GraphEditor* editor = user_data;
    editor->show_seven_steps =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE;
    gtk_widget_queue_draw(editor->drawing_area);
}
static gboolean
shortcut_undo_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    undo(editor->model);
    notify_graph_changed(editor->model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_undo_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_undo_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_redo_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    redo(editor->model);
    notify_graph_changed(editor->model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_redo_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_redo_cb(NULL, NULL, user_data);
}
static gboolean
shortcut_node_left_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    node_direction_action(editor->model, -1, 0);
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
    node_direction_action(editor->model, 1, 0);
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
    node_direction_action(editor->model, 0, -1);
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
    node_direction_action(editor->model, 0, 1);
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
    edge_direction_action(editor->model, -1, 0);
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
    edge_direction_action(editor->model, 1, 0);
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
    edge_direction_action(editor->model, 0, -1);
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
    edge_direction_action(editor->model, 0, 1);
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
    struct graph_model* model = editor->model;
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
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    model->active_node_id =
        create_new_node(editor->model, model->active_node_id);
    model->active_edge_id    = -1;
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;
    notify_graph_changed(model);
    push_undo_state(model);
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
    struct graph_model* model = editor->model;
    int prev_active_node_id   = model->active_node_id;
    (void)widget, (void)unused;
    model->active_node_id =
        create_new_node(editor->model, model->active_node_id);
    model->active_edge_id    = -1;
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;
    create_new_edge(editor->model, prev_active_node_id, model->active_node_id);
    notify_graph_changed(model);
    push_undo_state(model);
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
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    model->active_node_id    = delete_node(model, model->active_node_id);
    model->active_edge_id    = delete_edge(model, model->active_edge_id);
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;
    notify_graph_changed(model);
    push_undo_state(model);
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
    struct graph_model* model = editor->model;
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
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    create_new_edge(model, model->marked_node_id, model->active_node_id);
    notify_graph_changed(model);
    push_undo_state(model);
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
    struct graph_model* model = editor->model;
    model->node_in_id         = model->active_node_id;
    notify_graph_changed(model);
    push_undo_state(model);
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
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    model->node_out_id = model->active_node_id;
    notify_graph_changed(model);
    push_undo_state(model);
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
    struct graph_model* model = editor->model;
    struct csfg_edge* e;
    int e_id;
    (void)widget, (void)unused;
    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
            model->reconnect_node_id = -1;
            model->reconnect_edge_id = -1;
            if (model->active_node_id > -1)
            {
                model->reconnect_node_id = model->active_node_id;
                select_nearest_connected_edge(model, model->reconnect_node_id);
            }
            else if (model->active_edge_id > -1)
            {
                model->reconnect_edge_id = model->active_edge_id;
                select_nearest_connected_node(model, model->reconnect_edge_id);
            }
            else
                break;

            /* QoL: If it's a loop, we can skip selecting the "from" node
             * because there's no ambiguity */
            e = find_edge(model->graph, model->reconnect_edge_id);
            if (e != NULL && e->n_idx_from == e->n_idx_to)
            {
                model->reconnect_node_id =
                    csfg_graph_get_node(model->graph, e->n_idx_to)->id;
                model->mode = MODE_RECONNECT_TO;
            }
            /* QoL: If there is only one edge connected to this node, we can
             * skip the "from" edge because there's no ambiguity */
            else if (
                (e_id = find_single_node_edge(
                     model->graph, model->reconnect_node_id)) > -1)
            {
                model->reconnect_edge_id = e_id;
                model->active_node_id    = model->reconnect_node_id;
                model->active_edge_id    = -1;
                model->mode              = MODE_RECONNECT_TO;
            }
            else
                model->mode = MODE_RECONNECT_FROM;

            break;

        case MODE_RECONNECT_FROM:
            if (model->reconnect_node_id > -1)
            {
                model->reconnect_edge_id = model->active_edge_id;
                /* Set valid active node so that the next node move command
                 * finds the nearest node, instead of snapping to the furthest
                 * node */
                model->active_node_id = model->reconnect_node_id;
                model->active_edge_id = -1;

                model->mode = MODE_RECONNECT_TO;
            }
            else if (model->reconnect_edge_id > -1)
            {
                model->reconnect_node_id = model->active_node_id;
                model->mode              = MODE_RECONNECT_TO;
            }
            else
                model->mode = MODE_NORMAL;
            break;

        case MODE_RECONNECT_TO:
            model->reconnect_node_id = -1;
            model->reconnect_edge_id = -1;
            model->mode              = MODE_NORMAL;
            break;
    }
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_reconnect_edge_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_reconnect_edge_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_flip_edge_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    flip_active_edge(editor->model);
    notify_graph_changed(editor->model);
    push_undo_state(editor->model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_flip_edge_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_flip_edge_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_graph_mode_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    return TRUE;
}
static void button_graph_mode_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_graph_mode_cb(NULL, NULL, user_data);
}

static gboolean
shortcut_draw_mode_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)widget, (void)unused;
    return TRUE;
}
static void button_draw_mode_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_draw_mode_cb(NULL, NULL, user_data);
}

static void shortcut_color_cb(GraphEditor* editor, int idx)
{
    gtk_toggle_button_set_active(editor->color_buttons[idx], TRUE);
}
#define X(idx, shortcut, default_color)                                        \
    static gboolean shortcut_color##idx##_cb(                                  \
        GtkWidget* widget, GVariant* unused, gpointer user_data)               \
    {                                                                          \
        (void)widget, (void)unused;                                            \
        shortcut_color_cb(user_data, idx - 1);                                 \
        return TRUE;                                                           \
    }
COLOR_BUTTONS_LIST
#undef X

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

    draw_grid_with_size(cr, pan_x, pan_y, width, height, GRID, 0.8);
    draw_grid_with_size(cr, pan_x, pan_y, width, height, GRID * 6, 0.7);
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
    double text_angle, arrow_angle;

    orientation = calc_circle(
        &cx, &cy, &radius, src_x, src_y, control_x, control_y, dst_x, dst_y);
    if (orientation)
    {
        a1 = atan2(src_y - cy, src_x - cx);
        a2 = atan2(dst_y - cy, dst_x - cx);
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, r, g, b);
        if (orientation > 0)
            cairo_arc(cr, cx, cy, radius, a1, a2);
        else
            cairo_arc(cr, cx, cy, radius, a2, a1);
        cairo_stroke(cr);

        text_angle = atan2(cy - control_y, cx - control_x);
        arrow_angle =
            orientation < 0 ? text_angle + M_PI / 2 : text_angle - M_PI / 2;
    }
    else if (
        calc_circle_two_points_overlapping(
            &cx,
            &cy,
            &radius,
            src_x,
            src_y,
            control_x,
            control_y,
            dst_x,
            dst_y))
    {
        arrow_angle = atan2(src_x - control_x, control_y - src_y);
        text_angle  = arrow_angle - M_PI / 2;
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
    else
    {
        arrow_angle = atan2(dst_y - src_y, dst_x - src_x);
        text_angle  = arrow_angle + M_PI / 2;
        cairo_set_source_rgb(cr, r, g, b);
        cairo_move_to(cr, src_x, src_y);
        cairo_line_to(cr, dst_x, dst_y);
        cairo_stroke(cr);
    }

    if (text_angle < M_PI)
        text_angle += M_PI;
    draw_arrow(cr, control_x, control_y, arrow_angle, r, g, b);
    draw_text(cr, expr_str, control_x, control_y, text_angle);
}

/* -------------------------------------------------------------------------- */
static void draw_help(
    cairo_t* cr, RsvgHandle* svg_7steps, int show_7steps, int show_shortcuts)
{
    RsvgRectangle viewport = {20, 20, 136, 334};
    if (show_7steps)
        rsvg_handle_render_document(svg_7steps, cr, &viewport, NULL);
    if (show_shortcuts)
        draw_text(cr, "(TODO)", 0, show_7steps ? 380 : 20, 0);
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
    const struct graph_model* model = editor->model;
    (void)area;

    draw_help(
        cr,
        editor->seven_steps,
        editor->show_seven_steps,
        editor->show_shortcuts);

    cairo_translate(cr, editor->pan_x, editor->pan_y);
    cairo_scale(cr, editor->zoom, editor->zoom);

    cairo_set_line_width(cr, 0.5 / editor->zoom);
    draw_grid(cr, editor->pan_x, editor->pan_y, width, height, editor->zoom);

    cairo_set_line_width(cr, 2.0 / editor->zoom);
    csfg_graph_for_each_edge (model->graph, e)
    {
        struct color color;
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
        color = ea->color;
        if (e->id == model->active_edge_id)
            color = highlight_color(ea->color);
        if (e->id == model->reconnect_edge_id)
            color = highlight_color(ea->color);
        draw_edge(
            cr,
            n_src->x,
            n_src->y,
            e->x,
            e->y,
            n_dst->x,
            n_dst->y,
            str_cstr(ea->expr_str),
            color.r / 255.0,
            color.g / 255.0,
            color.b / 255.0);
        if ((model->mode == MODE_MOVE && e->id == model->active_edge_id) ||
            (model->mode == MODE_RECONNECT_FROM &&
             e->id == model->reconnect_edge_id))
        {
            draw_move_active_symbol(
                cr,
                e->x,
                e->y,
                color.r / 255.0,
                color.g / 255.0,
                color.b / 255.0);
        }
    }

    csfg_graph_for_each_node (model->graph, n)
    {
        struct color color;
        const struct node_attr* na =
            node_attr_hmap_find(model->node_attrs, n->id);
        if (na == NULL)
            continue;
        color = na->color;
        if (n->id == model->active_node_id)
            color = highlight_color(na->color);
        if (n->id == model->reconnect_node_id)
            color = highlight_color(na->color);
        draw_node(
            cr,
            n->x,
            n->y,
            na->radius,
            str_cstr(n->name),
            color.r / 255.0,
            color.g / 255.0,
            color.b / 255.0,
            n->id == model->marked_node_id,
            n->id == model->node_in_id,
            n->id == model->node_out_id);
        if ((model->mode == MODE_MOVE && n->id == model->active_node_id) ||
            (model->mode == MODE_RECONNECT_FROM &&
             n->id == model->reconnect_node_id))
        {
            draw_move_active_symbol(
                cr,
                n->x,
                n->y,
                color.r / 255.0,
                color.g / 255.0,
                color.b / 255.0);
        }
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
    struct graph_model* model = editor->model;
    (void)gesture;

    gtk_widget_grab_focus(GTK_WIDGET(editor));

    x = (x - editor->pan_x) / editor->zoom;
    y = (y - editor->pan_y) / editor->zoom;

    switch (model->mode)
    {
        case MODE_NORMAL:
            try_select_edge_or_node(model, x, y);
            if (n_press == 2 /* double-click */)
                start_editing_active_node_or_edge(editor);
            break;
        case MODE_MOVE: break;
        case MODE_RECONNECT_FROM:
            if (model->reconnect_node_id > -1)
            {
                int e_id = try_select_edge(x, y, model->graph);
                if (node_is_connected_to_edge(
                        model->graph, model->reconnect_node_id, e_id))
                {
                    model->active_edge_id    = e_id;
                    model->active_node_id    = -1;
                    model->reconnect_edge_id = e_id;
                    model->mode              = MODE_RECONNECT_TO;
                }
            }
            if (model->reconnect_edge_id > -1)
            {
                int n_id =
                    try_select_node(x, y, model->graph, model->node_attrs);
                if (node_is_connected_to_edge(
                        model->graph, n_id, model->reconnect_edge_id))
                {
                    model->active_edge_id    = -1;
                    model->active_node_id    = n_id;
                    model->reconnect_node_id = n_id;
                    model->mode              = MODE_RECONNECT_TO;
                }
            }
            break;
        case MODE_RECONNECT_TO: {
            int n_id = try_select_node(x, y, model->graph, model->node_attrs);
            if (n_id > -1)
            {
                model->active_node_id = reconnect_edge(
                    model,
                    model->reconnect_edge_id,
                    model->reconnect_node_id,
                    n_id);
                notify_graph_changed(model);
            }

            model->reconnect_node_id = -1;
            model->reconnect_edge_id = -1;
            model->active_edge_id    = -1;
            model->mode              = MODE_NORMAL;
        }
        break;
    }

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
    struct graph_model* model = editor->model;
    (void)gesture;

    x = (x - editor->pan_x) / editor->zoom;
    y = (y - editor->pan_y) / editor->zoom;

    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
            try_select_edge_or_node(model, x, y);
            if ((e = find_edge(model->graph, model->active_edge_id)) != NULL)
            {
                editor->drag_offset_x = round((x - e->x) / GRID) * GRID;
                editor->drag_offset_y = round((y - e->y) / GRID) * GRID;
            }
            else if (
                (n = find_node(model->graph, model->active_node_id)) != NULL)
            {
                editor->drag_offset_x = round((x - n->x) / GRID) * GRID;
                editor->drag_offset_y = round((y - n->y) / GRID) * GRID;
            }
            break;
        case MODE_RECONNECT_FROM:
        case MODE_RECONNECT_TO  : break;
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
    struct graph_model* model = editor->model;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - editor->pan_x) / editor->zoom;
    y = (start_y + offset_y - editor->pan_y) / editor->zoom;

    snap_size = GRID;
    event =
        gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(gesture));
    if (event)
    {
        GdkModifierType mods = gdk_event_get_modifier_state(event);
        if (mods & GDK_CONTROL_MASK)
            snap_size *= 6;
    }

    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
            n = find_node(model->graph, model->active_node_id);
            e = find_edge(model->graph, model->active_edge_id);

            if (n != NULL)
            {
                double old_x = n->x;
                double old_y = n->y;
                n->x =
                    round((x - editor->drag_offset_x) / snap_size) * snap_size;
                n->y =
                    round((y - editor->drag_offset_y) / snap_size) * snap_size;
                drag_all_edges_connected_to_node(
                    model->graph, model->active_node_id, old_x, old_y, 0);
            }
            else if (e != NULL)
            {
                e->x =
                    round((x - editor->drag_offset_x) / snap_size) * snap_size;
                e->y =
                    round((y - editor->drag_offset_y) / snap_size) * snap_size;
            }
            break;
        case MODE_RECONNECT_FROM:
        case MODE_RECONNECT_TO  : break;
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
static void add_toolbar_separator(GtkBox* toolbar)
{
    GtkWidget* sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(sep, 8);
    gtk_widget_set_margin_end(sep, 8);
    gtk_box_append(toolbar, sep);
}

/* -------------------------------------------------------------------------- */
static void setup_global_shortcuts(GraphEditor* editor)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction* action;
    GtkShortcut* shortcut;
    int i;

    struct
    {
        guint keyval;
        GdkModifierType modifiers;
        gboolean (*callback)(GtkWidget*, GVariant*, gpointer);
    } shortcuts[] = {
        /* clang-format off */
        {GDK_KEY_z, GDK_CONTROL_MASK,     shortcut_undo_cb},
        {GDK_KEY_z, GDK_CONTROL_MASK | GDK_SHIFT_MASK, shortcut_redo_cb},
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
        {GDK_KEY_f, GDK_NO_MODIFIER_MASK, shortcut_flip_edge_cb},
        {GDK_KEY_d, GDK_NO_MODIFIER_MASK, shortcut_draw_mode_cb},
        {GDK_KEY_g, GDK_NO_MODIFIER_MASK, shortcut_graph_mode_cb},
#define X(idx, shortcut, default_color) \
        {GDK_KEY_##idx, GDK_NO_MODIFIER_MASK,shortcut_color##idx##_cb},
        COLOR_BUTTONS_LIST
        #undef X
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
}

/* -------------------------------------------------------------------------- */
static void add_toolbar_buttons(GraphEditor* editor, GtkBox* toolbar)
{
    int i;
    GtkWidget* image;
    GtkWidget* button;
    GtkWidget* prev_button;
    struct str* str;

    str_init(&str);

    struct
    {
        const char* tooltip;
        const char* shortcut;
        const char* icon;
        void (*callback)(GtkButton*, gpointer);
        unsigned is_toggle_button     : 1;
        unsigned group_with_previous  : 1;
        unsigned is_initially_pressed : 1;
    } buttons[] = {
        /* clang-format off */
        {"Show Shortcut List",          NULL,           "help-shortcuts.svg",     button_show_shortcuts_cb,    1, 0, 0},
        {"Show the '7 Steps'",          NULL,           "help-7-steps.svg",       button_show_7steps_cb,       1, 0, 0},
        {"Undo",                        "ctrl+z",       "rotate-ccw.svg",         button_undo_cb,              0, 0, 0},
        {"Redo",                        "ctrl+shift+z", "rotate-cw.svg",          button_redo_cb,              0, 0, 0},
        {NULL,                          NULL,           NULL,                     NULL,                        0, 0, 0},
        {"New Node",                    "n",            "new-node.svg",           button_new_node_cb,          0, 0, 0},
        {"New Connected Node",          "shift+n",      "new-node-with-edge.svg", button_new_node_and_edge_cb, 0, 0, 0},
        {"New Edge",                    "e",            "new-edge.svg",           button_new_edge_cb,          0, 0, 0},
        {"Mark Node (for connections)", "s",            "mark-node.svg",          button_mark_node_cb,         0, 0, 0},
        {"Set Input Node",              "i",            "input-node.svg",         button_set_in_node_cb,       0, 0, 0},
        {"Set Output Node",             "o",            "output-node.svg",        button_set_out_node_cb,      0, 0, 0},
        {"Reconnect Edge",              "r",            "reconnect-edge.svg",     button_reconnect_edge_cb,    0, 0, 0},
        {"Flip Edge",                   "f",            "flip-edge.svg",          button_flip_edge_cb,         0, 0, 0},
        {"Delete",                      "x",            "delete.svg",             button_delete_cb,            0, 0, 0},
        {NULL,                          NULL,           NULL,                     NULL,                        0, 0, 0},
        {"Graph Mode",                  "g",            "mouse.svg",              button_graph_mode_cb,        1, 0, 1},
        {"Draw Mode",                   "d",            "draw.svg",               button_draw_mode_cb,         1, 1, 0},
        /* clang-format on */
    };

    prev_button = NULL;
    for (i = 0; i != G_N_ELEMENTS(buttons); ++i)
    {
        if (buttons[i].tooltip == NULL)
        {
            add_toolbar_separator(toolbar);
            continue;
        }

        button = buttons[i].is_toggle_button ? gtk_toggle_button_new()
                                             : gtk_button_new();
        gtk_box_append(toolbar, button);

        str_set_cstr(&str, "/ch/thecomet/dpsfg/graph-editor/icons/");
        str_append_cstr(&str, buttons[i].icon);
        image = gtk_image_new_from_resource(str_cstr(str));
        gtk_button_set_child(GTK_BUTTON(button), image);

        str_set_cstr(&str, buttons[i].tooltip);
        str_append_cstr(&str, "\n");
        if (buttons[i].shortcut != NULL)
        {
            str_append_cstr(&str, "Shortcut: ");
            str_append_cstr(&str, buttons[i].shortcut);
        }
        gtk_widget_set_tooltip_text(button, str_cstr(str));

        if (buttons[i].is_toggle_button)
        {
            if (buttons[i].group_with_previous)
                gtk_toggle_button_set_group(
                    GTK_TOGGLE_BUTTON(prev_button), GTK_TOGGLE_BUTTON(button));
            else
                prev_button = button;

            if (buttons[i].is_initially_pressed)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
        }

        g_signal_connect(
            button, "clicked", G_CALLBACK(buttons[i].callback), editor);
    }

    str_deinit(str);
}

/* -------------------------------------------------------------------------- */
static void pick_color_cb(ColorPicker* picker, gpointer user_data)
{
    GraphEditor* editor        = user_data;
    editor->model->graph_color = gdk_to_color(color_picker_get_color(picker));
}

/* -------------------------------------------------------------------------- */
static void
add_toolbar_color_picker_buttons(GraphEditor* editor, GtkBox* toolbar)
{
    int i;
    static const uint32_t default_colors[COLOR_BUTTONS_COUNT] = {
#define X(idx, shortcut, default_color) default_color,
        COLOR_BUTTONS_LIST
#undef X
    };

    editor->color_buttons[0] =
        GTK_TOGGLE_BUTTON(color_picker_new(hex_gdk(default_colors[0])));
    gtk_toggle_button_set_active(editor->color_buttons[0], TRUE);
    gtk_box_append(toolbar, GTK_WIDGET(editor->color_buttons[0]));
    g_signal_connect(
        editor->color_buttons[0],
        "color-selected",
        G_CALLBACK(pick_color_cb),
        editor);

    for (i = 1; i != COLOR_BUTTONS_COUNT; ++i)
    {
        editor->color_buttons[i] =
            GTK_TOGGLE_BUTTON(color_picker_new(hex_gdk(default_colors[i])));
        gtk_toggle_button_set_group(
            editor->color_buttons[i], editor->color_buttons[0]);
        gtk_box_append(toolbar, GTK_WIDGET(editor->color_buttons[i]));
        g_signal_connect(
            editor->color_buttons[i],
            "color-selected",
            G_CALLBACK(pick_color_cb),
            editor);
    }
}

/* -------------------------------------------------------------------------- */
static GtkWidget* create_toolbar(GraphEditor* editor)
{
    GtkBox* toolbar = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    add_toolbar_buttons(editor, toolbar);
    add_toolbar_separator(toolbar);
    add_toolbar_color_picker_buttons(editor, toolbar);
    return GTK_WIDGET(toolbar);
}

/* -------------------------------------------------------------------------- */
static RsvgHandle* load_svg(const char* resource)
{
    GError* error = NULL;
    GBytes* bytes =
        g_resources_lookup_data(resource, G_RESOURCE_LOOKUP_FLAGS_NONE, &error);

    if (bytes == NULL)
    {
        g_printerr("%s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    RsvgHandle* svg = rsvg_handle_new_from_data(
        g_bytes_get_data(bytes, NULL), g_bytes_get_size(bytes), &error);

    g_bytes_unref(bytes);

    if (svg == NULL)
    {
        g_printerr("%s\n", error->message);
        g_error_free(error);
    }

    return svg;
}

/* -------------------------------------------------------------------------- */
static void graph_editor_init(GraphEditor* self)
{
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

    self->show_shortcuts   = 0;
    self->show_seven_steps = 0;

    self->seven_steps =
        load_svg("/ch/thecomet/dpsfg/graph-editor/images/7-steps.svg");

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

    setup_global_shortcuts(self);

    gtk_orientable_set_orientation(
        GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(self), create_toolbar(self));
    gtk_box_append(GTK_BOX(self), self->overlay);
    gtk_widget_set_focusable(GTK_WIDGET(self), TRUE);
}

/* -------------------------------------------------------------------------- */
static void graph_editor_finalize(GObject* object)
{
    GraphEditor* self = PLUGIN_GRAPH_EDITOR(object);
    g_object_unref(self->seven_steps);
    G_OBJECT_CLASS(graph_editor_parent_class)->finalize(object);
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
GraphEditor* graph_editor_new(struct graph_model* model)
{
    GraphEditor* editor = g_object_new(PLUGIN_TYPE_GRAPH_EDITOR, NULL);
    editor->model       = model;
    return editor;
}

/* -------------------------------------------------------------------------- */
void graph_editor_redraw_graph(GraphEditor* editor)
{
    gtk_widget_queue_draw(editor->drawing_area);
}
