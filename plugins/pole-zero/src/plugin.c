#include "csfg/util/mem.h"
#include "dpsfg-plugin.h"
#include "pole-zero/pole_zero_plot.h"

struct plugin_ctx
{
#if defined(PLUGIN_MICROUI)
    int todo;
#else
    PoleZeroPlot* pole_zero_plot;
#endif
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
#if defined(PLUGIN_MICROUI)
    (void)ctx;
    return NULL;
#else
    ctx->pole_zero_plot = pole_zero_plot_new();
    return GTK_WIDGET(g_object_ref_sink(ctx->pole_zero_plot));
#endif
}
static void ui_pane_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
#if defined(PLUGIN_MICROUI)
    (void)ctx, (void)ui;
#else
    (void)ctx;
    g_object_unref(ui);
#endif
}
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};

/* -------------------------------------------------------------------------- */
static void on_tf_changed(struct plugin_ctx* ctx, const struct csfg_tf* tf)
{
#if defined(PLUGIN_MICROUI)
#else
    pole_zero_plot_set_tf(ctx->pole_zero_plot, tf);
#endif
}
static void on_impulse_response_changed(
    struct plugin_ctx* ctx, const struct csfg_pfd_poly* pfd_terms)
{
    (void)ctx, (void)pfd_terms;
}
static void on_step_response_changed(
    struct plugin_ctx* ctx, const struct csfg_pfd_poly* pfd_terms)
{
    (void)ctx, (void)pfd_terms;
}
static void on_ramp_response_changed(
    struct plugin_ctx* ctx, const struct csfg_pfd_poly* pfd_terms)
{
    (void)ctx, (void)pfd_terms;
}
static struct dpsfg_numeric_interface numeric = {
    on_tf_changed,
    on_impulse_response_changed,
    on_step_response_changed,
    on_ramp_response_changed};

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_notify_interface* notify_interface,
    struct plugin_notify_context* notify_ctx,
    GTypeModule* type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    (void)notify_interface, (void)notify_ctx;

#if defined(PLUGIN_MICROUI)
    (void)type_module;
#else
    pole_zero_plot_register_type_internal(type_module);
#endif

    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    mem_free(ctx);
}

static struct dpsfg_plugin_info info = {
    "Pole-Zero Plot",
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
    &numeric,
    NULL};
