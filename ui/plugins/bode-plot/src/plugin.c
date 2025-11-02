#include "bode-plot/bode_plot.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <gtk/gtk.h>

struct plugin_ctx
{
    BodePlot*                             bode_plot;
    const struct plugin_notify_interface* icb;
    struct dpsfg_plugin_callbacks*        cb;
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
    ctx->bode_plot = bode_plot_new();
    return GTK_WIDGET(g_object_ref_sink(ctx->bode_plot));
}
static void ui_pane_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    (void)ctx;
    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_notify_interface* icb,
    struct dpsfg_plugin_callbacks*        cb,
    GTypeModule*                          type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    ctx->icb = icb;
    ctx->cb = cb;

    bode_plot_register_type_internal(type_module);

    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static void on_tf_changed(struct plugin_ctx* ctx, const struct csfg_tf* tf)
{
    bode_plot_set_tf(ctx->bode_plot, tf);
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};

static struct dpsfg_numeric_interface numeric = {on_tf_changed};

static struct dpsfg_plugin_info info = {
    "Bode Plot",
    "visualizer",
    "TheComet",
    "@TheComet93",
    "Plots the Poles and Zeros of a Transfer Function"};

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
    NULL,
    &numeric};
