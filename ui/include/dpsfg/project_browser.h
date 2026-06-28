#pragma once

#include <gtk/gtk.h>

struct db_interface;
struct db;

#define DPSFG_TYPE_PROJECT_BROWSER (dpsfg_project_browser_get_type())
G_DECLARE_FINAL_TYPE(
    DPSFGProjectBrowser, dpsfg_project_browser, DPSFG, PROJECT_BROWSER, GtkBox)

GtkWidget* dpsfg_project_browser_new(void);
void dpsfg_project_browser_reload_from_db(
    DPSFGProjectBrowser* self, const struct db_interface* dbi, struct db* db);
