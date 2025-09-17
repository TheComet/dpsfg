#include <gtk/gtk.h>

static gboolean
shortcut_activated(GtkWidget* widget,
    GVariant* unused,
    gpointer user_data)
{
    fprintf(stderr, "activated shift+r\n");
    return TRUE;
}

static void
page_removed(GtkNotebook* self, GtkWidget* child, guint page_num, gpointer user_data)
{
    fprintf(stderr, "page_removed()\n");
}

static GtkWidget*
property_panel_new(void)
{
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "page-removed", G_CALLBACK(page_removed), NULL);
    return notebook;
}

static GtkWidget*
plugin_view_new(void)
{
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "page-removed", G_CALLBACK(page_removed), NULL);
    return notebook;
}

static void
setup_global_shortcuts(GtkWidget* window)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction* action;
    GtkShortcut* shortcut;

    controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(
        GTK_SHORTCUT_CONTROLLER(controller),
        GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(window, controller);

    trigger = gtk_keyval_trigger_new(GDK_KEY_r, GDK_SHIFT_MASK);
    action = gtk_callback_action_new(shortcut_activated, NULL, NULL);
    shortcut = gtk_shortcut_new(trigger, action);
    gtk_shortcut_controller_add_shortcut(
        GTK_SHORTCUT_CONTROLLER(controller),
        shortcut);
}

static void
activate(GtkApplication* app, gpointer user_data)
{
    GtkWidget* window;
    GtkWidget* project_browser;
    GtkWidget* paned1;
    GtkWidget* paned2;
    GtkWidget* plugin_view;
    GtkWidget* property_panel;
    struct app_activate_ctx* ctx = user_data;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "DPSFG");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    setup_global_shortcuts(window);

    plugin_view = plugin_view_new();
    property_panel = property_panel_new();
    project_browser = gtk_notebook_new();

    paned2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(paned2), plugin_view);
    gtk_paned_set_end_child(GTK_PANED(paned2), property_panel);
    gtk_paned_set_resize_start_child(GTK_PANED(paned2), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned2), FALSE);
    //gtk_paned_set_position(GTK_PANED(paned2), 800);

    paned1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(paned1), project_browser);
    gtk_paned_set_end_child(GTK_PANED(paned1), paned2);
    gtk_paned_set_resize_start_child(GTK_PANED(paned1), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned1), TRUE);
    gtk_paned_set_position(GTK_PANED(paned1), 600);

    gtk_window_set_child(GTK_WINDOW(window), paned1);
    gtk_window_maximize(GTK_WINDOW(window));
    gtk_widget_set_visible(window, 1);
}

int main(int argc, char** argv)
{
    GtkApplication* app;
    int status;

    app = gtk_application_new("ch.thecomet.dpsfg-ui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

