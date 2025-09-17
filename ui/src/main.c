#include "csfg/util/log.h"
#include "csfg/util/tracker.h"
#include "ui/args.h"
#include <gtk/gtk.h>

typedef struct
{
    double x, y, r;
    char*  label;
} GraphNode;

typedef struct
{
    GraphNode *src, *dst;
} GraphEdge;

// --- Global model for demo ---
static GraphNode node1 = {0, 0, 10, NULL};
static GraphNode node2 = {100, 0, 10, NULL};
static GraphEdge edge = {&node1, &node2};

// View transform
static double zoom = 1.0;
static double pan_x = 500.0, pan_y = 500.0;

// Drag state
static GraphNode* active_node = NULL;
static double     drag_offset_x, drag_offset_y;

// UI references
static GtkWidget* overlay;      // GtkOverlay containing drawing + entry
static GtkWidget* entry = NULL; // Active text entry

static gboolean point_in_node(GraphNode* n, double x, double y)
{
    double dx = x - n->x;
    double dy = y - n->y;
    return dx * dx + dy * dy <= n->r * n->r;
}

static void draw_grid(cairo_t* cr)
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

static void draw_edge(cairo_t* cr, GraphEdge* e, double r, double g, double b)
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

static void
draw_cb(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer data)
{
    cairo_translate(cr, pan_x, pan_y);
    cairo_scale(cr, zoom, zoom);

    draw_grid(cr);

    draw_edge(cr, &edge, 0.2, 0.2, 0.2);
    draw_node(cr, &node1, 0.2, 0.2, 0.2);
    draw_node(cr, &node2, 0.2, 0.2, 0.2);
}

static void finish_editing(GtkEntry* e, gpointer data)
{
    const char* text = gtk_editable_get_text(GTK_EDITABLE(e));
    if (active_node)
    {
        g_free(active_node->label);
        active_node->label = g_strdup(text);
    }
    gtk_widget_unparent(entry);
    entry = NULL;
    gtk_widget_queue_draw(GTK_WIDGET(data));
}

static void
on_entry_focus_changed(GObject* obj, GParamSpec* pspec, gpointer user_data)
{
    GtkWidget* entry = GTK_WIDGET(obj);
    if (!gtk_widget_has_focus(entry))
        finish_editing(GTK_ENTRY(entry), user_data);
}

static void start_editing(GraphNode* n, GtkWidget* area)
{
    if (entry)
        return; // Already editing

    entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(entry), n->label ? n->label : "");

    gtk_widget_set_hexpand(entry, FALSE);
    gtk_widget_set_vexpand(entry, FALSE);
    gtk_widget_set_halign(entry, GTK_ALIGN_START);
    gtk_widget_set_valign(entry, GTK_ALIGN_START);

    int ex = (int)(n->x * zoom + pan_x);
    int ey = (int)(n->y * zoom + pan_y);
    gtk_widget_set_margin_start(entry, ex);
    gtk_widget_set_margin_top(entry, ey);
    gtk_widget_set_size_request(entry, n->r, n->r);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), entry);
    gtk_widget_set_visible(entry, TRUE);
    gtk_widget_grab_focus(entry);

    g_signal_connect(entry, "activate", G_CALLBACK(finish_editing), area);

    active_node = n;
}

static void click_begin(
    GtkGestureClick* gesture, int n_press, double x, double y, gpointer area)
{
    x = (x - pan_x) / zoom;
    y = (y - pan_y) / zoom;

    if (point_in_node(&node1, x, y))
        active_node = &node1;
    else if (point_in_node(&node2, x, y))
        active_node = &node2;
    else
        active_node = NULL;

    if (active_node && n_press == 2 /* double-click */)
    {
        start_editing(active_node, GTK_WIDGET(area));
        return;
    }

    if (active_node)
    {
        drag_offset_x = x - active_node->x;
        drag_offset_y = y - active_node->y;
    }
}

static void click_end(
    GtkGestureClick* gesture, int n_press, double x, double y, gpointer area)
{
    if (n_press == 1)
        active_node = NULL;
}

static void drag_update(
    GtkGestureDrag* gesture, double offset_x, double offset_y, gpointer area)
{
    double start_x, start_y, x, y;
    if (active_node == NULL)
        return;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - pan_x) / zoom;
    y = (start_y + offset_y - pan_y) / zoom;

    active_node->x = floor((x - drag_offset_x) / 20 + 0.5) * 20;
    active_node->y = floor((y - drag_offset_y) / 20 + 0.5) * 20;

    gtk_widget_queue_draw(GTK_WIDGET(area));
}

static void
drag_begin(GtkGestureDrag* gesture, double x, double y, gpointer area)
{
    if (!active_node)
    {
        drag_offset_x = pan_x;
        drag_offset_y = pan_y;
    }
}

static void drag_end(GtkGestureDrag* gesture, double x, double y, gpointer area)
{
    active_node = NULL;
}

static void pan_update(
    GtkGestureDrag* gesture, double offset_x, double offset_y, gpointer area)
{
    double start_x, start_y;
    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    pan_x = drag_offset_x + offset_x;
    pan_y = drag_offset_y + offset_y;
    gtk_widget_queue_draw(GTK_WIDGET(area));
}

static void
pan_begin(GtkGestureDrag* gesture, double x, double y, gpointer area)
{
    drag_offset_x = pan_x;
    drag_offset_y = pan_y;
}

static gboolean scroll_cb(
    GtkEventControllerScroll* controller, double dx, double dy, gpointer area)
{
    if (dy > 0)
        zoom *= 0.9;
    else if (dy < 0)
        zoom *= 1.1;

    gtk_widget_queue_draw(GTK_WIDGET(area));
    return TRUE;
}

static gboolean
shortcut_activated(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    log_dbg("activated shift+r\n");
    return TRUE;
}

static void page_removed(
    GtkNotebook* self, GtkWidget* child, guint page_num, gpointer user_data)
{
    log_dbg("page_removed()\n");
}

static GtkWidget* property_panel_new(void)
{
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "page-removed", G_CALLBACK(page_removed), NULL);
    return notebook;
}

static GtkWidget* plugin_view_new(void)
{
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "page-removed", G_CALLBACK(page_removed), NULL);
    return notebook;
}

static GtkWidget* graph_editor_new(void)
{
    overlay = gtk_overlay_new();

    GtkWidget* area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_cb, NULL, NULL);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), area);

    GtkGesture* click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(click_begin), area);
    g_signal_connect(click, "released", G_CALLBACK(click_end), area);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(click));

    GtkGesture* drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), GDK_BUTTON_PRIMARY);
    g_signal_connect(drag, "drag-begin", G_CALLBACK(drag_begin), area);
    g_signal_connect(drag, "drag-update", G_CALLBACK(drag_update), area);
    g_signal_connect(drag, "drag-end", G_CALLBACK(drag_end), area);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(drag));

    GtkGesture* pan = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(pan), GDK_BUTTON_MIDDLE);
    g_signal_connect(pan, "drag-begin", G_CALLBACK(pan_begin), area);
    g_signal_connect(pan, "drag-update", G_CALLBACK(pan_update), area);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(pan));

    GtkEventController* zoom =
        gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(zoom, "scroll", G_CALLBACK(scroll_cb), area);
    gtk_widget_add_controller(area, zoom);

    return overlay;
}

static void setup_global_shortcuts(GtkWidget* window)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction*  action;
    GtkShortcut*        shortcut;

    controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(
        GTK_SHORTCUT_CONTROLLER(controller), GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(window, controller);

    trigger = gtk_keyval_trigger_new(GDK_KEY_r, GDK_SHIFT_MASK);
    action = gtk_callback_action_new(shortcut_activated, NULL, NULL);
    shortcut = gtk_shortcut_new(trigger, action);
    gtk_shortcut_controller_add_shortcut(
        GTK_SHORTCUT_CONTROLLER(controller), shortcut);
}

static void activate(GtkApplication* app, gpointer user_data)
{
    GtkWidget* window;
    GtkWidget* project_browser;
    GtkWidget* paned1;
    GtkWidget* paned2;
    GtkWidget* plugin_view;
    GtkWidget* property_panel;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "DPSFG");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    setup_global_shortcuts(window);

    // plugin_view = plugin_view_new();
    plugin_view = graph_editor_new();
    property_panel = property_panel_new();
    project_browser = gtk_notebook_new();

    paned2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(paned2), plugin_view);
    gtk_paned_set_end_child(GTK_PANED(paned2), property_panel);
    gtk_paned_set_resize_start_child(GTK_PANED(paned2), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned2), FALSE);
    // gtk_paned_set_position(GTK_PANED(paned2), 800);

    paned1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(paned1), project_browser);
    gtk_paned_set_end_child(GTK_PANED(paned1), paned2);
    gtk_paned_set_resize_start_child(GTK_PANED(paned1), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned1), TRUE);
    gtk_paned_set_position(GTK_PANED(paned1), 600);

    gtk_window_set_child(GTK_WINDOW(window), paned1);
    gtk_window_maximize(GTK_WINDOW(window));
    gtk_widget_set_visible(window, 1);
}

int main(int argc, char** argv)
{
    struct args     args;
    GtkApplication* app;
    int             status = EXIT_FAILURE;

    if (trackers_init_tls() != 0)
        goto mem_init_failed;
    log_init();

    switch (args_parse(&args, argc, argv))
    {
        case 0: break;
        case 1: return 0;
        default: goto parse_args_break;
    }

    if (args.tests)
    {
        int run_tests(int*, char*[]);
        status = run_tests(&argc, argv);
        goto parse_args_break;
    }

    node1.label = g_strdup("Node A");
    node2.label = g_strdup("Node B");

    app = gtk_application_new(
        "ch.thecomet.dpsfg-ui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

parse_args_break:
    trackers_deinit_tls();
mem_init_failed:
    return status;
}
