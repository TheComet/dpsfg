#include "csfg/init.h"
#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/util/log.h"
#include "csfg/util/tracker.h"
#include "csfg/util/vec.h"
#include "dpsfg-plugin.h"
#include "ui/args.h"
#include "ui/db.h"
#include "ui/math_pipeline.h"
#include "ui/plugin_loader.h"
#include "ui/project_browser.h"
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
G_DEFINE_TYPE(DPSFGPluginModule, dpsfg_plugin_module, G_TYPE_TYPE_MODULE)

static gboolean dpsfg_plugin_module_load(GTypeModule* type_module)
{
    /*log_dbg("dpsfg_plugin_module_load()\n");*/
    /*track_mem(type_module, 0, "GTypeModule*");*/
    (void)type_module;
    return TRUE;
}

static void dpsfg_plugin_module_unload(GTypeModule* type_module)
{
    /*log_dbg("dpsfg_plugin_module_unload()\n");*/
    /*untrack_mem(type_module);*/
    (void)type_module;
}

static void dpsfg_plugin_module_init(DPSFGPluginModule* self)
{
    (void)self;
}

static void dpsfg_plugin_module_class_init(DPSFGPluginModuleClass* class)
{
    GTypeModuleClass* module_class = G_TYPE_MODULE_CLASS(class);

    module_class->load   = dpsfg_plugin_module_load;
    module_class->unload = dpsfg_plugin_module_unload;
}

struct plugin
{
    struct plugin_lib lib;
    struct plugin_ctx* ctx;
    GTypeModule* plugin_module;
    GtkWidget* ui_center;
    GtkWidget* ui_pane;
};

#if defined(CSFG_DEBUG_MEMORY)
static void
track_plugin_widget_deallocation(GtkWidget* self, gpointer user_data)
{
    (void)user_data;
    untrack_mem(self);
}
#endif

VEC_DECLARE(plugin_vec, struct plugin, 8)
VEC_DEFINE(plugin_vec, struct plugin, 8)

struct app_ctx
{
    struct db_interface* dbi;
    struct db* db;
    struct plugin_vec** plugins;
    struct plugin_notify_context* plugin_callbacks_ctx;
    struct math_pipeline pipeline;

    GtkWidget* paned1;
    GtkWidget* paned2;

    int active_project_id;
};

/* Implement the opaque pointer from plugin.h. This is passed in when plugins
 * call back to the main application. */
struct plugin_notify_context
{
    struct app_ctx* app;
};

/* -------------------------------------------------------------------------- */
static void notify_plugins_about_change(
    const struct plugin_vec* plugins,
    const struct plugin_ctx* source_plugin,
    const struct math_pipeline* pipeline,
    enum math_pipeline_state state)
{
    const struct plugin* plugin;
    vec_for_each (plugins, plugin)
    {
        math_pipeline_notify_plugins(
            pipeline,
            plugin->lib.i,
            plugin->ctx,
            state,
            plugin->ctx == source_plugin);
    }
}

/* -------------------------------------------------------------------------- */
static void graph_structure_changed_cb(
    struct plugin_notify_context* cb,
    const struct plugin_ctx* source_plugin,
    int node_in,
    int node_out)
{
    struct app_ctx* app            = cb->app;
    struct math_pipeline* pipeline = &app->pipeline;

    pipeline->node_in  = node_in;
    pipeline->node_out = node_out;

    math_pipeline_update(pipeline, MATH_PIPELINE_GRAPH_CHANGED);
    notify_plugins_about_change(
        *app->plugins, source_plugin, pipeline, MATH_PIPELINE_GRAPH_CHANGED);
}
static void graph_layout_changed_cb(
    struct plugin_notify_context* cb, const struct plugin_ctx* source_plugin)
{
    struct app_ctx* app            = cb->app;
    struct math_pipeline* pipeline = &app->pipeline;
    notify_plugins_about_change(
        *app->plugins, source_plugin, pipeline, MATH_PIPELINE_GRAPH_CHANGED);
}
static void substitutions_changed_cb(
    struct plugin_notify_context* cb, const struct plugin_ctx* source_plugin)
{
    struct app_ctx* ctx            = cb->app;
    struct math_pipeline* pipeline = &ctx->pipeline;
    math_pipeline_update(pipeline, MATH_PIPELINE_SUBSTITUTIONS_CHANGED);
    notify_plugins_about_change(
        *ctx->plugins,
        source_plugin,
        pipeline,
        MATH_PIPELINE_SUBSTITUTIONS_CHANGED);
}
static void parameters_changed_cb(
    struct plugin_notify_context* cb, const struct plugin_ctx* source_plugin)
{
    struct app_ctx* app            = cb->app;
    struct math_pipeline* pipeline = &app->pipeline;
    math_pipeline_update(pipeline, MATH_PIPELINE_PARAMETERS_CHANGED);
    notify_plugins_about_change(
        *app->plugins,
        source_plugin,
        pipeline,
        MATH_PIPELINE_PARAMETERS_CHANGED);
}
static struct plugin_notify_interface plugin_callbacks = {
    graph_structure_changed_cb,
    graph_layout_changed_cb,
    substitutions_changed_cb,
    parameters_changed_cb};

/* -------------------------------------------------------------------------- */
static gboolean
shorcut_quit_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    GtkWindow* window = user_data;
    gtk_window_close(window);
    (void)widget, (void)unused;
    return TRUE;
}

/* -------------------------------------------------------------------------- */
static int start_plugin(
    struct plugin* plugin,
    struct plugin_lib lib,
    struct plugin_notify_context* callbacks_ctx,
    GtkNotebook* center,
    GtkBox* pane)
{
    plugin->plugin_module = g_object_new(DPSFG_TYPE_PLUGIN_MODULE, NULL);
    if (plugin->plugin_module == NULL)
        goto alloc_plugin_module_failed;
    g_type_module_use(plugin->plugin_module);

    plugin->ctx =
        lib.i->create(&plugin_callbacks, callbacks_ctx, plugin->plugin_module);
    if (plugin->ctx == NULL)
        goto create_context_failed;

    plugin->ui_center = NULL;
    if (lib.i->ui_center)
    {
        plugin->ui_center = lib.i->ui_center->create(plugin->ctx);
        if (plugin->ui_center == NULL)
            goto create_ui_center_failed;

        gtk_notebook_append_page(
            center, plugin->ui_center, gtk_label_new(lib.i->info->name));

#if defined(CSFG_DEBUG_MEMORY)
        track_mem(plugin->ui_center, 0, "ui_center");
        g_signal_connect(
            plugin->ui_center,
            "destroy",
            G_CALLBACK(track_plugin_widget_deallocation),
            NULL);
#endif
    }

    plugin->ui_pane = NULL;
    if (lib.i->ui_pane)
    {
        GtkWidget* expander;
        GtkWidget* section_box;

        plugin->ui_pane = lib.i->ui_pane->create(plugin->ctx);
        if (plugin->ui_pane == NULL)
            goto create_ui_pane_failed;

        expander    = gtk_expander_new(lib.i->info->name);
        section_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_box_append(GTK_BOX(section_box), plugin->ui_pane);
        gtk_expander_set_child(GTK_EXPANDER(expander), section_box);
        gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
        gtk_widget_set_hexpand(expander, TRUE);

        gtk_box_append(pane, expander);

#if defined(CSFG_DEBUG_MEMORY)
        track_mem(plugin->ui_pane, 0, "ui_pane");
        g_signal_connect(
            plugin->ui_pane,
            "destroy",
            G_CALLBACK(track_plugin_widget_deallocation),
            NULL);
#endif
    }

    plugin->lib = lib;
    return 0;

create_ui_pane_failed:
    if (plugin->ui_center)
        lib.i->ui_center->destroy(plugin->ctx, plugin->ui_center);
create_ui_center_failed:
    lib.i->destroy(plugin->ctx, plugin->plugin_module);
create_context_failed:
    g_type_module_unuse(plugin->plugin_module);
    /* g_object_unref(plugin->plugin_module); NOTE: Might still need this */
alloc_plugin_module_failed:
    return -1;
}

/* -------------------------------------------------------------------------- */
static void stop_plugin(struct plugin* plugin)
{
    if (plugin->ui_pane)
        plugin->lib.i->ui_pane->destroy(plugin->ctx, plugin->ui_pane);
    if (plugin->ui_center)
        plugin->lib.i->ui_center->destroy(plugin->ctx, plugin->ui_center);

    plugin->lib.i->destroy(plugin->ctx, plugin->plugin_module);
    g_type_module_unuse(plugin->plugin_module);
    // g_object_unref(plugin->plugin_module);
    plugin_unload(&plugin->lib);
}

/* -------------------------------------------------------------------------- */
static void page_removed(
    GtkNotebook* self, GtkWidget* child, guint page_num, gpointer user_data)
{
    (void)self, (void)child, (void)page_num, (void)user_data;
    log_dbg("page_removed()\n");
}

/* -------------------------------------------------------------------------- */
static GtkWidget* plugin_view_new(void)
{
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "page-removed", G_CALLBACK(page_removed), NULL);
    return notebook;
}

/* -------------------------------------------------------------------------- */
static void setup_global_shortcuts(GtkWidget* window)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction* action;
    GtkShortcut* shortcut;

    controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(
        GTK_SHORTCUT_CONTROLLER(controller), GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(window, controller);

    trigger  = gtk_keyval_trigger_new(GDK_KEY_q, GDK_CONTROL_MASK);
    action   = gtk_callback_action_new(shorcut_quit_cb, window, NULL);
    shortcut = gtk_shortcut_new(trigger, action);
    gtk_shortcut_controller_add_shortcut(
        GTK_SHORTCUT_CONTROLLER(controller), shortcut);
}

/* -------------------------------------------------------------------------- */
struct on_plugin_ctx
{
    struct plugin_vec** plugins;
    struct plugin_notify_context* plugin_callbacks_ctx;
    GtkNotebook* center_area;
    GtkBox* property_pane;
};
static int on_plugin(struct plugin_lib lib, void* user)
{
    struct on_plugin_ctx* ctx = user;
    struct plugin* plugin     = plugin_vec_emplace(ctx->plugins);
    if (plugin == NULL)
        return -1;

    if (start_plugin(
            plugin,
            lib,
            ctx->plugin_callbacks_ctx,
            ctx->center_area,
            ctx->property_pane) == 0)
        return 1;

    /* Failed to start, remove plugin from list but continue loading */
    plugin_vec_pop(*ctx->plugins);
    return 0;
}

/* -------------------------------------------------------------------------- */
static gboolean set_initial_pane_widths(gpointer user_data)
{
    int total_width, pane_width;
    struct app_ctx* ctx = user_data;

    total_width = gtk_widget_get_width(ctx->paned1);
    if (total_width <= 0)
        return G_SOURCE_CONTINUE;

    pane_width = total_width * 0.15;
    if (pane_width < 200)
        pane_width = 200;

    gtk_paned_set_position(GTK_PANED(ctx->paned1), pane_width);
    gtk_paned_set_position(GTK_PANED(ctx->paned2), total_width - pane_width);

    return G_SOURCE_REMOVE;
}

/* -------------------------------------------------------------------------- */
static int
load_pipeline_graph_data_on_row(const void* data, int data_len, void* user_data)
{
    struct math_pipeline* pl = user_data;
    struct deserializer des  = deserializer(data, data_len);
    return math_pipeline_load(pl, &des);
}
static int load_pipeline_plugin_data_on_row(
    const void* data, int data_len, void* user_data)
{
    struct plugin* plugin   = user_data;
    struct deserializer des = deserializer(data, data_len);
    return plugin->lib.i->io->on_load(plugin->ctx, &des);
}
static int
load_empty_project(struct math_pipeline* pipeline, struct plugin_vec* plugins)
{
    struct plugin* plugin;

    math_pipeline_clear(pipeline);
    math_pipeline_update(pipeline, MATH_PIPELINE_GRAPH_CHANGED);
    notify_plugins_about_change(
        plugins, NULL, pipeline, MATH_PIPELINE_GRAPH_CHANGED);

    vec_for_each (plugins, plugin)
    {
        if (plugin->lib.i->graph)
            plugin->lib.i->graph->on_load(
                plugin->ctx,
                &pipeline->graph,
                pipeline->node_in,
                pipeline->node_out);
        if (plugin->lib.i->substitutions)
            plugin->lib.i->substitutions->on_load(
                plugin->ctx, &pipeline->substitutions);
        if (plugin->lib.i->parameters)
            plugin->lib.i->parameters->on_load(
                plugin->ctx, &pipeline->parameters);
    }

    return 0;
}
static int load_project(
    const struct db_interface* dbi,
    struct db* db,
    int project_id,
    struct math_pipeline* pipeline,
    struct plugin_vec* plugins)
{
    struct plugin* plugin;

    math_pipeline_clear(pipeline);

    if (dbi->graph_data.exists(db, project_id))
        if (dbi->graph_data.load(
                db, project_id, load_pipeline_graph_data_on_row, pipeline) != 0)
            return -1;

    vec_for_each (plugins, plugin)
    {
        if (plugin->lib.i->graph)
            plugin->lib.i->graph->on_load(
                plugin->ctx,
                &pipeline->graph,
                pipeline->node_in,
                pipeline->node_out);
        if (plugin->lib.i->substitutions)
            plugin->lib.i->substitutions->on_load(
                plugin->ctx, &pipeline->substitutions);
        if (plugin->lib.i->parameters)
            plugin->lib.i->parameters->on_load(
                plugin->ctx, &pipeline->parameters);
    }

    vec_for_each (plugins, plugin)
        if (plugin->lib.i->io != NULL &&
            dbi->plugin_data.exists(
                db, project_id, plugin->lib.i->info->name) &&
            dbi->plugin_data.load(
                db,
                project_id,
                plugin->lib.i->info->name,
                load_pipeline_plugin_data_on_row,
                plugin) != 0)
        {
            return -1;
        }

    math_pipeline_update(pipeline, MATH_PIPELINE_GRAPH_CHANGED);
    /* Don't notify graph structure changing, because on_load() handles that
     * already */
    notify_plugins_about_change(
        plugins, NULL, pipeline, MATH_PIPELINE_SUBSTITUTIONS_CHANGED);

    return 0;
}
static int unload_project(
    const struct db_interface* dbi,
    struct db* db,
    int project_id,
    struct math_pipeline* pipeline,
    struct plugin_vec* plugins)
{
    struct serializer* ser;
    struct plugin* plugin;

    serializer_init(&ser);

    vec_for_each (plugins, plugin)
        if (plugin->lib.i->io != NULL)
        {
            serializer_clear(ser);
            if (plugin->lib.i->io->on_save(plugin->ctx, &ser) != 0)
                continue;
            if (vec_count(ser) == 0)
                continue;
            if (dbi->plugin_data.save(
                    db,
                    project_id,
                    plugin->lib.i->info->name,
                    vec_data(ser),
                    vec_count(ser)) != 0)
                goto serialize_failed;
        }

    vec_for_each (plugins, plugin)
    {
        if (plugin->lib.i->graph)
            plugin->lib.i->graph->on_unload(plugin->ctx);
        if (plugin->lib.i->substitutions)
            plugin->lib.i->substitutions->on_unload(plugin->ctx);
        if (plugin->lib.i->parameters)
            plugin->lib.i->parameters->on_unload(plugin->ctx);
    }

    serializer_clear(ser);
    if (math_pipeline_save(pipeline, &ser) != 0)
        goto serialize_failed;
    if (dbi->graph_data.save(db, project_id, vec_data(ser), vec_count(ser)) !=
        0)
        goto serialize_failed;

    math_pipeline_clear(pipeline);

    serializer_deinit(ser);
    return 0;

serialize_failed:
    serializer_deinit(ser);
    return 0;
}
static void on_project_selected(
    DPSFGProjectBrowser* project_browser, int project_id, gpointer user_data)
{
    struct app_ctx* ctx = user_data;
    (void)project_browser;
    if (load_project(
            ctx->dbi, ctx->db, project_id, &ctx->pipeline, *ctx->plugins) != 0)
    {
        if (project_id == SCRATCH_PROJECT_ID)
            load_empty_project(&ctx->pipeline, *ctx->plugins);
        else
            project_id = -1;
    }
    ctx->active_project_id = project_id;
}
static void on_project_deselected(
    DPSFGProjectBrowser* project_browser, int project_id, gpointer user_data)
{
    struct app_ctx* ctx = user_data;
    (void)project_browser;
    if (ctx->active_project_id > -1)
        unload_project(
            ctx->dbi, ctx->db, project_id, &ctx->pipeline, *ctx->plugins);
    ctx->active_project_id = -1;
}

/* -------------------------------------------------------------------------- */
static void focus_first_center_plugin(struct plugin_vec* plugins)
{
    struct plugin* plugin;
    vec_for_each (plugins, plugin)
        if (plugin->ui_center != NULL)
        {
            gtk_widget_grab_focus(plugin->ui_center);
            break;
        }
}

/* -------------------------------------------------------------------------- */
static void activate(GtkApplication* app, gpointer user_data)
{
    GtkWidget* window;
    GtkWidget* project_browser;
    GtkWidget* center_area;
    GtkWidget* property_pane_container;
    GtkWidget* property_pane_scrolled_window;

    struct on_plugin_ctx on_plugin_ctx;
    struct app_ctx* ctx = user_data;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "DPSFG");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    setup_global_shortcuts(window);

    property_pane_scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(property_pane_scrolled_window),
        GTK_POLICY_NEVER,
        GTK_POLICY_AUTOMATIC);

    property_pane_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(property_pane_scrolled_window),
        property_pane_container);

    center_area     = plugin_view_new();
    project_browser = dpsfg_project_browser_new(ctx->dbi, ctx->db);

    ctx->paned2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(ctx->paned2), center_area);
    gtk_paned_set_end_child(
        GTK_PANED(ctx->paned2), property_pane_scrolled_window);
    gtk_paned_set_resize_start_child(GTK_PANED(ctx->paned2), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(ctx->paned2), FALSE);

    ctx->paned1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(ctx->paned1), project_browser);
    gtk_paned_set_end_child(GTK_PANED(ctx->paned1), ctx->paned2);
    gtk_paned_set_resize_start_child(GTK_PANED(ctx->paned1), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(ctx->paned1), TRUE);

    g_idle_add(set_initial_pane_widths, ctx);

    g_signal_connect(
        project_browser,
        "project-deselected",
        G_CALLBACK(on_project_deselected),
        ctx);
    g_signal_connect(
        project_browser,
        "project-selected",
        G_CALLBACK(on_project_selected),
        ctx);

    on_plugin_ctx.plugins              = ctx->plugins;
    on_plugin_ctx.center_area          = GTK_NOTEBOOK(center_area);
    on_plugin_ctx.property_pane        = GTK_BOX(property_pane_container);
    on_plugin_ctx.plugin_callbacks_ctx = ctx->plugin_callbacks_ctx;
    plugin_scan("share/dpsfg/plugins", on_plugin, &on_plugin_ctx);

    dpsfg_project_browser_reload_from_db(
        DPSFG_PROJECT_BROWSER(project_browser));

    gtk_window_set_child(GTK_WINDOW(window), ctx->paned1);
    gtk_window_maximize(GTK_WINDOW(window));
    gtk_widget_set_visible(window, 1);

    focus_first_center_plugin(*ctx->plugins);
}

/* -------------------------------------------------------------------------- */
static void shutdown(GtkApplication* app, gpointer user_data)
{
    struct plugin* plugin;
    struct app_ctx* ctx = user_data;
    (void)app;

    if (ctx->active_project_id != -1)
        unload_project(
            ctx->dbi,
            ctx->db,
            ctx->active_project_id,
            &ctx->pipeline,
            *ctx->plugins);

    vec_for_each (*ctx->plugins, plugin)
        stop_plugin(plugin);
    plugin_vec_clear(*ctx->plugins);
}

/* -------------------------------------------------------------------------- */
int main(int argc, char** argv)
{
    struct args args;
    struct app_ctx app_ctx;
    struct plugin_notify_context callbacks_ctx;
    struct plugin_vec* plugins;
    GtkApplication* app;
    int status = EXIT_FAILURE;

    if (csfg_init() != 0)
        goto csfg_init_failed;

    switch (args_parse(&args, argc, argv))
    {
        case 0 : break;
        case 1 : return 0;
        default: goto parse_args_break;
    }

    if (args.tests)
    {
        int run_tests(int*, char*[]);
        status = run_tests(&argc, argv);
        goto parse_args_break;
    }

    callbacks_ctx.app = &app_ctx;

    if (db_init() != 0)
        goto db_init_failed;
    app_ctx.dbi = db("sqlite3");
    app_ctx.db  = app_ctx.dbi->open("projects.db");
    if (app_ctx.db == NULL)
        goto open_db_failed;
    if (app_ctx.dbi->upgrade(app_ctx.db) != 0)
        goto migrate_db_failed;

    app_ctx.paned1 = NULL;
    app_ctx.paned2 = NULL;

    plugin_vec_init(&plugins);
    app_ctx.plugins              = &plugins;
    app_ctx.plugin_callbacks_ctx = &callbacks_ctx;

    math_pipeline_init(&app_ctx.pipeline);
    app_ctx.active_project_id = -1;

    app = gtk_application_new(
        "ch.thecomet.dpsfg-ui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &app_ctx);
    g_signal_connect(app, "shutdown", G_CALLBACK(shutdown), &app_ctx);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    math_pipeline_deinit(&app_ctx.pipeline);
    plugin_vec_deinit(plugins);

migrate_db_failed:
    app_ctx.dbi->close(app_ctx.db);
open_db_failed:
    db_deinit();
db_init_failed:
parse_args_break:
    csfg_deinit();
csfg_init_failed:
    return status;
}
