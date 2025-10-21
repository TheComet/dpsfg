#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <ctype.h>
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
void on_set(struct plugin_ctx* ctx, struct csfg_var_table* parameters)
{
}
void on_changed(struct plugin_ctx* ctx, struct csfg_var_table* parameters)
{
    int                                slot;
    const struct str*                  name;
    const struct csfg_var_table_entry* entry;

    log_dbg("parameters updated:\n");
    hmap_for_each (parameters->map, slot, name, entry)
    {
        log_dbg(
            "  %s: %f\n",
            str_cstr(name),
            csfg_expr_eval(entry->pool, entry->expr, NULL));
    }
}
void on_clear(struct plugin_ctx* ctx)
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
    &parameters};
