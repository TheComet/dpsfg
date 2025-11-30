#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include "graph-editor/graph_editor.h"

struct plugin_ctx
{
    GraphEditor*                          graph_editor;
    GtkWidget*                            pole_zero_plot;
    const struct plugin_notify_interface* icb;
    struct dpsfg_plugin_callbacks*        cb;
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_center_create(struct plugin_ctx* ctx)
{
    ctx->graph_editor = graph_editor_new(ctx, ctx->icb, ctx->cb);
    return GTK_WIDGET(g_object_ref_sink(ctx->graph_editor));
}
static void ui_center_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    (void)ctx;
    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
static void graph_on_set(
    struct plugin_ctx* ctx, struct csfg_graph* g, int node_in, int node_out)
{
    graph_editor_set_graph(ctx->graph_editor, g, node_in, node_out);
}
static void graph_on_clear(struct plugin_ctx* ctx)
{
    graph_editor_clear_graph(ctx->graph_editor);
}
static void
graph_on_structure_changed(struct plugin_ctx* ctx, int node_in, int node_out)
{
    graph_editor_rebuild_graph(ctx->graph_editor, node_in, node_out);
}
static void graph_on_layout_changed(struct plugin_ctx* ctx)
{
    graph_editor_redraw_graph(ctx->graph_editor);
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

    graph_editor_register_type_internal(type_module);

    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_center_interface ui_center = {
    ui_center_create, ui_center_destroy};
static struct dpsfg_graph_interface graph = {
    graph_on_set,
    graph_on_clear,
    graph_on_structure_changed,
    graph_on_layout_changed};

static struct dpsfg_plugin_info info = {
    "Graph Editor",
    "editor",
    "TheComet",
    "@TheComet93",
    "Signal Flow Graph Editor"};

PLUGIN_API struct dpsfg_plugin_interface dpsfg_plugin = {
    PLUGIN_VERSION,
    0,
    &info,
    create,
    destroy,
    &ui_center,
    NULL,
    &graph,
    NULL,
    NULL,
    NULL,
    NULL};
