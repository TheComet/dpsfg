#include "csfg/io/deserialize.h"
#include "csfg/io/serialize.h"
#include "csfg/util/mem.h"
#include "csfg/util/str.h"
#include "dpsfg-plugin.h"
#include "graph-editor/graph_editor.h"

struct plugin_ctx
{
    const struct plugin_notify_interface* notify_interface;
    struct plugin_notify_context* notify_ctx;
    struct edge_attr_hmap* edge_attrs;

#if !defined(PLUGIN_MICROUI)
    GraphEditor* graph_editor;
    GtkWidget* pole_zero_plot;
#endif
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_center_create(struct plugin_ctx* ctx)
{
#if defined(PLUGIN_MICROUI)
    (void)ctx;
    return NULL;
#else
    ctx->graph_editor =
        graph_editor_new(ctx, ctx->notify_interface, ctx->notify_ctx);
    return GTK_WIDGET(g_object_ref_sink(ctx->graph_editor));
#endif
}
static void ui_center_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
#if defined(PLUGIN_MICROUI)
    (void)ctx, (void)ui;
#else
    (void)ctx;
    g_object_unref(ui);
#endif
}

/* -------------------------------------------------------------------------- */
static void graph_on_load(
    struct plugin_ctx* ctx, struct csfg_graph* g, int node_in, int node_out)
{
#if defined(PLUGIN_MICROUI)
#else
    graph_editor_set_edge_attributes(ctx->graph_editor, ctx->edge_attrs);
    graph_editor_set_graph(ctx->graph_editor, g, node_in, node_out);
#endif
    ctx->edge_attrs = NULL;
}
static void graph_on_unload(struct plugin_ctx* ctx)
{
#if defined(PLUGIN_MICROUI)
#else
    ctx->edge_attrs = graph_editor_take_edge_attributes(ctx->graph_editor);
    graph_editor_clear_graph(ctx->graph_editor);
#endif
}
static void
graph_on_structure_changed(struct plugin_ctx* ctx, int node_in, int node_out)
{
#if defined(PLUGIN_MICROUI)
#else
    graph_editor_clear_attrs(ctx->graph_editor);
    graph_editor_rebuild_graph(ctx->graph_editor, node_in, node_out);
#endif
}
static void graph_on_layout_changed(struct plugin_ctx* ctx)
{
#if defined(PLUGIN_MICROUI)
#else
    graph_editor_redraw_graph(ctx->graph_editor);
#endif
}

/* -------------------------------------------------------------------------- */
static int io_on_save(struct plugin_ctx* ctx, struct serializer** ser)
{
    struct edge_attr* ea;
    int i, id, err = 0;

    if (ctx->edge_attrs == NULL)
        return 0;

    err += serialize_lu16(ser, 0x0000); /* version */

    err += serialize_li16(ser, hmap_count(ctx->edge_attrs));
    hmap_for_each (ctx->edge_attrs, i, id, ea)
    {
        err += serialize_li32(ser, id);
        err += serialize_cstr(ser, str_cstr(ea->expr_str));
    }

    return err;
}
static int io_on_load(struct plugin_ctx* ctx, struct deserializer* des)
{
    int i, id, count, edge_id;
    const char* edge_expr;
    struct edge_attr* ea;
    uint16_t version;

    if (ctx->edge_attrs == NULL)
        return 0;

    hmap_for_each (ctx->edge_attrs, i, id, ea)
    {
        (void)id, (void)ea;
        edge_attr_deinit(edge_attr_hmap_erase_slot(ctx->edge_attrs, i));
    }

    version = deserialize_lu16(des);
    if (deserializer_err(des))
        return 0;

    switch (version)
    {
        case 0x000: {
            count = deserialize_li16(des);
            while (count-- > 0)
            {
                edge_id   = deserialize_li32(des);
                edge_expr = deserialize_cstr(des);
                ea = edge_attr_hmap_emplace_new(&ctx->edge_attrs, edge_id);
                if (ea == NULL)
                    return -1;
                str_init(&ea->expr_str);
                str_set_cstr(&ea->expr_str, edge_expr);
            }
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_notify_interface* notify_interface,
    struct plugin_notify_context* notify_ctx,
    GTypeModule* type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    ctx->notify_interface  = notify_interface;
    ctx->notify_ctx        = notify_ctx;
    edge_attr_hmap_init(&ctx->edge_attrs);

#if defined(PLUGIN_MICROUI)
    (void)type_module;
#else
    graph_editor_register_type_internal(type_module);
#endif

    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    int i, id;
    struct edge_attr* ea;
    (void)type_module;

    hmap_for_each (ctx->edge_attrs, i, id, ea)
    {
        (void)id, (void)ea;
        edge_attr_deinit(edge_attr_hmap_erase_slot(ctx->edge_attrs, i));
    }
    edge_attr_hmap_deinit(ctx->edge_attrs);

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
