#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <gtk/gtk.h>

typedef struct
{
    GtkAdjustment* adj;           /* holds L = ln(value) */
    double         start_L;       /* L at drag-begin */
    double         start_x;       /* pointer x at drag-begin */
    double         px_to_log;     /* sensitivity: pixels -> change in L */
    double         expand_margin; /* margin (in log units) at which to expand */
    GtkWidget*     value_label;   /* label that shows exp(L) */

    struct plugin_ctx* plugin_ctx;
} InfScaleState;

/* Format and update the displayed real value */
static void update_value_label(InfScaleState* s)
{
    double L = gtk_adjustment_get_value(s->adj);
    double v = exp(L);
    char   buf[64];
    /* Choose formatting you like */
    if (v < 0.001 || v > 1000.0)
        g_snprintf(buf, sizeof(buf), "%.3e", v);
    else
        g_snprintf(buf, sizeof(buf), "%.6g", v);
    gtk_label_set_text(GTK_LABEL(s->value_label), buf);
}

/* Called when adjustment changed (also when we programmatically change it) */
static void on_adj_value_changed(GtkAdjustment* adj, gpointer user_data)
{
    InfScaleState* s = user_data;
    update_value_label(s);
}

/* When the drag starts, record baseline */
static void on_drag_begin(
    GtkGestureDrag* gesture,
    gdouble         start_x,
    gdouble         start_y,
    gpointer        user_data)
{
    InfScaleState* s = user_data;
    log_dbg("drag begin\n");
    s->start_L = gtk_adjustment_get_value(s->adj);
    s->start_x = start_x;
}

/* Expand the adjustment bounds if L moves outside them.
   The expansion strategy: take current span = upper - lower.
   If L < lower, set lower = L - span, upper = lower + 2*span.
   If L > upper, set upper = L + span, lower = upper - 2*span.
   That doubles the visible span each time you pass an edge (exponential
   growth).
*/
static void maybe_expand_bounds(InfScaleState* s, double L)
{
    double lower = gtk_adjustment_get_lower(s->adj);
    double upper = gtk_adjustment_get_upper(s->adj);
    double span = upper - lower;

    if (L < lower + s->expand_margin)
    {
        /* expand to the left */
        double new_lower = L - span;
        double new_upper = new_lower + 2.0 * span;
        gtk_adjustment_set_lower(s->adj, new_lower);
        gtk_adjustment_set_upper(s->adj, new_upper);
    }
    else if (L > upper - s->expand_margin)
    {
        /* expand to the right */
        double new_upper = L + span;
        double new_lower = new_upper - 2.0 * span;
        gtk_adjustment_set_lower(s->adj, new_lower);
        gtk_adjustment_set_upper(s->adj, new_upper);
    }
}

/* On drag update, compute new L from pixel delta and set adjustment */
static void on_drag_update(
    GtkGestureDrag* gesture, gdouble x, gdouble y, gpointer user_data)
{
    InfScaleState* s = user_data;
    double         dx = x - s->start_x;
    double         newL = s->start_L + dx * s->px_to_log;

    log_dbg("drag update\n");

    /* Optionally: clamp to some absolute limits to avoid NaNs/infinite ranges
     */
    const double ABS_MAX_L = 100.0; /* corresponds to e^100 huge */
    if (newL < -ABS_MAX_L)
        newL = -ABS_MAX_L;
    if (newL > ABS_MAX_L)
        newL = ABS_MAX_L;

    /* Expand bounds if needed so the user can keep dragging past edges */
    maybe_expand_bounds(s, newL);

    /* Apply the new log-value */
    g_signal_handlers_block_by_func(
        s->adj, G_CALLBACK(on_adj_value_changed), s);
    gtk_adjustment_set_value(s->adj, newL);
    g_signal_handlers_unblock_by_func(
        s->adj, G_CALLBACK(on_adj_value_changed), s);

    /* Update label ourselves */
    update_value_label(s);
}

struct plugin_ctx
{
    GtkWidget*                               grid;
    const struct plugin_callbacks_interface* icb;
    struct plugin_callbacks*                 cb;
    struct csfg_var_table*                   parameters;
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
    ctx->grid = gtk_grid_new();
    // gtk_grid_set_row_spacing(GTK_GRID(ctx->grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(ctx->grid), 16);
    gtk_widget_set_margin_top(ctx->grid, 12);
    gtk_widget_set_margin_bottom(ctx->grid, 12);
    gtk_widget_set_margin_start(ctx->grid, 12);
    gtk_widget_set_margin_end(ctx->grid, 12);

    return GTK_WIDGET(g_object_ref_sink(ctx->grid));
}
static void ui_pane_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    (void)ctx;
    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_callbacks_interface* icb,
    struct plugin_callbacks*                 cb,
    GTypeModule*                             type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    (void)type_module;
    ctx->icb = icb;
    ctx->cb = cb;
    ctx->parameters = NULL;
    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static void notify_parameters_changed(struct plugin_ctx* ctx)
{
    if (ctx->parameters != NULL)
        ctx->icb->parameters_changed(ctx->cb, ctx);
}
static void on_set(struct plugin_ctx* ctx, struct csfg_var_table* parameters)
{
    ctx->parameters = parameters;
}
static void
on_changed(struct plugin_ctx* ctx, struct csfg_var_table* parameters)
{
    int                                slot, row;
    const struct str*                  name;
    const struct csfg_var_table_entry* entry;
    GtkWidget*                         iter;

    iter = gtk_widget_get_first_child(ctx->grid);
    while (iter != NULL)
    {
        GtkWidget* next = gtk_widget_get_next_sibling(iter);
        gtk_grid_remove(GTK_GRID(ctx->grid), iter);
        iter = next;
    }

    row = 0;
    hmap_for_each (parameters->map, slot, name, entry)
    {
        GtkWidget*     label;
        GtkWidget*     scale;
        GtkAdjustment* adj;
        double         value = csfg_expr_eval(entry->pool, entry->expr, NULL);

        label = gtk_label_new(str_cstr(name));
        gtk_widget_set_halign(label, GTK_ALIGN_END);
        gtk_label_set_xalign(GTK_LABEL(label), 1.0);
        gtk_grid_attach(GTK_GRID(ctx->grid), label, 0, row, 1, 1);

        /*
        scale = gtk_scale_new_with_range(
            GTK_ORIENTATION_HORIZONTAL, 0.0, 10.0, 0.1);
        gtk_widget_set_hexpand(scale, TRUE);
        gtk_widget_set_halign(scale, GTK_ALIGN_FILL);
        gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
        gtk_grid_attach(GTK_GRID(ctx->grid), scale, 1, row, 1, 1);*/

        /* Setup initial log-range for [0.5, 2.0] */
        double L_low = log(0.5);  /* ~= -0.693147 */
        double L_high = log(2.0); /* ~=  0.693147 */
        double L_center = 0.0;    /* ln(1.0) */

        adj = gtk_adjustment_new(L_center, L_low, L_high, 0.0, 0.0, 0.0);
        scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
        gtk_widget_set_hexpand(scale, TRUE);
        /* we show real value in a label */
        gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);

        gtk_grid_attach(GTK_GRID(ctx->grid), scale, 1, row * 2 + 0, 1, 1);

        /* Display of the actual multiplicative value */
        GtkWidget* value_label = gtk_label_new(NULL);
        gtk_widget_set_halign(value_label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(ctx->grid), value_label, 1, row * 2 + 1, 1, 1);

        /* Set up the InfScaleState */
        InfScaleState* state = g_new0(InfScaleState, 1);
        state->adj = adj;
        /* sensitivity: 1 pixel -> 0.01 in log units (tweak for feel) */
        state->px_to_log = 0.01;
        /* start expanding when within this many log-units from edge */
        state->expand_margin = 0.5;
        state->value_label = value_label;
        state->plugin_ctx = ctx;

        /* Connect adj changed to update label (covers programmatic changes) */
        g_signal_connect(
            adj, "value-changed", G_CALLBACK(on_adj_value_changed), state);
        update_value_label(state);

        /* Add drag gesture to the scale; interpret horizontal motion as
         * multiplicative change */
        GtkGesture* drag = gtk_gesture_drag_new();
        /* allow mouse too */
        gtk_event_controller_set_propagation_phase(
            GTK_EVENT_CONTROLLER(drag), GTK_PHASE_CAPTURE);
        gtk_gesture_single_set_touch_only(GTK_GESTURE_SINGLE(drag), FALSE);
        gtk_widget_add_controller(scale, GTK_EVENT_CONTROLLER(drag));

        g_signal_connect(drag, "drag-begin", G_CALLBACK(on_drag_begin), state);
        g_signal_connect(
            drag, "drag-update", G_CALLBACK(on_drag_update), state);

        row++;
    }
}
static void on_clear(struct plugin_ctx* ctx)
{
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};

static struct dpsfg_parameters_interface parameters = {
    on_set, on_changed, on_clear};

static struct plugin_info info = {
    "Tweakables",
    "editor",
    "TheComet",
    "@TheComet93",
    "Tweak the Values of Variables"};

PLUGIN_API struct plugin_interface dpsfg_plugin = {
    PLUGIN_VERSION,
    0,
    &info,
    create,
    destroy,
    NULL,
    &ui_pane,
    NULL,
    NULL,
    NULL,
    &parameters,
    NULL};
