#pragma once

#include <gtk/gtk.h>

struct csfg_tf;

#define PLUGIN_TYPE_POLE_ZERO_PLOT (pole_zero_plot_get_type())
G_DECLARE_FINAL_TYPE(
    PoleZeroPlot, pole_zero_plot, PLUGIN, POLE_ZERO_PLOT, GtkBox)

void          pole_zero_plot_register_type_internal(GTypeModule* type_module);
PoleZeroPlot* pole_zero_plot_new(void);
void pole_zero_plot_set_tf(PoleZeroPlot* plot, const struct csfg_tf* tf);
