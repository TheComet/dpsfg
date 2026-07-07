#pragma once

#include "csfg/util/hmap.h"
#include <gtk/gtk.h>

struct node_attr
{
    double radius;
};

struct edge_attr
{
    struct str* expr_str;
};

void edge_attr_deinit(struct edge_attr* ea);

HMAP_DECLARE(extern, node_attr_hmap, int, struct node_attr, 16)
HMAP_DECLARE(extern, edge_attr_hmap, int, struct edge_attr, 16)

#define PLUGIN_TYPE_GRAPH_EDITOR (graph_editor_get_type())
G_DECLARE_FINAL_TYPE(GraphEditor, graph_editor, PLUGIN, GRAPH_EDITOR, GtkBox)

struct csfg_graph;
struct serializer;
struct deserializer;
struct plugin_ctx;
struct plugin_notify_interface;
struct plugin_notify_context;

void graph_editor_register_type_internal(GTypeModule* type_module);
GraphEditor* graph_editor_new(
    struct plugin_ctx* plugin_ctx,
    const struct plugin_notify_interface* icb,
    struct plugin_notify_context* cb);
void graph_editor_set_graph(
    GraphEditor* editor, struct csfg_graph* g, int node_in, int node_out);
void graph_editor_clear_graph(GraphEditor* editor);
void graph_editor_rebuild_graph(GraphEditor* editor, int node_in, int node_out);
void graph_editor_redraw_graph(GraphEditor* editor);
struct edge_attr_hmap* graph_editor_take_edge_attributes(GraphEditor* editor);
void graph_editor_set_edge_attributes(
    GraphEditor* editor, struct edge_attr_hmap* edge_attrs);
void graph_editor_clear_attrs(GraphEditor* editor);
