#pragma once

#include <gtk/gtk.h>

#define PLUGIN_TYPE_GRAPH_EDITOR (graph_editor_get_type())
G_DECLARE_FINAL_TYPE(GraphEditor, graph_editor, PLUGIN, GRAPH_EDITOR, GtkBox);

struct csfg_graph;
struct plugin_ctx;
struct plugin_callbacks_interface;
struct plugin_callbacks;

void         graph_editor_register_type_internal(GTypeModule* type_module);
GraphEditor* graph_editor_new(
    struct plugin_ctx*                       plugin_ctx,
    const struct plugin_callbacks_interface* icb,
    struct plugin_callbacks*                 cb);
void graph_editor_set_graph(GraphEditor* editor, struct csfg_graph* g);
void graph_editor_notify_graph_changed(
    GraphEditor* editor, struct csfg_graph* g);
