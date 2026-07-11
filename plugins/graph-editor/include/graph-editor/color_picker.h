#pragma once

#include <gtk/gtk.h>

#define PLUGIN_TYPE_COLOR_PICKER (color_picker_get_type())
G_DECLARE_FINAL_TYPE(ColorPicker, color_picker, PLUGIN, COLOR_PICKER, GtkToggleButton)

void color_picker_register_type_internal(GTypeModule* type_module);
GtkWidget* color_picker_new(GdkRGBA color);
GdkRGBA color_picker_get_color(ColorPicker* picker);
