#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"
#include "graph-editor/attr.h"
#include "graph-editor/clipboard.h"
#include "graph-editor/color.h"
#include "graph-editor/color_picker.h"
#include "graph-editor/constants.h"
#include "graph-editor/draw.h"
#include "graph-editor/drawing.h"
#include "graph-editor/graph_editor.h"
#include "graph-editor/graph_helpers.h"
#include "graph-editor/graph_model.h"
#include "graph-editor/undo.h"
#include <librsvg/rsvg.h>

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

    gulong color_button_signal_handles[COLOR_BUTTONS_COUNT];
    GtkToggleButton* color_buttons[COLOR_BUTTONS_COUNT];
    GtkToggleButton* graph_mode_button;
    GtkToggleButton* draw_mode_button;

    /* It's difficult to get the mouse coordinates in some callbacks, so we use
     * GtkEventControllerMotion to store them here */
    double mouse_x, mouse_y;

    /* View transform */
    double zoom;
    double pan_x, pan_y;
    double pan_offset_x, pan_offset_y;

    /* Drag state */
    int drag_begin_x, drag_begin_y;
    int drag_end_x, drag_end_y;

    struct graph_model* model;

    unsigned show_shortcuts     : 1;
    unsigned show_seven_steps   : 1;
    unsigned fix_physical_units : 1;
    unsigned is_box_selecting   : 1;
};

G_DEFINE_DYNAMIC_TYPE(GraphEditor, graph_editor, GTK_TYPE_BOX)

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
    undo_push_state(model);
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
            undo_push_state(model);
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
            undo_push_state(model);
            break;
        case MODE_DRAW: break;
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
            undo_push_state(model);
            break;
        case MODE_RECONNECT_FROM:
            select_next_connected_node_in_direction(
                model, model->reconnect_node_id, dx, dy);
            select_next_connected_edge_in_direction(
                model, model->reconnect_node_id, dx, dy);
        case MODE_RECONNECT_TO: break;
        case MODE_DRAW        : break;
    }
}

/* -------------------------------------------------------------------------- */
static void
select_matching_color_button_for_node(GraphEditor* editor, int n_id);
static void
select_matching_color_button_for_edge(GraphEditor* editor, int e_id);
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

static void button_fix_physical_units_cb(GtkButton* button, gpointer user_data)
{
    GraphEditor* editor = user_data;
    editor->fix_physical_units =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE;
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    node_direction_action(model, -1, 0);
    select_matching_color_button_for_node(editor, model->active_node_id);
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    node_direction_action(editor->model, 1, 0);
    select_matching_color_button_for_node(editor, model->active_node_id);
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    node_direction_action(editor->model, 0, -1);
    select_matching_color_button_for_node(editor, model->active_node_id);
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    node_direction_action(editor->model, 0, 1);
    select_matching_color_button_for_node(editor, model->active_node_id);
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    edge_direction_action(editor->model, -1, 0);
    select_matching_color_button_for_edge(editor, model->active_edge_id);
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    edge_direction_action(editor->model, 1, 0);
    select_matching_color_button_for_edge(editor, model->active_edge_id);
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    edge_direction_action(editor->model, 0, -1);
    select_matching_color_button_for_edge(editor, model->active_edge_id);
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
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    edge_direction_action(editor->model, 0, 1);
    select_matching_color_button_for_edge(editor, model->active_edge_id);
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
        case MODE_DRAW        : break;
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
    undo_push_state(model);
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
    undo_push_state(model);
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
    model->active_node_id = delete_node(model, model->active_node_id);
    model->active_edge_id = delete_edge(model, model->active_edge_id);
    delete_multi_selection_objects(model);
    model->reconnect_node_id = -1;
    model->reconnect_edge_id = -1;
    model->mode              = MODE_NORMAL;
    notify_graph_changed(model);
    undo_push_state(model);
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
    undo_push_state(model);
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
    undo_push_state(model);
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
    undo_push_state(model);
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

        case MODE_DRAW: break;
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
    undo_push_state(editor->model);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void button_flip_edge_cb(GtkButton* button, gpointer user_data)
{
    (void)button, shortcut_flip_edge_cb(NULL, NULL, user_data);
}

static void button_graph_mode_cb(GtkButton* button, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)button;
    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
        case MODE_RECONNECT_FROM:
        case MODE_RECONNECT_TO  : break;

        case MODE_DRAW: model->mode = MODE_NORMAL; break;
    }
}
static void button_draw_mode_cb(GtkButton* button, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)button;
    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE  : model->mode = MODE_DRAW; break;

        case MODE_RECONNECT_FROM: break;
        case MODE_RECONNECT_TO  :
        case MODE_DRAW          : break;
    }
}
static void
select_matching_color_button(GraphEditor* editor, struct color color)
{
    int i;
    struct color other_color;
    for (i = 0; i != COLOR_BUTTONS_COUNT; ++i)
    {
        other_color = gdk_to_color(color_picker_get_color(
            PLUGIN_COLOR_PICKER(editor->color_buttons[i])));
        if (color.r == other_color.r && color.g == other_color.g &&
            color.b == other_color.b)
        {
            g_signal_handler_block(
                editor->color_buttons[i],
                editor->color_button_signal_handles[i]);
            gtk_toggle_button_set_active(editor->color_buttons[i], TRUE);
            g_signal_handler_unblock(
                editor->color_buttons[i],
                editor->color_button_signal_handles[i]);
            break;
        }
    }
}
static void select_matching_color_button_for_node(GraphEditor* editor, int n_id)
{
    struct node_attr* na = node_attr_hmap_find(editor->model->node_attrs, n_id);
    if (na != NULL)
        select_matching_color_button(editor, na->color);
}
static void select_matching_color_button_for_edge(GraphEditor* editor, int e_id)
{
    struct edge_attr* ea = edge_attr_hmap_find(editor->model->edge_attrs, e_id);
    if (ea != NULL)
        select_matching_color_button(editor, ea->color);
}
static void select_matching_color_button_for_selected_lines(
    GraphEditor* editor, const struct line_vec* drawing)
{
    const struct line* line;
    vec_for_each (drawing, line)
        if (line->selected)
        {
            select_matching_color_button(editor, line->color);
            break;
        }
}
static void select_matching_color_button_for_selected_objects(
    GraphEditor* editor, const struct graph_model* model)
{
    const struct csfg_node* n;
    const struct line* line;

    csfg_graph_for_each_node (model->graph, n)
        if (node_attr_hmap_find(model->node_attrs, n->id)->selected)
        {
            select_matching_color_button_for_node(editor, n->id);
            return;
        }

    vec_for_each (model->drawing, line)
        if (line->selected)
        {
            select_matching_color_button(editor, line->color);
            return;
        }
}
static gboolean shortcut_draw_and_graph_mode_cb(
    GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)widget, (void)unused;
    if (gtk_toggle_button_get_active(editor->draw_mode_button))
    {
        button_graph_mode_cb(NULL, user_data);
        gtk_toggle_button_set_active(editor->graph_mode_button, TRUE);
        select_matching_color_button(editor, model->graph_color);
    }
    else
    {
        button_draw_mode_cb(NULL, user_data);
        gtk_toggle_button_set_active(editor->draw_mode_button, TRUE);
        select_matching_color_button(editor, model->draw_color);
    }
    return TRUE;
}

static gboolean
shortcut_copy_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    GdkClipboard* clipboard =
        gdk_display_get_clipboard(gtk_widget_get_display(GTK_WIDGET(editor)));
    (void)widget, (void)unused;
    copy_selection_to_clipboard(clipboard, editor->model);
    return TRUE;
}
static gboolean
shortcut_paste_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GraphEditor* editor = user_data;
    GdkClipboard* clipboard =
        gdk_display_get_clipboard(gtk_widget_get_display(GTK_WIDGET(editor)));
    (void)widget, (void)unused;
    paste_from_clipboard(clipboard, editor->model);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static void button_color_cb(ColorPicker* picker, gpointer user_data)
{
    GraphEditor* editor    = user_data;
    struct color new_color = gdk_to_color(color_picker_get_color(picker));

    if (gtk_toggle_button_get_active(editor->graph_mode_button))
        editor->model->graph_color = new_color;

    if (gtk_toggle_button_get_active(editor->draw_mode_button))
        editor->model->draw_color = new_color;

    recolor_selected_objects(editor->model, new_color);
    undo_push_state(editor->model);
    gtk_widget_queue_draw(editor->drawing_area);
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
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t* cr,
    int width,
    int height,
    gpointer user_data)
{
    const GraphEditor* editor       = user_data;
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
    draw_edges(
        cr,
        model->graph,
        model->node_attrs,
        model->edge_attrs,
        model->mode,
        model->active_edge_id,
        model->reconnect_edge_id);
    draw_nodes(
        cr,
        model->graph,
        model->node_attrs,
        model->mode,
        model->node_in_id,
        model->node_out_id,
        model->active_node_id,
        model->marked_node_id,
        model->reconnect_node_id);

    draw_drawing(cr, model->drawing, editor->zoom);
    cairo_set_line_width(cr, 2.0 / editor->zoom);

    if (editor->is_box_selecting)
        draw_multi_selection(
            cr,
            editor->drag_begin_x,
            editor->drag_begin_y,
            editor->drag_end_x,
            editor->drag_end_y);
}

/* -------------------------------------------------------------------------- */
static void click_down(
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
            if (n_press == 2 /* double-click */)
                start_editing_active_node_or_edge(editor);
            break;
        case MODE_MOVE: break;
        case MODE_RECONNECT_FROM:
            if (model->reconnect_node_id > -1)
            {
                int e_id = try_select_edge(model->graph, x, y);
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
                    try_select_node(model->graph, model->node_attrs, x, y);
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
            int n_id = try_select_node(model->graph, model->node_attrs, x, y);
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
        case MODE_DRAW: break;
    }

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void click_up(
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
    struct node_attr* na;
    struct edge_attr* ea;
    struct line* line;
    struct point* point;
    GdkEvent* event;
    int id, line_idx;

    int add_to_multiselect      = 0;
    int remove_from_multiselect = 0;

    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)gesture;

    x = (x - editor->pan_x) / editor->zoom;
    y = (y - editor->pan_y) / editor->zoom;

    event =
        gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(gesture));
    if (event)
    {
        GdkModifierType mods = gdk_event_get_modifier_state(event);
        if (mods & GDK_CONTROL_MASK)
            remove_from_multiselect = 1;
        if (mods & GDK_SHIFT_MASK)
            add_to_multiselect = 1;
    }

    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
            editor->drag_begin_x = x;
            editor->drag_begin_y = y;
            editor->drag_end_x   = x;
            editor->drag_end_y   = y;

            /* We select here and not in click_down() because GTK emits both
             * signals. It's better to have the selection logic in one place and
             * not spread over 2 event handlers */
            /* Priorities: Drawing > Edge > Node -- feels better */

            if ((id = try_select_edge(model->graph, x, y)) > -1)
            {
                if (add_to_multiselect && model->active_node_id > -1)
                {
                    node_attr_hmap_find(
                        model->node_attrs, model->active_node_id)
                        ->selected = 1;
                }
                if (!add_to_multiselect && !remove_from_multiselect &&
                    id != model->active_edge_id)
                {
                    multi_deselect_all(model);
                }
                model->active_edge_id = id;
                model->active_node_id = -1;

                select_matching_color_button_for_edge(editor, id);
                csfg_graph_for_each_edge (model->graph, e)
                {
                    ea = edge_attr_hmap_find(model->edge_attrs, e->id);
                    ea->drag_begin_x = e->x;
                    ea->drag_begin_y = e->y;
                }
            }
            else if (
                (id = try_select_node(model->graph, model->node_attrs, x, y)) >
                -1)
            {
                na = node_attr_hmap_find(model->node_attrs, id);
                if (add_to_multiselect)
                    na->selected = 1;
                if (remove_from_multiselect)
                    na->selected = 0;
                if (!add_to_multiselect && !remove_from_multiselect &&
                    id != model->active_node_id && !na->selected)
                {
                    multi_deselect_all(model);
                }
                if (add_to_multiselect && model->active_node_id > -1)
                {
                    node_attr_hmap_find(
                        model->node_attrs, model->active_node_id)
                        ->selected = 1;
                }
                model->active_node_id = id;
                model->active_edge_id = -1;

                select_matching_color_button_for_node(editor, id);
                csfg_graph_for_each_node (model->graph, n)
                {
                    na = node_attr_hmap_find(model->node_attrs, n->id);
                    na->drag_begin_x = n->x;
                    na->drag_begin_y = n->y;
                }
            }
            else if ((line_idx = try_select_line(model->drawing, x, y)) > -1)
            {
                if (add_to_multiselect && model->active_node_id > -1)
                {
                    node_attr_hmap_find(
                        model->node_attrs, model->active_node_id)
                        ->selected = 1;
                }
                vec_get(model->drawing, line_idx)->selected = 1;
                if (remove_from_multiselect)
                    vec_get(model->drawing, line_idx)->selected = 0;
                if (!add_to_multiselect && !remove_from_multiselect &&
                    !vec_get(model->drawing, line_idx)->selected)
                {
                    multi_deselect_all(model);
                }
                model->active_edge_id = -1;
                model->active_node_id = -1;

                select_matching_color_button(
                    editor, vec_get(model->drawing, line_idx)->color);
                vec_for_each (model->drawing, line)
                {
                    line->drag_begin_x = 0;
                    line->drag_begin_y = 0;
                }
            }
            else
            {
                if (!add_to_multiselect && !remove_from_multiselect)
                    multi_deselect_all(model);
                editor->is_box_selecting = 1;
            }

            break;
        case MODE_RECONNECT_FROM: break;
        case MODE_RECONNECT_TO  : break;
        case MODE_DRAW:
            line = line_vec_emplace(&model->drawing);
            if (line == NULL)
                return;
            line_init(line, model->draw_color);
            point = point_vec_emplace(&line->points);
            if (point == NULL)
                return;
            point->x = (int)x;
            point->y = (int)y;
            break;
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
    struct node_attr* na;
    struct edge_attr* ea;
    struct line* line;
    struct point* point;
    double start_x, start_y, x, y;
    GdkEvent* event;
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    int multi_select_append   = 1;

    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    x = (start_x + offset_x - editor->pan_x) / editor->zoom;
    y = (start_y + offset_y - editor->pan_y) / editor->zoom;
    offset_x /= editor->zoom;
    offset_y /= editor->zoom;

    snap_size = GRID;
    event =
        gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(gesture));
    if (event)
    {
        GdkModifierType mods = gdk_event_get_modifier_state(event);
        if (mods & GDK_CONTROL_MASK)
        {
            snap_size *= 6;
            multi_select_append = 0;
        }
    }

    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
            editor->drag_end_x = x;
            editor->drag_end_y = y;
            if (!editor->is_box_selecting)
            {
                /* Drag around nodes */
                csfg_graph_for_each_node (model->graph, n)
                {
                    int n_x, n_y, prev_x, prev_y;
                    na = node_attr_hmap_find(model->node_attrs, n->id);
                    if (!na->selected && model->active_node_id != n->id)
                        continue;

                    prev_x = n->x, prev_y = n->y;
                    n_x = na->drag_begin_x + (int)offset_x;
                    n_y = na->drag_begin_y + (int)offset_y;
                    n_x += n_x >= 0 ? +GRID / 2 : -GRID / 2 + 1;
                    n_y += n_y >= 0 ? +GRID / 2 : -GRID / 2 + 1;
                    n->x = (n_x / snap_size) * snap_size;
                    n->y = (n_y / snap_size) * snap_size;

                    drag_all_edges_connected_to_node(
                        model->graph, n->id, prev_x, prev_y, 0);
                }
#if 0
                /* Try to fix any edges that end up overlapping stuff they
                 * shouldn't as a result of dragging around nodes */
                csfg_graph_for_each_edge (model->graph, e)
                {
                    struct csfg_node* n1 =
                        csfg_graph_get_node(model->graph, e->n_idx_from);
                    struct csfg_node* n2 =
                        csfg_graph_get_node(model->graph, e->n_idx_to);
                    struct node_attr* na1 =
                        node_attr_hmap_find(model->node_attrs, n1->id);
                    struct node_attr* na2 =
                        node_attr_hmap_find(model->node_attrs, n2->id);
                    int n1_selected =
                        na1->selected || model->active_node_id == n1->id;
                    int n2_selected =
                        na2->selected || model->active_node_id == n2->id;
                    if ((n1_selected && !n2_selected) ||
                        (n2_selected && !n1_selected))
                        bump_edge(model->graph, e);
                }
#endif

                /* Drag around edges */
                csfg_graph_for_each_edge (model->graph, e)
                {
                    int e_x, e_y;
                    ea = edge_attr_hmap_find(model->edge_attrs, e->id);
                    if (model->active_edge_id != e->id)
                        continue;

                    e_x = ea->drag_begin_x + (int)offset_x;
                    e_y = ea->drag_begin_y + (int)offset_y;
                    e_x += e_x >= 0 ? +GRID / 2 : -GRID / 2 + 1;
                    e_y += e_y >= 0 ? +GRID / 2 : -GRID / 2 + 1;
                    e->x = (e_x / snap_size) * snap_size;
                    e->y = (e_y / snap_size) * snap_size;
                }

                /* Drag around lines/drawings */
                vec_for_each (model->drawing, line)
                    if (line->selected && !editor->is_box_selecting)
                    {
                        vec_for_each (line->points, point)
                        {
                            point->x -= line->drag_begin_x;
                            point->y -= line->drag_begin_y;
                            point->x += (int)offset_x;
                            point->y += (int)offset_y;
                        }
                        line->drag_begin_x = (int)offset_x;
                        line->drag_begin_y = (int)offset_y;
                    }
            }

            /* Update selection of objects if box selecting is active */
            if (editor->is_box_selecting)
            {
                multi_select_nodes_and_lines(
                    editor->model,
                    editor->drag_begin_x,
                    editor->drag_begin_y,
                    editor->drag_end_x,
                    editor->drag_end_y,
                    multi_select_append);
            }
            break;
        case MODE_RECONNECT_FROM: break;
        case MODE_RECONNECT_TO  : break;
        case MODE_DRAW:
            if (vec_count(model->drawing) == 0)
                return;
            line  = vec_last(model->drawing);
            point = point_vec_emplace(&line->points);
            if (point == NULL)
                return;
            point->x = (int)x;
            point->y = (int)y;
            break;
    }

    gtk_widget_queue_draw(editor->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void
drag_end(GtkGestureDrag* gesture, double x, double y, gpointer user_data)
{
    int dx, dy, drag_was_significant;
    GraphEditor* editor       = user_data;
    struct graph_model* model = editor->model;
    (void)gesture, (void)x, (void)y;

    dx                   = editor->drag_begin_x - editor->drag_end_x;
    dy                   = editor->drag_begin_y - editor->drag_end_y;
    drag_was_significant = dx * dx + dy * dy > GRID * GRID / 4;

    switch (model->mode)
    {
        case MODE_NORMAL:
        case MODE_MOVE:
            if (editor->is_box_selecting)
                select_matching_color_button_for_selected_objects(
                    editor, model);

            if (!editor->is_box_selecting && drag_was_significant)
                undo_push_state(model);

            editor->is_box_selecting = 0;
            gtk_widget_queue_draw(editor->drawing_area);
            break;
        case MODE_RECONNECT_FROM: break;
        case MODE_RECONNECT_TO  : break;
        case MODE_DRAW          : undo_push_state(model); break;
    }
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
static void update_zoom(GraphEditor* editor, double new_zoom)
{
    double gx, gy;

    new_zoom = new_zoom > 4.0 ? 4.0 : new_zoom;
    new_zoom = new_zoom < 1.0 / 4 ? 1.0 / 4 : new_zoom;

    /* Convert mouse coordinates to graph space */
    gx = (editor->mouse_x - editor->pan_x) / editor->zoom;
    gy = (editor->mouse_y - editor->pan_y) / editor->zoom;

    /* Adjust pan so we zoom around the mouse coordinates and not 0,0 */
    editor->pan_x = editor->mouse_x - gx * new_zoom;
    editor->pan_y = editor->mouse_y - gy * new_zoom;
    editor->zoom  = new_zoom;
}
static gboolean mouse_scroll_cb(
    GtkEventControllerScroll* controller,
    double dx,
    double dy,
    gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)controller, (void)dx;
    update_zoom(editor, dy > 0 ? editor->zoom * 0.9 : editor->zoom * 1.1);
    gtk_widget_queue_draw(editor->drawing_area);
    return TRUE;
}
static void
pinch_zoom_cb(GtkGestureZoom* controller, gdouble scale, gpointer user_data)
{
    GraphEditor* editor = user_data;
    (void)controller;
    update_zoom(editor, editor->zoom * scale);
    gtk_widget_queue_draw(editor->drawing_area);
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
        {GDK_KEY_d, GDK_NO_MODIFIER_MASK, shortcut_draw_and_graph_mode_cb},
        {GDK_KEY_c, GDK_CONTROL_MASK,     shortcut_copy_cb},
        {GDK_KEY_v, GDK_CONTROL_MASK,     shortcut_paste_cb},
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
        {"Show Shortcut List",          NULL,           "help-shortcuts.svg",     button_show_shortcuts_cb,     1, 0, 0},
        {"Show the '7 Steps'",          NULL,           "help-7-steps.svg",       button_show_7steps_cb,        1, 0, 0},
        {"Try to fix physical units",   NULL,           "physical-units.svg",     button_fix_physical_units_cb, 1, 0, 1},
        {NULL,                          NULL,           NULL,                     NULL,                         0, 0, 0},
        {"Delete",                      "x",            "delete.svg",             button_delete_cb,             0, 0, 0},
        {"Undo",                        "ctrl+z",       "rotate-ccw.svg",         button_undo_cb,               0, 0, 0},
        {"Redo",                        "ctrl+shift+z", "rotate-cw.svg",          button_redo_cb,               0, 0, 0},
        {NULL,                          NULL,           NULL,                     NULL,                         0, 0, 0},
        {"New Node",                    "n",            "new-node.svg",           button_new_node_cb,           0, 0, 0},
        {"New Connected Node",          "shift+n",      "new-node-with-edge.svg", button_new_node_and_edge_cb,  0, 0, 0},
        {"New Edge",                    "e",            "new-edge.svg",           button_new_edge_cb,           0, 0, 0},
        {"Mark Node (for connections)", "s",            "mark-node.svg",          button_mark_node_cb,          0, 0, 0},
        {"Set Input Node",              "i",            "input-node.svg",         button_set_in_node_cb,        0, 0, 0},
        {"Set Output Node",             "o",            "output-node.svg",        button_set_out_node_cb,       0, 0, 0},
        {"Reconnect Edge",              "r",            "reconnect-edge.svg",     button_reconnect_edge_cb,     0, 0, 0},
        {"Flip Edge",                   "f",            "flip-edge.svg",          button_flip_edge_cb,          0, 0, 0},
        {NULL,                          NULL,           NULL,                     NULL,                         0, 0, 0},
        {"Graph Mode",                  "d (again)",    "mouse.svg",              button_graph_mode_cb,         1, 0, 1},
        {"Draw Mode",                   "d",            "draw.svg",               button_draw_mode_cb,          1, 1, 0},
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

        if (buttons[i].callback == button_graph_mode_cb)
            editor->graph_mode_button = GTK_TOGGLE_BUTTON(button);
        if (buttons[i].callback == button_draw_mode_cb)
            editor->draw_mode_button = GTK_TOGGLE_BUTTON(button);

        str_set_cstr(&str, "/ch/thecomet/dpsfg/graph-editor/icons/");
        str_append_cstr(&str, buttons[i].icon);
        image = gtk_image_new_from_resource(str_cstr(str));
        gtk_button_set_child(GTK_BUTTON(button), image);

        str_set_cstr(&str, buttons[i].tooltip);
        if (buttons[i].shortcut != NULL)
        {
            str_append_cstr(&str, "\nShortcut: ");
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
    gtk_widget_set_tooltip_text(
        GTK_WIDGET(editor->color_buttons[0]), "Shortcut: 1");
    gtk_widget_set_margin_start(GTK_WIDGET(editor->color_buttons[0]), 8);
    gtk_box_append(toolbar, GTK_WIDGET(editor->color_buttons[0]));
    editor->color_button_signal_handles[0] = g_signal_connect(
        editor->color_buttons[0],
        "color-selected",
        G_CALLBACK(button_color_cb),
        editor);

    for (i = 1; i != COLOR_BUTTONS_COUNT; ++i)
    {
        char tooltip[] = "Shortcut: 1";
        tooltip[10]    = '1' + i;

        editor->color_buttons[i] =
            GTK_TOGGLE_BUTTON(color_picker_new(hex_gdk(default_colors[i])));
        gtk_toggle_button_set_group(
            editor->color_buttons[i], editor->color_buttons[0]);
        gtk_widget_set_tooltip_text(
            GTK_WIDGET(editor->color_buttons[i]), tooltip);
        gtk_box_append(toolbar, GTK_WIDGET(editor->color_buttons[i]));

        editor->color_button_signal_handles[i] = g_signal_connect(
            editor->color_buttons[i],
            "color-selected",
            G_CALLBACK(button_color_cb),
            editor);
    }
}

/* -------------------------------------------------------------------------- */
static GtkWidget* create_toolbar(GraphEditor* editor)
{
    GtkBox* toolbar = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    add_toolbar_buttons(editor, toolbar);
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

    self->zoom         = 1.0;
    self->pan_x        = 500.0;
    self->pan_y        = 500.0;
    self->drag_begin_x = 0;
    self->drag_begin_y = 0;

    self->mouse_x = 0.0;
    self->mouse_y = 0.0;

    self->show_shortcuts     = 0;
    self->show_seven_steps   = 0;
    self->fix_physical_units = 0;
    self->is_box_selecting   = 0;

    self->seven_steps =
        load_svg("/ch/thecomet/dpsfg/graph-editor/images/7-steps.svg");

    self->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area, TRUE);
    gtk_widget_set_vexpand(self->drawing_area, TRUE);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area), draw_cb, self, NULL);

    GtkGesture* click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(click_down), self);
    g_signal_connect(click, "released", G_CALLBACK(click_up), self);
    gtk_widget_add_controller(self->drawing_area, GTK_EVENT_CONTROLLER(click));

    GtkGesture* drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), GDK_BUTTON_PRIMARY);
    g_signal_connect(drag, "drag-begin", G_CALLBACK(drag_begin), self);
    g_signal_connect(drag, "drag-update", G_CALLBACK(drag_update), self);
    g_signal_connect(drag, "drag-end", G_CALLBACK(drag_end), self);
    gtk_widget_add_controller(self->drawing_area, GTK_EVENT_CONTROLLER(drag));

    GtkGesture* pan = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(
        GTK_GESTURE_SINGLE(pan), GDK_BUTTON_SECONDARY);
    g_signal_connect(pan, "drag-begin", G_CALLBACK(pan_begin), self);
    g_signal_connect(pan, "drag-update", G_CALLBACK(pan_update), self);
    gtk_widget_add_controller(self->drawing_area, GTK_EVENT_CONTROLLER(pan));

    GtkEventController* mouse_scroll =
        gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(mouse_scroll, "scroll", G_CALLBACK(mouse_scroll_cb), self);
    gtk_widget_add_controller(self->drawing_area, mouse_scroll);

    GtkGesture* pinch_zoom = gtk_gesture_zoom_new();
    g_signal_connect(
        pinch_zoom, "scale-changed", G_CALLBACK(pinch_zoom_cb), self);
    gtk_widget_add_controller(
        self->drawing_area, GTK_EVENT_CONTROLLER(pinch_zoom));

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
