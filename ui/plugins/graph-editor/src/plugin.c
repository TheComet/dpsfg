#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include "graph-editor/graph_editor.h"

struct plugin_ctx
{
    GraphEditor*             graph_editor;
    struct plugin_callbacks* callbacks;
};

static GtkWidget* ui_center_create(struct plugin_ctx* ctx)
{
    GraphEditor* graph_editor = graph_editor_new();
    return GTK_WIDGET(g_object_ref_sink(graph_editor));
}

static void ui_center_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    g_object_unref(ui);
}

static struct dpsfg_ui_center_interface ui_center = {
    ui_center_create, ui_center_destroy};

static struct plugin_ctx*
create(const struct plugin_callbacks* cb, GTypeModule* type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));

    graph_editor_register_type_internal(type_module);

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
