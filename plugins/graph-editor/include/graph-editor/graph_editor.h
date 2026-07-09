#pragma once

#include <gtk/gtk.h>

struct csfg_graph;
struct serializer;
struct deserializer;
struct plugin_ctx;
struct plugin_notify_interface;
struct plugin_notify_context;

enum graph_model_mode
{
    MODE_NORMAL,
    MODE_MOVE,
    MODE_RECONNECT_FROM,
    MODE_RECONNECT_TO,
};

struct graph_model
{
    const struct plugin_notify_interface* icb;
    struct plugin_notify_context* cb;
    struct plugin_ctx* plugin_ctx;

    struct csfg_graph* graph;
    struct node_attr_hmap* node_attrs;
    struct edge_attr_hmap* edge_attrs;
    struct undo_stack_vec* undo_stack;

    int undo_stack_ptr;
    int active_node_id;
    int active_edge_id;
    int marked_node_id;
    int reconnect_node_id;
    int reconnect_edge_id;
    int node_in_id;
    int node_out_id;

    enum graph_model_mode mode;
};

void graph_model_init(
    struct graph_model* model,
    struct plugin_ctx* plugin_ctx,
    const struct plugin_notify_interface* icb,
    struct plugin_notify_context* cb);
void graph_model_deinit(struct graph_model* model);
int graph_model_save_attrs(struct graph_model* model, struct serializer** ser);
int graph_model_load_attrs(struct graph_model* model, struct deserializer* des);
void graph_model_clear_attrs(struct graph_model* model);
void graph_model_set_graph(
    struct graph_model* model, struct csfg_graph* g, int node_in, int node_out);
void graph_model_clear_graph(struct graph_model* model);
void graph_model_rebuild_graph(
    struct graph_model* model, int node_in, int node_out);

#define PLUGIN_TYPE_GRAPH_EDITOR (graph_editor_get_type())
G_DECLARE_FINAL_TYPE(GraphEditor, graph_editor, PLUGIN, GRAPH_EDITOR, GtkBox)

void graph_editor_register_type_internal(GTypeModule* type_module);
GraphEditor* graph_editor_new(struct graph_model* model);
void graph_editor_redraw_graph(GraphEditor* editor);
