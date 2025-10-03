#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include "math-viewer/math_viewer.h"

struct plugin_ctx
{
    GtkWidget*                         top_level;
    MathViewer*                        graph_expr_viewer;
    MathViewer*                        standard_tf_viewer;
    MathViewer*                        tf_after_limit_viewer;
    struct plugin_callbacks_interface* callbacks;
};

/* -------------------------------------------------------------------------- */
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
    ctx->graph_expr_viewer = math_viewer_new();
    ctx->standard_tf_viewer = math_viewer_new();
    ctx->tf_after_limit_viewer = math_viewer_new();

    ctx->top_level = gtk_notebook_new();
    gtk_notebook_append_page(
        GTK_NOTEBOOK(ctx->top_level),
        GTK_WIDGET(ctx->tf_after_limit_viewer),
        gtk_label_new("TF with Limit"));
    gtk_notebook_append_page(
        GTK_NOTEBOOK(ctx->top_level),
        GTK_WIDGET(ctx->standard_tf_viewer),
        gtk_label_new("Transfer Function"));
    gtk_notebook_append_page(
        GTK_NOTEBOOK(ctx->top_level),
        GTK_WIDGET(ctx->graph_expr_viewer),
        gtk_label_new("Graph Expression"));

    return GTK_WIDGET(g_object_ref_sink(ctx->top_level));
}
static void ui_pane_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_callbacks_interface* icb,
    struct plugin_callbacks*                 cb,
    GTypeModule*                             type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    math_viewer_register_type_internal(type_module);
    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static void on_graph_expr(
    struct plugin_ctx* ctx, const struct csfg_expr_pool* pool, int expr)
{
    math_viewer_set_expr(ctx->graph_expr_viewer, pool, expr);
}
static void on_graph_tf(
    struct plugin_ctx*           ctx,
    const struct csfg_expr_pool* num_pool,
    int                          num_expr,
    const struct csfg_expr_pool* den_pool,
    int                          den_expr)
{
    math_viewer_set_expr(ctx->standard_tf_viewer, num_pool, num_expr);
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};

static struct dpsfg_expr_interface expr = {on_graph_expr, on_graph_tf};

static struct plugin_info info = {
    "Math Viewer",
    "viewer",
    "TheComet",
    "@TheComet93",
    "Renders various mathematical expressions"};

PLUGIN_API struct plugin_interface dpsfg_plugin = {
    PLUGIN_VERSION, 0, &info, create, destroy, NULL, &ui_pane, NULL, &expr};
