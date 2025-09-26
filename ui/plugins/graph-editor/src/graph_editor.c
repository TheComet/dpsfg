#include "csfg/util/log.h"
#include "graph-editor/graph_editor.h"

typedef struct
{
    double x, y, r;
    char*  label;
} GraphNode;

typedef struct
{
    GraphNode *src, *dst;
    double     control_x, control_y;
} GraphEdge;

struct _GraphEditor
{
    GtkBox parent_instance;

    /* drawing area is a child of "overlay". We use overlay to create GtkEntry's
     * on top of the drawing area so the user can enter formulas and give names
     * to nodes */
    GtkWidget* overlay;
    GtkWidget* drawing_area;
    GtkWidget* entry; /* Text entry, when active */

    // View transform
    double zoom;
    double pan_x, pan_y;
    double pan_offset_x, pan_offset_y;

    // Drag state
    GraphNode* active_node;
    double     drag_offset_x, drag_offset_y;

    /* It's difficult to get the mouse coordinates in some callbacks, so we use
     * GtkEventControllerMotion to store them here */
    double mouse_x, mouse_y;

    GraphNode node1;
    GraphNode node2;
    GraphEdge edge;
};

G_DEFINE_DYNAMIC_TYPE(GraphEditor, graph_editor, GTK_TYPE_BOX)

static gboolean point_in_node(GraphNode* n, double x, double y)
{
    double dx = x - n->x;
    double dy = y - n->y;
    return dx * dx + dy * dy <= n->r * n->r;
}

static void draw_grid(
    cairo_t* cr, double pan_x, double pan_y, int width, int height, double zoom)
{
    static const int grid_width = 20;

    pan_x /= zoom;
    pan_y /= zoom;
    width /= zoom;
    height /= zoom;

    int from_x = -(int)pan_x / grid_width * grid_width - grid_width;
    int from_y = -(int)pan_y / grid_width * grid_width - grid_width;
    int to_x = from_x + width + grid_width;
    int to_y = from_y + height + grid_width;

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

static void draw_node(cairo_t* cr, GraphNode* n, double r, double g, double b)
{
    cairo_set_source_rgb(cr, r, g, b);
    cairo_arc(cr, n->x, n->y, n->r, 0, 2 * M_PI);
    cairo_fill(cr);

    // Draw label with Pango
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, n->label ? n->label : "", -1);
    PangoFontDescription* desc = pango_font_description_from_string("Sans 12");
    pango_layout_set_font_description(layout, desc);

    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    double tx = n->x - tw / 2.0;
    double ty = n->y + th;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, tx, ty);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    pango_font_description_free(desc);
}

static void
draw_edge(cairo_t* cr, GraphEdge* e, double r, double g, double b, double zoom)
{
    // Extract the three points
    double x1 = e->src->x, y1 = e->src->y;
    double x2 = e->dst->x, y2 = e->dst->y;
    double x3 = e->control_x, y3 = e->control_y;

    // Check for collinearity (area of triangle = 0)
    double area = (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (fabs(area) < 1e-5)
    {
        // Nearly collinear — draw straight line
        cairo_set_source_rgb(cr, r, g, b);
        cairo_set_line_width(cr, 2.0 / zoom);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
        return;
    }

    // Compute circumcenter (cx, cy)
    double A = x1 - x2;
    double B = y1 - y2;
    double C = x1 - x3;
    double D = y1 - y3;

    double E = ((x1 * x1 - x2 * x2) + (y1 * y1 - y2 * y2)) / 2.0;
    double F = ((x1 * x1 - x3 * x3) + (y1 * y1 - y3 * y3)) / 2.0;

    double denom = A * D - B * C;
    if (fabs(denom) < 1e-5)
    {
        cairo_set_source_rgb(cr, r, g, b);
        cairo_set_line_width(cr, 2.0 / zoom);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
        return;
    }

    double cx = (D * E - B * F) / denom;
    double cy = (-C * E + A * F) / denom;

    // Compute radius
    double radius = hypot(cx - x1, cy - y1);
    if (radius > 1000)
    {
        cairo_set_source_rgb(cr, r, g, b);
        cairo_set_line_width(cr, 2.0 / zoom);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
        return;
    }

    // Compute angles from center to src, dst, control
    double angle1 = atan2(y1 - cy, x1 - cx);
    double angle2 = atan2(y2 - cy, x2 - cx);
    double angle_control = atan2(y3 - cy, x3 - cx);

    // Convert angles into unit vectors
    double vx1 = cos(angle1), vy1 = sin(angle1);
    double vx2 = cos(angle2), vy2 = sin(angle2);
    double vx3 = cos(angle_control), vy3 = sin(angle_control);

    // 2D cross product to determine orientation
    double cross = (vx1 * vy2 - vy1 * vx2);
    double cross_control = (vx1 * vy3 - vy1 * vx3);

    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 2.0 / zoom);

    // Determine arc direction so it passes through control point
    if ((cross > 0 && cross_control > 0) || (cross < 0 && cross_control < 0))
        cairo_arc(cr, cx, cy, radius, angle1, angle2); // CCW
    else
        cairo_arc_negative(cr, cx, cy, radius, angle1, angle2); // CW

    cairo_stroke(cr);
}

static void draw_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    GraphEditor* editor = user_pointer;
    cairo_translate(cr, editor->pan_x, editor->pan_y);
    cairo_scale(cr, editor->zoom, editor->zoom);

    draw_grid(cr, editor->pan_x, editor->pan_y, width, height, editor->zoom);

    draw_edge(cr, &editor->edge, 0.2, 0.2, 0.2, editor->zoom);
    draw_node(cr, &editor->node1, 0.2, 0.2, 0.2);
    draw_node(cr, &editor->node2, 0.2, 0.2, 0.2);
}

static void finish_editing(GtkEntry* e, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;
    const char*  text = gtk_editable_get_text(GTK_EDITABLE(e));
    if (editor->active_node)
    {
        g_free(editor->active_node->label);
        editor->active_node->label = g_strdup(text);
    }
    gtk_widget_unparent(editor->entry);
    editor->entry = NULL;
    gtk_widget_queue_draw(GTK_WIDGET(user_pointer));
}

static void
on_entry_focus_changed(GObject* obj, GParamSpec* pspec, gpointer user_data)
{
    GtkWidget* entry = GTK_WIDGET(obj);
    if (!gtk_widget_has_focus(entry))
        finish_editing(GTK_ENTRY(entry), user_data);
}

static void start_editing(GraphEditor* editor, GraphNode* n)
{
    if (editor->entry)
        return; // Already editing

    editor->entry = gtk_entry_new();
    gtk_editable_set_text(
        GTK_EDITABLE(editor->entry), n->label ? n->label : "");

    gtk_widget_set_hexpand(editor->entry, FALSE);
    gtk_widget_set_vexpand(editor->entry, FALSE);
    gtk_widget_set_halign(editor->entry, GTK_ALIGN_START);
    gtk_widget_set_valign(editor->entry, GTK_ALIGN_START);

    int ex = (int)(n->x * editor->zoom + editor->pan_x);
    int ey = (int)(n->y * editor->zoom + editor->pan_y);
    gtk_widget_set_margin_start(editor->entry, ex);
    gtk_widget_set_margin_top(editor->entry, ey);
    gtk_widget_set_size_request(editor->entry, n->r, n->r);

    gtk_overlay_add_overlay(GTK_OVERLAY(editor->overlay), editor->entry);
    gtk_widget_set_visible(editor->entry, TRUE);
    gtk_widget_grab_focus(editor->entry);

    g_signal_connect(
        editor->entry, "activate", G_CALLBACK(finish_editing), editor);

    editor->active_node = n;
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

    if (point_in_node(&editor->node1, x, y))
        editor->active_node = &editor->node1;
    else if (point_in_node(&editor->node2, x, y))
        editor->active_node = &editor->node2;
    else
        editor->active_node = NULL;

    if (editor->active_node && n_press == 2 /* double-click */)
    {
        start_editing(editor, editor->active_node);
        return;
    }
}

static void click_end(
    GtkGestureClick* gesture,
    int              n_press,
    double           x,
    double           y,
    gpointer         user_pointer)
{
    GraphEditor* editor = user_pointer;
    if (n_press == 1)
        editor->active_node = NULL;
}

static void drag_update(
    GtkGestureDrag* gesture,
    double          offset_x,
    double          offset_y,
    gpointer        user_pointer)
{
    double       start_x, start_y, x, y;
    GraphEditor* editor = user_pointer;

    if (editor->active_node == NULL)
        return;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - editor->pan_x) / editor->zoom;
    y = (start_y + offset_y - editor->pan_y) / editor->zoom;

    editor->active_node->x = round((x - editor->drag_offset_x) / 20) * 20;
    editor->active_node->y = round((y - editor->drag_offset_y) / 20) * 20;

    gtk_widget_queue_draw(GTK_WIDGET(editor->drawing_area));
}

static void
drag_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;

    if (point_in_node(&editor->node1, x, y))
        editor->active_node = &editor->node1;
    else if (point_in_node(&editor->node2, x, y))
        editor->active_node = &editor->node2;
    else
        editor->active_node = NULL;

    if (editor->active_node)
    {
        editor->drag_offset_x = round((x - editor->active_node->x) / 20) * 20;
        editor->drag_offset_y = round((y - editor->active_node->y) / 20) * 20;
    }
}

static void
drag_end(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;
    editor->active_node = NULL;
}

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

static void
pan_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    GraphEditor* editor = user_pointer;
    editor->pan_offset_x = editor->pan_x;
    editor->pan_offset_y = editor->pan_y;
}

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

static void graph_editor_init(GraphEditor* self)
{
    log_dbg("graph_editor_init()\n");
    self->node1.x = 0;
    self->node1.y = 0;
    self->node1.r = 10;
    self->node1.label = g_strdup("Node A");

    self->node2.x = 100;
    self->node2.y = 0;
    self->node2.r = 10;
    self->node2.label = g_strdup("Node B");

    self->edge.src = &self->node1;
    self->edge.dst = &self->node2;
    self->edge.control_x = 50;
    self->edge.control_y = 10;

    self->zoom = 1.0;
    self->pan_x = 500.0;
    self->pan_y = 500.0;
    self->active_node = NULL;
    self->drag_offset_x = 0.0;
    self->drag_offset_y = 0.0;

    self->overlay = NULL;
    self->entry = NULL;
    self->drawing_area = NULL;

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

static void graph_editor_finalize(GObject* obj)
{
    log_dbg("graph_editor_finalize()\n");
    GraphEditor* self = PLUGIN_GRAPH_EDITOR(obj);
    g_free(self->node1.label);
    g_free(self->node2.label);
}

static void graph_editor_class_init(GraphEditorClass* class)
{
    log_dbg("graph_editor_class_init()\n");
    GObjectClass*   object_class = G_OBJECT_CLASS(class);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(class);

    object_class->finalize = graph_editor_finalize;
}

static void graph_editor_class_finalize(GraphEditorClass* class)
{
    log_dbg("graph_editor_class_finalize()\n");
}

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

GraphEditor* graph_editor_new(void)
{
    return g_object_new(PLUGIN_TYPE_GRAPH_EDITOR, NULL);
}
