#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/log.h"
#include "csfg/util/tracker.h"
#include "csfg/util/vec.h"
#include "dpsfg/args.h"
#include "dpsfg/plugin.h"
#include "dpsfg/plugin_loader.h"
#include <gtk/gtk.h>

#define DPSFG_TYPE_PLUGIN_MODULE (dpsfg_plugin_module_get_type())
G_DECLARE_FINAL_TYPE(
    DPSFGPluginModule, dpsfg_plugin_module, DPSFG, PLUGIN_MODULE, GTypeModule)

struct _DPSFGPluginModule
{
    GTypeModule parent_instance;
};
struct _DPSFGPluginModuleClass
{
    GTypeModuleClass parent_class;
};
G_DEFINE_TYPE(DPSFGPluginModule, dpsfg_plugin_module, G_TYPE_TYPE_MODULE);

static gboolean dpsfg_plugin_module_load(GTypeModule* type_module)
{
    log_dbg("dpsfg_plugin_module_load()\n");
    track_mem(type_module, 0, "GTypeModule*");
    return TRUE;
}

static void dpsfg_plugin_module_unload(GTypeModule* type_module)
{
    log_dbg("dpsfg_plugin_module_unload()\n");
    untrack_mem(type_module);
}

static void dpsfg_plugin_module_init(DPSFGPluginModule* self)
{
}

static void dpsfg_plugin_module_class_init(DPSFGPluginModuleClass* class)
{
    GTypeModuleClass* module_class = G_TYPE_MODULE_CLASS(class);

    module_class->load = dpsfg_plugin_module_load;
    module_class->unload = dpsfg_plugin_module_unload;
}

struct plugin
{
    struct plugin_lib  lib;
    struct plugin_ctx* ctx;
    GTypeModule*       plugin_module;
    GtkWidget*         ui_center;
};

#if defined(CSFG_DEBUG_MEMORY)
static void
track_plugin_widget_deallocation(GtkWidget* self, gpointer user_pointer)
{
    untrack_mem(self);
}
#endif

VEC_DECLARE(plugin_vec, struct plugin, 8)
VEC_DEFINE(plugin_vec, struct plugin, 8)

static gboolean
shortcut_activated(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    log_dbg("activated shift+r\n");
    return TRUE;
}

static int
start_plugin(struct plugin* plugin, struct plugin_lib lib, GtkNotebook* center)
{
    int insert_pos;

    plugin->plugin_module = g_object_new(DPSFG_TYPE_PLUGIN_MODULE, NULL);
    if (plugin->plugin_module == NULL)
        goto alloc_plugin_module_failed;
    g_type_module_use(plugin->plugin_module);

    plugin->ctx = lib.i->create(NULL, plugin->plugin_module);
    if (plugin->ctx == NULL)
        goto create_context_failed;

    plugin->ui_center = NULL;
    if (lib.i->ui_center)
    {
        plugin->ui_center = lib.i->ui_center->create(plugin->ctx);
        if (plugin->ui_center == NULL)
            goto create_ui_center_failed;

#if defined(CSFG_DEBUG_MEMORY)
        track_mem(plugin->ui_center, 0, "ui_center");
        g_signal_connect(
            plugin->ui_center,
            "destroy",
            G_CALLBACK(track_plugin_widget_deallocation),
            NULL);
#endif
    }

    if (plugin->ui_center)
    {
        gtk_notebook_append_page(
            center, plugin->ui_center, gtk_label_new(lib.i->info->name));
    }

    plugin->lib = lib;
    return 0;

create_ui_pane_failed:
    if (plugin->ui_center)
        lib.i->ui_center->destroy(plugin->ctx, plugin->ui_center);
create_ui_center_failed:
    lib.i->destroy(plugin->plugin_module, plugin->ctx);
create_context_failed:
    g_type_module_unuse(plugin->plugin_module);
    g_object_unref(plugin->plugin_module);
alloc_plugin_module_failed:
    return -1;
}

static void stop_plugin(struct plugin* plugin)
{
    if (plugin->ui_center)
        plugin->lib.i->ui_center->destroy(plugin->ctx, plugin->ui_center);

    plugin->lib.i->destroy(plugin->plugin_module, plugin->ctx);
    g_type_module_unuse(plugin->plugin_module);
    g_object_unref(plugin->plugin_module);
    plugin_unload(&plugin->lib);
}

static void page_removed(
    GtkNotebook* self, GtkWidget* child, guint page_num, gpointer user_data)
{
    log_dbg("page_removed()\n");
}

static GtkWidget* property_panel_new(void)
{
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "page-removed", G_CALLBACK(page_removed), NULL);
    return notebook;
}

static GtkWidget* plugin_view_new(void)
{
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "page-removed", G_CALLBACK(page_removed), NULL);
    return notebook;
}

static void setup_global_shortcuts(GtkWidget* window)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction*  action;
    GtkShortcut*        shortcut;

    controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(
        GTK_SHORTCUT_CONTROLLER(controller), GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(window, controller);

    trigger = gtk_keyval_trigger_new(GDK_KEY_r, GDK_SHIFT_MASK);
    action = gtk_callback_action_new(shortcut_activated, NULL, NULL);
    shortcut = gtk_shortcut_new(trigger, action);
    gtk_shortcut_controller_add_shortcut(
        GTK_SHORTCUT_CONTROLLER(controller), shortcut);
}

struct on_plugin_ctx
{
    struct plugin_vec** plugins;
    GtkNotebook*        center_area;
};

static int on_plugin(struct plugin_lib lib, void* user)
{
    struct on_plugin_ctx* ctx = user;
    struct plugin*        plugin = plugin_vec_emplace(ctx->plugins);
    if (plugin == NULL)
        return -1;

    if (start_plugin(plugin, lib, ctx->center_area) == 0)
        return 1;

    /* Failed to start, remove plugin from list but continue loading */
    plugin_vec_pop(*ctx->plugins);
    return 0;
}

struct app_activate_ctx
{
    struct plugin_vec** plugins;
    struct csfg_graph   g;
};

static void activate(GtkApplication* app, gpointer user_data)
{
    GtkWidget* window;
    GtkWidget* project_browser;
    GtkWidget* paned1;
    GtkWidget* paned2;
    GtkWidget* plugin_view;
    GtkWidget* property_panel;

    struct on_plugin_ctx     on_plugin_ctx;
    struct app_activate_ctx* ctx = user_data;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "DPSFG");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    setup_global_shortcuts(window);

    plugin_view = plugin_view_new();
    on_plugin_ctx.plugins = ctx->plugins;
    on_plugin_ctx.center_area = GTK_NOTEBOOK(plugin_view);
    plugin_scan("share/dpsfg/plugins", on_plugin, &on_plugin_ctx);

    property_panel = property_panel_new();
    project_browser = gtk_notebook_new();

    paned2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(paned2), plugin_view);
    gtk_paned_set_end_child(GTK_PANED(paned2), property_panel);
    gtk_paned_set_resize_start_child(GTK_PANED(paned2), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned2), FALSE);
    // gtk_paned_set_position(GTK_PANED(paned2), 800);

    paned1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(paned1), project_browser);
    gtk_paned_set_end_child(GTK_PANED(paned1), paned2);
    gtk_paned_set_resize_start_child(GTK_PANED(paned1), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned1), TRUE);
    gtk_paned_set_position(GTK_PANED(paned1), 600);

    gtk_window_set_child(GTK_WINDOW(window), paned1);
    gtk_window_maximize(GTK_WINDOW(window));
    gtk_widget_set_visible(window, 1);

    csfg_graph_init(&ctx->g);
    int                    n1 = csfg_graph_add_node(&ctx->g, "V1");
    int                    n2 = csfg_graph_add_node(&ctx->g, "I2");
    struct csfg_expr_pool* e_pool;
    csfg_expr_pool_init(&e_pool);
    int e_expr = csfg_expr_parse(&e_pool, "G1 + G2 + s*C");
    csfg_graph_add_edge(&ctx->g, n1, n2, e_pool, e_expr);

    if (vec_count(*ctx->plugins))
    {
        struct plugin* plugin = vec_get(*ctx->plugins, 0);
        plugin->lib.i->graph->on_set(plugin->ctx, &ctx->g);
    }
}

int main(int argc, char** argv)
{
    struct args             args;
    struct app_activate_ctx ctx;
    struct plugin*          plugin;
    struct plugin_vec*      plugins;
    GtkApplication*         app;
    int                     status = EXIT_FAILURE;

    if (trackers_init_tls() != 0)
        goto mem_init_failed;
    log_init();

    switch (args_parse(&args, argc, argv))
    {
        case 0: break;
        case 1: return 0;
        default: goto parse_args_break;
    }

    if (args.tests)
    {
        int run_tests(int*, char*[]);
        status = run_tests(&argc, argv);
        goto parse_args_break;
    }

    plugin_vec_init(&plugins);
    ctx.plugins = &plugins;
    app = gtk_application_new(
        "ch.thecomet.dpsfg-ui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &ctx);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    vec_for_each (plugins, plugin)
        stop_plugin(plugin);
    plugin_vec_deinit(plugins);

    csfg_graph_deinit(&ctx.g);

parse_args_break:
    trackers_deinit_tls();
mem_init_failed:
    return status;
}
