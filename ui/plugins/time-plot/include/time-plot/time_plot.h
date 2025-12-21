#pragma once

#include <gtk/gtk.h>

struct csfg_tf;

#define PLUGIN_TYPE_TIME_PLOT (time_plot_get_type())
G_DECLARE_FINAL_TYPE(TimePlot, time_plot, PLUGIN, time_PLOT, GtkBox)

void      time_plot_register_type_internal(GTypeModule* type_module);
TimePlot* time_plot_new(void);
void      time_plot_set_tf(TimePlot* plot, const struct csfg_tf* tf);
