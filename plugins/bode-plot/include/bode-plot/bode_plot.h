#pragma once

#include <gtk/gtk.h>

struct csfg_tf;

#define PLUGIN_TYPE_BODE_PLOT (bode_plot_get_type())
G_DECLARE_FINAL_TYPE(BodePlot, bode_plot, PLUGIN, BODE_PLOT, GtkBox)

void      bode_plot_register_type_internal(GTypeModule* type_module);
BodePlot* bode_plot_new(void);
void      bode_plot_set_tf(BodePlot* plot, const struct csfg_tf* tf);
