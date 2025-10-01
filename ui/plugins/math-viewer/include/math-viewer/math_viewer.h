#pragma once

#include <gtk/gtk.h>

#define PLUGIN_TYPE_MATH_VIEWER (math_viewer_get_type())
G_DECLARE_FINAL_TYPE(MathViewer, math_viewer, PLUGIN, MATH_VIEWER, GtkBox);

struct csfg_expr_pool;

void        math_viewer_register_type_internal(GTypeModule* type_module);
MathViewer* math_viewer_new(void);
void        math_viewer_set_graph_expr(
           MathViewer* viewer, const struct csfg_expr_pool* pool, int expr);
void math_viewer_set_graph_tf(
    MathViewer* viewer, const struct csfg_expr_pool* pool, int expr);
