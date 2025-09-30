#include "csfg/graph/graph.h"
#include "csfg/util/hmap.h"
#include "csfg/util/log.h"
#include "csfg/util/str.h"
#include "graph-editor/graph_editor.h"

static const int GRID_WIDTH = 20;
static const int ARROW_RADIUS = 8;

struct node_attr
{
    double x, y;
    double radius;
};

struct edge_attr
{
    double x, y;
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

    struct csfg_graph*     graph;
    struct node_attr_hmap* node_attrs;
    struct edge_attr_hmap* edge_attrs;

    // View transform
    double zoom;
    double pan_x, pan_y;
    double pan_offset_x, pan_offset_y;

    // Drag state
    int    active_node;
    int    active_edge;
    double drag_offset_x, drag_offset_y;

    /* It's difficult to get the mouse coordinates in some callbacks, so we use
     * GtkEventControllerMotion to store them here */
    double mouse_x, mouse_y;
};

G_DEFINE_DYNAMIC_TYPE(GraphEditor, graph_editor, GTK_TYPE_BOX)

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
    cairo_set_line_width(cr, 0.5 / zoom);
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
static void draw_node(
    cairo_t*    cr,
    double      x,
    double      y,
    double      radius,
    const char* name,
    double      r,
    double      g,
    double      b)
{
    draw_text(cr, name, x, y, M_PI);

    cairo_set_source_rgb(cr, r, g, b);
    cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
    cairo_fill(cr);
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

    double angle1 = atan2(y1 - *cy, x1 - *cx);
    double angle2 = atan2(y2 - *cy, x2 - *cx);
    double angle_control = atan2(y3 - *cy, x3 - *cx);

    double vx1 = (x2 - x1), vy1 = (y2 - y1);
    double vx2 = (x3 - x1), vy2 = (y3 - y1);

    double cross = (vx1 * vy2 - vy1 * vx2);

    return cross > 0 ? 1 : -1;
}

/* -------------------------------------------------------------------------- */
static void draw_arrow(
    cairo_t* cr,
    double   x,
    double   y,
    double   a,
    double   r,
    double   g,
    double   b,
    double   zoom)
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
    cairo_line_to(cr, x1, y1);
    cairo_fill(cr);
}

/* -------------------------------------------------------------------------- */
static void draw_edge(
    cairo_t* cr,
    double   src_x,
    double   src_y,
    double   control_x,
    double   control_y,
    double   dst_x,
    double   dst_y,
    double   r,
    double   g,
    double   b,
    double   zoom)
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
        cairo_set_line_width(cr, 2.0 / zoom);
        cairo_move_to(cr, src_x, src_y);
        cairo_line_to(cr, dst_x, dst_y);
        cairo_stroke(cr);
        draw_arrow(cr, control_x, control_y, arrow_angle, r, g, b, zoom);
        draw_text(cr, "G1 + G2 + s*C", control_x, control_y, text_angle);
        return;
    }

    a1 = atan2(src_y - cy, src_x - cx);
    a2 = atan2(dst_y - cy, dst_x - cx);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 2.0 / zoom);
    if (orientation > 0)
        cairo_arc(cr, cx, cy, radius, a1, a2);
    else
        cairo_arc(cr, cx, cy, radius, a2, a1);
    cairo_stroke(cr);

    double text_angle = atan2(control_y - cy, control_x - cx);
    double arrow_angle =
        orientation > 0 ? text_angle + M_PI / 2 : text_angle - M_PI / 2;
    draw_arrow(cr, control_x, control_y, arrow_angle, r, g, b, zoom);
    draw_text(cr, "G1 + G2 + s*C", control_x, control_y, text_angle);
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
    int                     idx, id;
    GraphEditor*            editor = user_pointer;
    cairo_translate(cr, editor->pan_x, editor->pan_y);
    cairo_scale(cr, editor->zoom, editor->zoom);

    draw_grid(cr, editor->pan_x, editor->pan_y, width, height, editor->zoom);

    vec_for_each_node (editor->graph, n)
    {
        const struct node_attr* na =
            node_attr_hmap_find(editor->node_attrs, n->id);
        if (na == NULL)
            continue;
        draw_node(
            cr, na->x, na->y, na->radius, str_cstr(n->name), 0.2, 0.2, 0.2);
    }

    vec_for_each_edge (editor->graph, e)
    {
        const struct csfg_node* src_n = vec_get(editor->graph->nodes, e->from);
        const struct csfg_node* dst_n = vec_get(editor->graph->nodes, e->to);
        const struct edge_attr* ea =
            edge_attr_hmap_find(editor->edge_attrs, e->id);
        const struct node_attr* src_na =
            node_attr_hmap_find(editor->node_attrs, src_n->id);
        const struct node_attr* dst_na =
            node_attr_hmap_find(editor->node_attrs, dst_n->id);
        if (ea == NULL || src_na == NULL || dst_na == NULL)
            continue;
        draw_edge(
            cr,
            src_na->x,
            src_na->y,
            ea->x,
            ea->y,
            dst_na->x,
            dst_na->y,
            0.2,
            0.2,
            0.2,
            editor->zoom);
    }
}

/* -------------------------------------------------------------------------- */
static void finish_editing(GtkEntry* e, gpointer user_pointer);
static void
on_entry_focus_changed(GObject* obj, GParamSpec* pspec, gpointer user_data)
{
    GtkWidget* entry = GTK_WIDGET(obj);
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
static int try_select_node(
    double                       mouse_x,
    double                       mouse_y,
    const struct csfg_graph*     graph,
    const struct node_attr_hmap* attrs)
{
    double                  dx, dy;
    int                     idx;
    const struct csfg_node* n;

    vec_enumerate_nodes (graph, idx, n)
    {
        const struct node_attr* na = node_attr_hmap_find(attrs, n->id);
        if (na == NULL)
            continue;

        dx = mouse_x - na->x;
        dy = mouse_y - na->y;
        if (dx * dx + dy * dy < na->radius * na->radius)
            return idx;
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
static int try_select_edge(
    double                       mouse_x,
    double                       mouse_y,
    const struct csfg_graph*     graph,
    const struct edge_attr_hmap* attrs)
{
    double                  dx, dy;
    int                     idx;
    const struct csfg_edge* e;

    vec_enumerate_edges (graph, idx, e)
    {
        const struct edge_attr* ea = edge_attr_hmap_find(attrs, e->id);
        if (ea == NULL)
            continue;

        dx = mouse_x - ea->x;
        dy = mouse_y - ea->y;
        if (dx * dx + dy * dy < ARROW_RADIUS * ARROW_RADIUS)
            return idx;
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
static void finish_editing(GtkEntry* e, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;
    const char*  text = gtk_editable_get_text(GTK_EDITABLE(e));
    if (editor->active_node)
    {
        // g_free(editor->active_node->label);
        // editor->active_node->label = g_strdup(text);
    }
    gtk_widget_unparent(editor->entry);
    editor->entry = NULL;
    gtk_widget_queue_draw(GTK_WIDGET(user_pointer));
}

static void click_begin(
    GtkGestureClick* gesture,
    int              n_press,
    double           x,
    double           y,
    gpointer         user_pointer)
{
    GraphEditor* editor = user_pointer;

    x = (x - editor->pan_x) / editor->zoom;
    y = (y - editor->pan_y) / editor->zoom;

    editor->active_node = -1;
    editor->active_edge = -1;

    editor->active_node =
        try_select_node(x, y, editor->graph, editor->node_attrs);
    editor->active_edge =
        try_select_edge(x, y, editor->graph, editor->edge_attrs);

    if (editor->active_node > -1 && n_press == 2 /* double-click */)
    {
        const struct csfg_node* n =
            vec_get(editor->graph->nodes, editor->active_node);
        const struct node_attr* na =
            node_attr_hmap_find(editor->node_attrs, n->id);
        if (na != NULL)
            start_editing(editor, na->x, na->y, na->radius, str_cstr(n->name));
        return;
    }
}

/* -------------------------------------------------------------------------- */
static void click_end(
    GtkGestureClick* gesture,
    int              n_press,
    double           x,
    double           y,
    gpointer         user_pointer)
{
    GraphEditor* editor = user_pointer;
    if (n_press == 1)
    {
        editor->active_node = -1;
        editor->active_edge = -1;
    }
}

/* -------------------------------------------------------------------------- */
static void
drag_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;

    editor->active_node =
        try_select_node(x, y, editor->graph, editor->node_attrs);
    editor->active_edge =
        try_select_edge(x, y, editor->graph, editor->edge_attrs);

    if (editor->active_node > -1)
    {
        const struct csfg_node* n =
            vec_get(editor->graph->nodes, editor->active_node);
        const struct node_attr* na =
            node_attr_hmap_find(editor->node_attrs, n->id);
        editor->drag_offset_x = round((x - na->x) / 20) * 20;
        editor->drag_offset_y = round((y - na->y) / 20) * 20;
    }
    else if (editor->active_edge > -1)
    {
        const struct csfg_edge* e =
            vec_get(editor->graph->edges, editor->active_edge);
        const struct edge_attr* ea =
            edge_attr_hmap_find(editor->edge_attrs, e->id);
        editor->drag_offset_x = round((x - ea->x) / 20) * 20;
        editor->drag_offset_y = round((y - ea->y) / 20) * 20;
    }
}

/* -------------------------------------------------------------------------- */
static void drag_update(
    GtkGestureDrag* gesture,
    double          offset_x,
    double          offset_y,
    gpointer        user_pointer)
{
    double       start_x, start_y, x, y;
    GraphEditor* editor = user_pointer;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - editor->pan_x) / editor->zoom;
    y = (start_y + offset_y - editor->pan_y) / editor->zoom;

    if (editor->active_node > -1)
    {
        const struct csfg_node* n =
            vec_get(editor->graph->nodes, editor->active_node);
        struct node_attr* na = node_attr_hmap_find(editor->node_attrs, n->id);
        na->x = round((x - editor->drag_offset_x) / 20) * 20;
        na->y = round((y - editor->drag_offset_y) / 20) * 20;
    }
    else if (editor->active_edge > -1)
    {
        const struct csfg_edge* e =
            vec_get(editor->graph->edges, editor->active_edge);
        struct edge_attr* ea = edge_attr_hmap_find(editor->edge_attrs, e->id);
        ea->x = round((x - editor->drag_offset_x) / 20) * 20;
        ea->y = round((y - editor->drag_offset_y) / 20) * 20;
    }

    gtk_widget_queue_draw(GTK_WIDGET(editor->drawing_area));
}

/* -------------------------------------------------------------------------- */
static void
drag_end(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;
    editor->active_node = -1;
    editor->active_edge = -1;
}

/* -------------------------------------------------------------------------- */
static void
pan_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;
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
    gtk_widget_queue_draw(GTK_WIDGET(editor->drawing_area));
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

    gtk_widget_queue_draw(GTK_WIDGET(editor->drawing_area));
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

    self->zoom = 1.0;
    self->pan_x = 500.0;
    self->pan_y = 500.0;
    self->active_node = -1;
    self->active_edge = -1;
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

    gtk_box_append(GTK_BOX(self), self->overlay);
}

/* -------------------------------------------------------------------------- */
static void graph_editor_finalize(GObject* obj)
{
    log_dbg("graph_editor_finalize()\n");
    GraphEditor* self = PLUGIN_GRAPH_EDITOR(obj);
    edge_attr_hmap_deinit(self->edge_attrs);
    node_attr_hmap_deinit(self->node_attrs);
}

/* -------------------------------------------------------------------------- */
static void graph_editor_class_init(GraphEditorClass* class)
{
    log_dbg("graph_editor_class_init()\n");
    GObjectClass*   object_class = G_OBJECT_CLASS(class);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(class);

    object_class->finalize = graph_editor_finalize;
}

/* -------------------------------------------------------------------------- */
static void graph_editor_class_finalize(GraphEditorClass* class)
{
    log_dbg("graph_editor_class_finalize()\n");
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
GraphEditor* graph_editor_new(void)
{
    return g_object_new(PLUGIN_TYPE_GRAPH_EDITOR, NULL);
}

/* -------------------------------------------------------------------------- */
static void
auto_position_node(struct node_attr* na, const struct node_attr_hmap* attrs)
{
    int                     idx, id;
    const struct node_attr* other_na;
    const double            spacing = GRID_WIDTH * 6;
    const double            row_end = (int)sqrt(hmap_count(attrs)) * spacing;

    double x = 0.0, y = 0.0;
try_again:
    hmap_for_each (attrs, idx, id, other_na)
    {
        double dx = x - other_na->x;
        double dy = y - other_na->y;
        if (na == other_na)
            continue;
        if (dx * dx - dy * dy < spacing * spacing / 4)
        {
            y += spacing;
            if (y >= row_end)
            {
                y = 0.0;
                x += spacing;
            }
            goto try_again;
        }
    }

    na->x = x;
    na->y = y;
}

/* -------------------------------------------------------------------------- */
static void auto_position_edge(struct edge_attr* ea)
{
    ea->x = 60;
    ea->y = 0.0;
}

/* -------------------------------------------------------------------------- */
void graph_editor_set_graph(GraphEditor* editor, struct csfg_graph* g)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    struct node_attr*       na;
    struct edge_attr*       ea;

    vec_for_each (g->nodes, n)
        switch (node_attr_hmap_emplace_or_get(&editor->node_attrs, n->id, &na))
        {
            case HMAP_OOM: return;
            case HMAP_NEW:
                na->radius = 10;
                auto_position_node(na, editor->node_attrs);
            case HMAP_EXISTS: break;
        }

    vec_for_each (g->edges, e)
        switch (edge_attr_hmap_emplace_or_get(&editor->edge_attrs, e->id, &ea))
        {
            case HMAP_OOM: return;
            case HMAP_NEW: auto_position_edge(ea);
            case HMAP_EXISTS: break;
        }

    editor->graph = g;
}
