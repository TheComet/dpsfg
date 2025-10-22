#include "csfg/numeric/cpoly.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <gtk/gtk.h>

struct plugin_ctx
{
    GtkWidget*                               tweakables_view;
    const struct plugin_callbacks_interface* icb;
    struct plugin_callbacks*                 cb;
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
    ctx->tweakables_view = gtk_text_view_new();
    return GTK_WIDGET(g_object_ref_sink(ctx->tweakables_view));
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
    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
void on_polynomial_changed(
    struct plugin_ctx*       ctx,
    const struct csfg_cpoly* numerator,
    const struct csfg_cpoly* denominator,
    const struct csfg_rpoly* zeros,
    const struct csfg_rpoly* poles)
{
    int                        i;
    const struct csfg_complex* c;

    log_dbg("Zeros: ");
    vec_enumerate (zeros, i, c)
    {
        if (i)
            log_raw(", ");
        log_raw("%.2f + %.2fi", c->real, c->imag);
    }
    log_raw("\n");

    log_dbg("Poles: ");
    vec_enumerate (poles, i, c)
    {
        if (i)
            log_raw(", ");
        log_raw("%.2f + %.2fi", c->real, c->imag);
    }
    log_raw("\n");

    (void)ctx, (void)numerator, (void)denominator;
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};

static struct dpsfg_numeric_interface numeric = {on_polynomial_changed};

static struct plugin_info info = {
    "Pole-Zero Plot",
    "visualizer",
    "TheComet",
    "@TheComet93",
    "Plots the Poles and Zeros of a Transfer Function"};

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
    NULL,
    &numeric};
