#pragma once

#include <gtk/gtk.h>

#define PLUGIN_TYPE_GRAPH_EDITOR (graph_editor_get_type())
G_DECLARE_FINAL_TYPE(GraphEditor, graph_editor, PLUGIN, GRAPH_EDITOR, GtkBox);

void graph_editor_register_type_internal(GTypeModule* type_module);
GraphEditor* graph_editor_new(void);
