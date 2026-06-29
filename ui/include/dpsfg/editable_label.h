#pragma once

#include <gtk/gtk.h>

#define DPSFG_TYPE_EDITABLE_LABEL (dpsfg_editable_label_get_type())
G_DECLARE_FINAL_TYPE(
    DPSFGEditableLabel, dpsfg_editable_label, DPSFG, EDITABLE_LABEL, GtkBox)

GtkWidget* dpsfg_editable_label_new(void);
const char* dpsfg_editable_label_get_text(DPSFGEditableLabel* self);
void dpsfg_editable_label_set_text(DPSFGEditableLabel* self, const char* text);
