#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <gtk/gtk.h>

struct tweak
{
    struct plugin_ctx*     plugin_ctx;
    struct csfg_var_table* vt;
    struct str*            name;

    GtkWidget*     label;
    GtkAdjustment* scale_adj;
    GtkWidget*     scale;
    GtkAdjustment* spin_adj;
    GtkWidget*     spin_button;
};

HMAP_DECLARE_STR(extern, tweak_hmap, struct tweak, 16)
HMAP_DEFINE_STR(extern, tweak_hmap, struct tweak, 16)

static void notify_parameters_changed(struct plugin_ctx* ctx);
static void on_scale_adj_changed(GtkAdjustment* adj, gpointer user_data);
static void on_spin_adj_changed(GtkAdjustment* adj, gpointer user_data);

/* -------------------------------------------------------------------------- */
static void remove_row_containing(GtkGrid* grid, GtkWidget* target)
{
    int row;
    int left, top, width, height;
    gtk_grid_query_child(grid, target, &left, &top, &width, &height);

    for (row = top + height - 1; row >= top; row--)
        gtk_grid_remove_row(grid, row);
}

/* -------------------------------------------------------------------------- */
static void clear_grid(GtkWidget* grid)
{
    GtkWidget* child = gtk_widget_get_first_child(grid);
    while (child != NULL)
    {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_grid_remove(GTK_GRID(grid), child);
        child = next;
    }
}

/* -------------------------------------------------------------------------- */
static void on_scale_adj_changed(GtkAdjustment* adj, gpointer user_data)
{
    struct tweak* tweak = user_data;

    double log_val = gtk_adjustment_get_value(adj);
    double value = pow(10.0, log_val);
    double rounded = floor(log_val);
    double step = pow(10.0, rounded) / 10.0;
    int    digits = rounded >= 1.0 ? 0 : -(int)rounded + 1.0;

    g_signal_handlers_block_by_func(
        tweak->spin_adj, G_CALLBACK(on_spin_adj_changed), tweak);
    {
        gtk_adjustment_configure(
            tweak->spin_adj, value, -DBL_MAX, DBL_MAX, step, 0.0, 0.0);
        gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tweak->spin_button), digits);
    }
    g_signal_handlers_unblock_by_func(
        tweak->spin_adj, G_CALLBACK(on_spin_adj_changed), tweak);

    csfg_var_table_set_lit(tweak->vt, str_view(tweak->name), value);
    notify_parameters_changed(tweak->plugin_ctx);
}

/* -------------------------------------------------------------------------- */
static void on_spin_adj_changed(GtkAdjustment* adj, gpointer user_data)
{
    struct tweak* tweak = user_data;

    double value = gtk_adjustment_get_value(adj);
    double log_min = log(value) / log(10) - 1.0;
    double log_max = log(value) / log(10) + 1.0;
    double log_val = log(value) / log(10);
    double rounded = floor(log_val);
    double step = pow(10.0, rounded) / 10.0;
    int    digits = rounded < 0.0 ? 0 : (int)rounded;

    g_signal_handlers_block_by_func(
        tweak->scale_adj, G_CALLBACK(on_scale_adj_changed), tweak);
    gtk_adjustment_configure(
        tweak->scale_adj, log_val, log_min, log_max, 0.0, 0.0, 0.0);
    g_signal_handlers_unblock_by_func(
        tweak->scale_adj, G_CALLBACK(on_scale_adj_changed), tweak);

    gtk_adjustment_set_step_increment(adj, step);
    gtk_scale_set_digits(GTK_SCALE(tweak->scale), digits);

    csfg_var_table_set_lit(tweak->vt, str_view(tweak->name), value);
    notify_parameters_changed(tweak->plugin_ctx);
}

/* -------------------------------------------------------------------------- */
static void maybe_expand_bounds(struct tweak* s, double L)
{
    double lower = gtk_adjustment_get_lower(s->scale_adj);
    double upper = gtk_adjustment_get_upper(s->scale_adj);
    double span = upper - lower;

#if 0
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
#endif
}

struct plugin_ctx
{
    GtkWidget*                            grid;
    const struct plugin_notify_interface* icb;
    struct dpsfg_plugin_callbacks*        cb;
    struct csfg_var_table*                parameters;
    struct tweak_hmap*                    tweaks;
};

/* -------------------------------------------------------------------------- */
static void
update_tweak(struct tweak* tweak, const struct csfg_var_table_entry* entry)
{

    double value = csfg_expr_eval(entry->pool, entry->expr, NULL);
    double step = value / 10.0;
    double log_min = log(value / 10.0);
    double log_max = log(value * 10.0);
    double log_val = log(value);

    gtk_adjustment_configure(
        tweak->scale_adj, log_val, log_min, log_max, 0.0, 0.0, 0.0);
    gtk_adjustment_configure(
        tweak->spin_adj, value, -DBL_MAX, DBL_MAX, step, 0.0, 0.0);
}

/* -------------------------------------------------------------------------- */
static void rebuild_ui(struct plugin_ctx* ctx)
{
    int                                slot, row;
    const struct str*                  name;
    const struct csfg_var_table_entry* entry;
    struct tweak*                      tweak;

    row = 0;
    hmap_for_each (ctx->tweaks, slot, name, tweak)
    {
        if (ctx->parameters != NULL &&
            csfg_var_hmap_find(ctx->parameters->map, str_view(name)) != NULL)
        {
            row++;
            continue;
        }

        remove_row_containing(GTK_GRID(ctx->grid), tweak->label);
        str_deinit(tweak->name);
        tweak_hmap_erase_slot(ctx->tweaks, slot);
    }

    if (ctx->parameters == NULL)
        return;

    hmap_for_each (ctx->parameters->map, slot, name, entry)
    {
        switch (tweak_hmap_emplace_or_get(&ctx->tweaks, str_view(name), &tweak))
        {
            case HMAP_OOM: return;
            case HMAP_NEW: break;
            case HMAP_EXISTS: update_tweak(tweak, entry); continue;
        }

        tweak->plugin_ctx = ctx;
        tweak->vt = ctx->parameters;
        str_init(&tweak->name);
        str_set_str(&tweak->name, name);

        tweak->label = gtk_label_new(str_cstr(name));
        gtk_widget_set_halign(tweak->label, GTK_ALIGN_END);
        gtk_label_set_xalign(GTK_LABEL(tweak->label), 1.0);

        tweak->scale_adj = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        tweak->scale =
            gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, tweak->scale_adj);
        gtk_widget_set_hexpand(tweak->scale, TRUE);
        // gtk_scale_set_draw_value(GTK_SCALE(tweak->scale), FALSE);

        tweak->spin_adj = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        tweak->spin_button = gtk_spin_button_new(tweak->spin_adj, 0.0, 1);

        gtk_grid_attach(GTK_GRID(ctx->grid), tweak->label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(ctx->grid), tweak->scale, 1, row, 1, 1);
        gtk_grid_attach(GTK_GRID(ctx->grid), tweak->spin_button, 2, row, 1, 1);

        update_tweak(tweak, entry);

        g_signal_connect(
            tweak->scale_adj,
            "value-changed",
            G_CALLBACK(on_scale_adj_changed),
            tweak);
        g_signal_connect(
            tweak->spin_adj,
            "value-changed",
            G_CALLBACK(on_spin_adj_changed),
            tweak);

        row++;
    }
}

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
    int               slot;
    const struct str* name;
    struct tweak*     tweak;

    hmap_for_each (ctx->tweaks, slot, name, tweak)
        (void)slot, (void)name, str_deinit(tweak->name);
    tweak_hmap_clear(ctx->tweaks);

    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_notify_interface* icb,
    struct dpsfg_plugin_callbacks*        cb,
    GTypeModule*                          type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    (void)type_module;
    ctx->icb = icb;
    ctx->cb = cb;
    ctx->parameters = NULL;
    tweak_hmap_init(&ctx->tweaks);
    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    int               slot;
    const struct str* name;
    struct tweak*     tweak;
    (void)type_module;

    hmap_for_each (ctx->tweaks, slot, name, tweak)
        (void)slot, (void)name, str_deinit(tweak->name);
    tweak_hmap_deinit(ctx->tweaks);

    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static void notify_parameters_changed(struct plugin_ctx* ctx)
{
    if (ctx->parameters != NULL)
        ctx->icb->parameters_changed(ctx->cb, ctx);
}

/* -------------------------------------------------------------------------- */
static void
on_parameters_set(struct plugin_ctx* ctx, struct csfg_var_table* parameters)
{
    ctx->parameters = parameters;
    rebuild_ui(ctx);
}
static void on_parameters_clear(struct plugin_ctx* ctx)
{
    ctx->parameters = NULL;
    rebuild_ui(ctx);
}
static void on_parameters_changed(struct plugin_ctx* ctx)
{
    rebuild_ui(ctx);
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};

static struct dpsfg_parameters_interface parameters = {
    on_parameters_set, on_parameters_clear, on_parameters_changed};

static struct dpsfg_plugin_info info = {
    "Tweakables",
    "editor",
    "TheComet",
    "@TheComet93",
    "Tweak the Values of Variables"};

PLUGIN_API struct dpsfg_plugin_interface dpsfg_plugin = {
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
