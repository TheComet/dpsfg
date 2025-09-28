#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <gtk/gtk.h>
#include <stdio.h>

struct _GraphEditor
{
    GtkBox parent_instance;

    GtkWidget* drawing_area;
};

struct _GraphEditorClass
{
    GtkBoxClass parent_class;
};

#define PLUGIN_TYPE_GRAPH_EDITOR (graph_editor_get_type())
G_DECLARE_FINAL_TYPE(GraphEditor, graph_editor, Plugin, GraphEditor, GtkBox);
G_DEFINE_DYNAMIC_TYPE(GraphEditor, graph_editor, GTK_TYPE_BOX)

static void graph_editor_class_init(GraphEditorClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    GtkBoxClass*  box_class = GTK_BOX_CLASS(class);

    object_class->finalize = graph_editor_finalize;
}

static void graph_editor_class_finalize(GraphEditorClass* class)
{
}

void graph_editor_register_type_internal(GTypeModule* type_module)
{
    graph_editor_register_type(type_module);

    /*
     * Have to re-initialize static variables explicitly, because GTK doesn't
     * support types registered by modules to be unloaded, but we do it anyway.
     */
    gpointer class = g_type_class_peek(PLUGIN_TYPE_GRAPH_EDITOR);
    if (class)
        graph_editor_parent_class = g_type_class_peek_parent(class);
}

GtkWidget* graph_editor_new(void)
{
    GraphEditor* editor = g_object_new(PLUGIN_TYPE_GRAPH_EDITOR, NULL);
    return GTK_WIDGET(editor);
}

typedef struct
{
    double x, y, r;
    char*  label;
} GraphNode;

typedef struct
{
    GraphNode *src, *dst;
} GraphEdge;

struct plugin_ctx
{
    GraphNode node1;
    GraphNode node2;
    GraphEdge edge;

    // View transform
    double zoom;
    double pan_x, pan_y;

    // Drag state
    GraphNode* active_node;
    double     drag_offset_x, drag_offset_y;

    // UI references
    GtkWidget* overlay; // GtkOverlay containing drawing + entry
    GtkWidget* entry;   // Active text entry

    GtkWidget* drawing_area;
};

// --- Global model for demo ---

static gboolean point_in_node(GraphNode* n, double x, double y)
{
    double dx = x - n->x;
    double dy = y - n->y;
    return dx * dx + dy * dy <= n->r * n->r;
}

static void draw_grid(cairo_t* cr, double zoom)
{
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    for (int x = -1000; x < 2000; x += 20)
    {
        cairo_move_to(cr, x, -1000);
        cairo_line_to(cr, x, 2000);
    }
    for (int y = -1000; y < 2000; y += 20)
    {
        cairo_move_to(cr, -1000, y);
        cairo_line_to(cr, 2000, y);
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
    double cx = (e->src->x + e->dst->x) / 2;
    double cy = (e->src->y + e->dst->y) / 2;
    double dx = e->src->x - e->dst->x;
    double dy = e->src->y - e->dst->y;
    double a = atan2(dy, dx);
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 2.0 / zoom);
    cairo_arc(cr, cx, cy, sqrt(dx * dx + dy * dy) / 2, a, a + M_PI);
    cairo_stroke(cr);
}

static void draw_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;
    cairo_translate(cr, ctx->pan_x, ctx->pan_y);
    cairo_scale(cr, ctx->zoom, ctx->zoom);

    draw_grid(cr, ctx->zoom);

    draw_edge(cr, &ctx->edge, 0.2, 0.2, 0.2, ctx->zoom);
    draw_node(cr, &ctx->node1, 0.2, 0.2, 0.2);
    draw_node(cr, &ctx->node2, 0.2, 0.2, 0.2);
}

static void finish_editing(GtkEntry* e, gpointer user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;
    const char*        text = gtk_editable_get_text(GTK_EDITABLE(e));
    if (ctx->active_node)
    {
        g_free(ctx->active_node->label);
        ctx->active_node->label = g_strdup(text);
    }
    gtk_widget_unparent(ctx->entry);
    ctx->entry = NULL;
    gtk_widget_queue_draw(GTK_WIDGET(user_pointer));
}

static void
on_entry_focus_changed(GObject* obj, GParamSpec* pspec, gpointer user_data)
{
    GtkWidget* entry = GTK_WIDGET(obj);
    if (!gtk_widget_has_focus(entry))
        finish_editing(GTK_ENTRY(entry), user_data);
}

static void start_editing(GraphNode* n, struct plugin_ctx* ctx)
{
    if (ctx->entry)
        return; // Already editing

    ctx->entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(ctx->entry), n->label ? n->label : "");

    gtk_widget_set_hexpand(ctx->entry, FALSE);
    gtk_widget_set_vexpand(ctx->entry, FALSE);
    gtk_widget_set_halign(ctx->entry, GTK_ALIGN_START);
    gtk_widget_set_valign(ctx->entry, GTK_ALIGN_START);

    int ex = (int)(n->x * ctx->zoom + ctx->pan_x);
    int ey = (int)(n->y * ctx->zoom + ctx->pan_y);
    gtk_widget_set_margin_start(ctx->entry, ex);
    gtk_widget_set_margin_top(ctx->entry, ey);
    gtk_widget_set_size_request(ctx->entry, n->r, n->r);

    gtk_overlay_add_overlay(GTK_OVERLAY(ctx->overlay), ctx->entry);
    gtk_widget_set_visible(ctx->entry, TRUE);
    gtk_widget_grab_focus(ctx->entry);

    g_signal_connect(ctx->entry, "activate", G_CALLBACK(finish_editing), ctx);

    ctx->active_node = n;
}

static void click_begin(
    GtkGestureClick* gesture,
    int              n_press,
    double           x,
    double           y,
    gpointer         user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;

    x = (x - ctx->pan_x) / ctx->zoom;
    y = (y - ctx->pan_y) / ctx->zoom;

    if (point_in_node(&ctx->node1, x, y))
        ctx->active_node = &ctx->node1;
    else if (point_in_node(&ctx->node2, x, y))
        ctx->active_node = &ctx->node2;
    else
        ctx->active_node = NULL;

    if (ctx->active_node && n_press == 2 /* double-click */)
    {
        start_editing(ctx->active_node, ctx);
        return;
    }

    if (ctx->active_node)
    {
        ctx->drag_offset_x = x - ctx->active_node->x;
        ctx->drag_offset_y = y - ctx->active_node->y;
    }
}

static void click_end(
    GtkGestureClick* gesture,
    int              n_press,
    double           x,
    double           y,
    gpointer         user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;
    if (n_press == 1)
        ctx->active_node = NULL;
}

static void drag_update(
    GtkGestureDrag* gesture,
    double          offset_x,
    double          offset_y,
    gpointer        user_pointer)
{
    double             start_x, start_y, x, y;
    struct plugin_ctx* ctx = user_pointer;

    if (ctx->active_node == NULL)
        return;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - ctx->pan_x) / ctx->zoom;
    y = (start_y + offset_y - ctx->pan_y) / ctx->zoom;

    ctx->active_node->x = floor((x - ctx->drag_offset_x) / 20 + 0.5) * 20;
    ctx->active_node->y = floor((y - ctx->drag_offset_y) / 20 + 0.5) * 20;

    gtk_widget_queue_draw(GTK_WIDGET(ctx->drawing_area));
}

static void
drag_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;
    if (!ctx->active_node)
    {
        ctx->drag_offset_y = ctx->pan_y;
        ctx->drag_offset_x = ctx->pan_x;
    }
}

static void
drag_end(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;
    ctx->active_node = NULL;
}

static void pan_update(
    GtkGestureDrag* gesture,
    double          offset_x,
    double          offset_y,
    gpointer        user_pointer)
{
    double             start_x, start_y;
    struct plugin_ctx* ctx = user_pointer;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    ctx->pan_x = ctx->drag_offset_x + offset_x;
    ctx->pan_y = ctx->drag_offset_y + offset_y;
    gtk_widget_queue_draw(GTK_WIDGET(ctx->drawing_area));
}

static void
pan_begin(GtkGestureDrag* gesture, double x, double y, gpointer user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;
    ctx->drag_offset_x = ctx->pan_x;
    ctx->drag_offset_y = ctx->pan_y;
}

static gboolean scroll_cb(
    GtkEventControllerScroll* controller,
    double                    dx,
    double                    dy,
    gpointer                  user_pointer)
{
    struct plugin_ctx* ctx = user_pointer;
    if (dy > 0)
        ctx->zoom *= 0.9;
    else if (dy < 0)
        ctx->zoom *= 1.1;

    gtk_widget_queue_draw(GTK_WIDGET(ctx->drawing_area));
    return TRUE;
}

static GtkWidget* graph_editor_new(struct plugin_ctx* ctx)
{
    ctx->overlay = gtk_overlay_new();

    ctx->drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(ctx->drawing_area), draw_cb, ctx, NULL);
    gtk_overlay_set_child(GTK_OVERLAY(ctx->overlay), ctx->drawing_area);

    GtkGesture* click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(click_begin), ctx);
    g_signal_connect(click, "released", G_CALLBACK(click_end), ctx);
    gtk_widget_add_controller(ctx->drawing_area, GTK_EVENT_CONTROLLER(click));

    GtkGesture* drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), GDK_BUTTON_PRIMARY);
    g_signal_connect(drag, "drag-begin", G_CALLBACK(drag_begin), ctx);
    g_signal_connect(drag, "drag-update", G_CALLBACK(drag_update), ctx);
    g_signal_connect(drag, "drag-end", G_CALLBACK(drag_end), ctx);
    gtk_widget_add_controller(ctx->drawing_area, GTK_EVENT_CONTROLLER(drag));

    GtkGesture* pan = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(pan), GDK_BUTTON_MIDDLE);
    g_signal_connect(pan, "drag-begin", G_CALLBACK(pan_begin), ctx);
    g_signal_connect(pan, "drag-update", G_CALLBACK(pan_update), ctx);
    gtk_widget_add_controller(ctx->drawing_area, GTK_EVENT_CONTROLLER(pan));

    GtkEventController* zoom =
        gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(zoom, "scroll", G_CALLBACK(scroll_cb), ctx);
    gtk_widget_add_controller(ctx->drawing_area, zoom);

    return ctx->overlay;
}

static GtkWidget* ui_center_create(struct plugin_ctx* ctx)
{
    GtkWidget* graph_editor = graph_editor_new(ctx);
    return g_object_ref_sink(graph_editor);
}

static void ui_center_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    g_object_unref(ui);
}

static struct dpsfg_ui_center_interface ui_center = {
    ui_center_create, ui_center_destroy};

static struct plugin_ctx* create(GTypeModule* type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));

    ctx->node1.x = 0;
    ctx->node1.y = 0;
    ctx->node1.r = 10;
    ctx->node1.label = g_strdup("Node A");

    ctx->node2.x = 100;
    ctx->node2.y = 0;
    ctx->node2.r = 10;
    ctx->node2.label = g_strdup("Node B");

    ctx->edge.src = &ctx->node1;
    ctx->edge.dst = &ctx->node2;

    ctx->zoom = 1.0;
    ctx->pan_x = 500.0;
    ctx->pan_y = 500.0;
    ctx->active_node = NULL;
    ctx->drag_offset_x = 0.0;
    ctx->drag_offset_y = 0.0;

    ctx->overlay = NULL;
    ctx->entry = NULL;
    ctx->drawing_area = NULL;

    return ctx;
}

static void destroy(GTypeModule* type_module, struct plugin_ctx* ctx)
{
    mem_free(ctx);
}

static struct plugin_info info = {
    "Graph Editor",
    "editor",
    "TheComet",
    "@TheComet93",
    "Signal Flow Graph Editor"};

PLUGIN_API struct plugin_interface dpsfg_plugin = {
    PLUGIN_VERSION, 0, &info, create, destroy, &ui_center};
