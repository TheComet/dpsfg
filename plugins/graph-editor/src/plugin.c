#include "csfg/io/deserialize.h"
#include "csfg/io/serialize.h"
#include "csfg/util/mem.h"
#include "dpsfg-plugin.h"
#include "graph-editor/color_picker.h"
#include "graph-editor/graph_editor.h"

struct plugin_ctx
{
    struct graph_model graph_model;
    GraphEditor* graph_editor;
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_center_create(struct plugin_ctx* ctx)
{
    ctx->graph_editor = graph_editor_new(&ctx->graph_model);
    return GTK_WIDGET(g_object_ref_sink(ctx->graph_editor));
}
static void ui_center_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    (void)ctx;
    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
static void graph_on_load(
    struct plugin_ctx* ctx, struct csfg_graph* g, int node_in, int node_out)
{
    graph_model_set_graph(&ctx->graph_model, g, node_in, node_out);
    graph_editor_redraw_graph(ctx->graph_editor);
}
static void graph_on_unload(struct plugin_ctx* ctx)
{
    graph_model_clear_graph(&ctx->graph_model);
    graph_model_clear_drawings(&ctx->graph_model);
    graph_editor_redraw_graph(ctx->graph_editor);
}
static void
graph_on_structure_changed(struct plugin_ctx* ctx, int node_in, int node_out)
{
    graph_model_clear_attrs(&ctx->graph_model);
    graph_model_rebuild_graph(&ctx->graph_model, node_in, node_out);
    graph_editor_redraw_graph(ctx->graph_editor);
}
static void graph_on_layout_changed(struct plugin_ctx* ctx)
{
    graph_editor_redraw_graph(ctx->graph_editor);
}

/* -------------------------------------------------------------------------- */
static int io_on_save(struct plugin_ctx* ctx, struct serializer** ser)
{
    serialize_lu16(ser, 0x0000); /* version */
    if (graph_model_save_attrs(&ctx->graph_model, ser) != 0)
        return -1;
    if (graph_model_save_drawings(&ctx->graph_model, ser) != 0)
        return -1;
    return 0;
}
static int io_on_load(struct plugin_ctx* ctx, struct deserializer* des)
{
    uint16_t version = deserialize_lu16(des);
    if (deserializer_err(des))
        return 0;

    switch (version)
    {
        case 0x000:
            graph_model_load_attrs(&ctx->graph_model, des);
            graph_model_load_drawings(&ctx->graph_model, des);
            break;
    }

    graph_model_reinit_undo_stack(&ctx->graph_model);

    return 0;
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_notify_interface* notify_interface,
    struct plugin_notify_context* notify_ctx,
    GTypeModule* type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));

    graph_model_init(&ctx->graph_model, ctx, notify_interface, notify_ctx);

    graph_editor_register_type_internal(type_module);
    color_picker_register_type_internal(type_module);

    GResource* graph_editor_get_resource(void);
    g_resources_register(graph_editor_get_resource());

    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    graph_model_deinit(&ctx->graph_model);
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_center_interface ui_center = {
    ui_center_create, ui_center_destroy};
static struct dpsfg_graph_interface graph = {
    graph_on_load,
    graph_on_unload,
    graph_on_structure_changed,
    graph_on_layout_changed};
static struct dpsfg_io_interface io = {io_on_save, io_on_load};

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
    NULL,
    &io};
