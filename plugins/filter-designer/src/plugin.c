#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/mem.h"
#include "dpsfg-plugin.h"
#include <gtk/gtk.h>

#define NODE_SPACING 240

#define FILTERS_LIST                                                           \
    X(lp1,                                                                     \
      "Lowpass 1st Order",                                                     \
      create_low_order_filter_ui(ctx, G_CALLBACK(on_lp1_generate_clicked)))    \
    X(hp1,                                                                     \
      "Highpass 1st Order",                                                    \
      create_low_order_filter_ui(ctx, G_CALLBACK(on_hp1_generate_clicked)))    \
    X(lp2,                                                                     \
      "Lowpass 2nd Order",                                                     \
      create_low_order_filter_ui(ctx, G_CALLBACK(on_lp2_generate_clicked)))    \
    X(bp2,                                                                     \
      "Bandpass 2nd Order",                                                    \
      create_low_order_filter_ui(ctx, G_CALLBACK(on_bp2_generate_clicked)))    \
    X(hp2,                                                                     \
      "Highpass 2nd Order",                                                    \
      create_low_order_filter_ui(ctx, G_CALLBACK(on_hp2_generate_clicked)))    \
    X(butterworth, "Butterworth", create_butterworth_ui(ctx))

struct plugin_ctx
{
    const struct plugin_notify_interface* notify_interface;
    struct plugin_notify_context* notify_ctx;
    struct csfg_graph* graph;
    struct csfg_var_table* parameters;
    struct str* str;
    int order;
    unsigned we_are_in_control : 1;

#if !defined(PLUGIN_MICROUI)
    GtkWidget* stack;
#    define X(member, name, create_func) GtkWidget* member;
    FILTERS_LIST
#    undef X
#endif
};

/* -------------------------------------------------------------------------- */
static int generate_edge(
    struct csfg_graph* g, int* n_out, int* posx, int* posy, struct strview expr)
{
    int n, e;
    n = csfg_graph_add_node(g, "");
    if (n < 0)
        return -1;

    e = csfg_graph_add_edge_parse_expr(g, *n_out, n, expr);
    if (e < 0)
        return -1;

    csfg_graph_get_node(g, n)->x = *posx + NODE_SPACING;
    csfg_graph_get_node(g, n)->y = *posy;

    csfg_graph_get_edge(g, e)->x = *posx + NODE_SPACING / 2;
    csfg_graph_get_edge(g, e)->y = *posy;

    *posx += NODE_SPACING;
    *n_out = n;

    return e;
}

/* -------------------------------------------------------------------------- */
static void generate_graph_single_edge(struct plugin_ctx* ctx, const char* expr)
{
    int n_in, n_out, e;

    if (ctx->graph == NULL)
        return;

    csfg_graph_clear(ctx->graph);
    n_in  = csfg_graph_add_node(ctx->graph, "In");
    n_out = csfg_graph_add_node(ctx->graph, "Out");
    if (n_in < 0 || n_out < 0)
        goto fail;
    e = csfg_graph_add_edge_parse_expr(
        ctx->graph, n_in, n_out, cstr_view(expr));
    if (e < 0)
        goto fail;

    csfg_graph_get_node(ctx->graph, n_out)->x = NODE_SPACING;
    csfg_graph_get_edge(ctx->graph, e)->x     = NODE_SPACING / 2;

    ctx->notify_interface->graph_structure_changed(
        ctx->notify_ctx, ctx, n_in, n_out);
    return;

fail:
    csfg_graph_clear(ctx->graph);
    ctx->notify_interface->graph_structure_changed(
        ctx->notify_ctx, ctx, n_in, n_out);
}
static void generate_graph_butterworth(struct plugin_ctx* ctx)
{
    int k, n_in, n_out, posx, posy, is_odd;
    posx = 0, posy = 0;

    if (ctx->graph == NULL)
        return;

    csfg_graph_clear(ctx->graph);
    n_in = n_out = csfg_graph_add_node(ctx->graph, "In");

    /* Gain */
    if (generate_edge(ctx->graph, &n_out, &posx, &posy, cstr_view("k")) < 0)
        goto fail;

    is_odd = !!(ctx->order % 2);
    if (is_odd)
    {
        if (generate_edge(
                ctx->graph, &n_out, &posx, &posy, cstr_view("1/(s/c+1)")) < 0)
            goto fail;
    }

    for (k = 1; k <= ctx->order / 2; ++k)
    {
        double sk = 2 * cos(M_PI * (2 * k + ctx->order - 1) / (2 * ctx->order));
        if (str_fmt(&ctx->str, "1 / ((s/c)^2 - s/c*%f + 1)", sk) != 0)
            goto fail;

        if (generate_edge(
                ctx->graph, &n_out, &posx, &posy, str_view(ctx->str)) < 0)
            goto fail;
    }

    if (str_set_cstr(&csfg_graph_get_node(ctx->graph, n_out)->name, "Out") != 0)
        goto fail;

    ctx->notify_interface->graph_structure_changed(
        ctx->notify_ctx, ctx, n_in, n_out);
    return;

fail:
    csfg_graph_clear(ctx->graph);
    ctx->notify_interface->graph_structure_changed(
        ctx->notify_ctx, ctx, n_in, n_out);
}

/* -------------------------------------------------------------------------- */
#if !defined(PLUGIN_MICROUI)
static void on_lp1_generate_clicked(GtkButton* button, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    generate_graph_single_edge(ctx, "k/(s+wp)");
    ctx->we_are_in_control = 1;
    (void)button;
}
static void on_hp1_generate_clicked(GtkButton* button, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    generate_graph_single_edge(ctx, "k*s/(s+wp)");
    ctx->we_are_in_control = 1;
    (void)button;
}
static void on_lp2_generate_clicked(GtkButton* button, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    generate_graph_single_edge(ctx, "k*wp^2/(s^2+wp/qp*s+wp^2)");
    ctx->we_are_in_control = 1;
    (void)button;
}
static void on_bp2_generate_clicked(GtkButton* button, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    generate_graph_single_edge(ctx, "k*wp*s/(s^2+wp/qp*s+wp^2)");
    ctx->we_are_in_control = 1;
    (void)button;
}
static void on_hp2_generate_clicked(GtkButton* button, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    generate_graph_single_edge(ctx, "k*s^2/(s^2+wp/qp*s+wp^2)");
    ctx->we_are_in_control = 1;
    (void)button;
}
static void
on_butterworth_generate_clicked(GtkButton* button, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    generate_graph_butterworth(ctx);
    ctx->we_are_in_control = 1;
    (void)button;
}
static void on_butterworth_order_changed(GtkAdjustment* adj, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    ctx->order             = (int)gtk_adjustment_get_value(adj);
    if (ctx->we_are_in_control)
        generate_graph_butterworth(ctx);
}
static void
on_filter_selected(GtkDropDown* dropdown, GParamSpec* pspec, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    guint selected         = gtk_drop_down_get_selected(dropdown);
    guint idx              = 0;
    (void)pspec;

#    define X(member, name, create_func)                                       \
        if (selected == idx++)                                                 \
        {                                                                      \
            gtk_stack_set_visible_child(GTK_STACK(ctx->stack), ctx->member);   \
            if (ctx->we_are_in_control)                                        \
                on_##member##_generate_clicked(NULL, ctx);                     \
        }
    FILTERS_LIST
#    undef X
}
#endif

/* -------------------------------------------------------------------------- */
#if !defined(PLUGIN_MICROUI)
static GtkWidget* create_low_order_filter_ui(
    struct plugin_ctx* ctx, GCallback on_generate_graph_callback)
{
    GtkWidget* vbox;
    GtkWidget* button_update_graph;

    button_update_graph = gtk_button_new_with_label("Generate Graph");

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(vbox), button_update_graph);

    g_signal_connect(
        button_update_graph, "clicked", on_generate_graph_callback, ctx);

    return vbox;
}
static GtkWidget* create_butterworth_ui(struct plugin_ctx* ctx)
{
    GtkWidget* grid;
    GtkWidget* spin_button;
    GtkAdjustment* spin_adj;
    GtkWidget* button_update_graph;
    GtkWidget* info;

    info = gtk_label_new(
        "The Butterworth filter is designed to have as flat a frequency "
        "response as possible in the passband.");
    gtk_label_set_wrap(GTK_LABEL(info), TRUE);

    spin_adj    = gtk_adjustment_new(ctx->order, 1.0, 12.0, 1.0, 0.0, 0.0);
    spin_button = gtk_spin_button_new(spin_adj, 0.0, 1);

    button_update_graph = gtk_button_new_with_label("Generate Graph");

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_attach(GTK_GRID(grid), info, 0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Order"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_update_graph, 0, 2, 2, 1);

    g_signal_connect(
        spin_adj,
        "value-changed",
        G_CALLBACK(on_butterworth_order_changed),
        ctx);
    g_signal_connect(
        button_update_graph,
        "clicked",
        G_CALLBACK(on_butterworth_generate_clicked),
        ctx);

    return grid;
}
#endif
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
#if defined(PLUGIN_MICROUI)
    (void)ctx;
    return NULL;
#else
    GtkWidget* dropdown;
    GtkWidget* top;

    static const char* filter_names[] = {
#    define X(member, name, create_func) name,
        FILTERS_LIST
#    undef X
            NULL,
    };
    dropdown = gtk_drop_down_new_from_strings(filter_names);

    ctx->stack = gtk_stack_new();
#    define X(member, name, create_func)                                       \
        ctx->member = create_func;                                             \
        gtk_stack_add_child(GTK_STACK(ctx->stack), ctx->member);
    FILTERS_LIST
#    undef X

    top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(top), dropdown);
    gtk_box_append(GTK_BOX(top), ctx->stack);

    g_signal_connect(
        dropdown, "notify::selected-item", G_CALLBACK(on_filter_selected), ctx);

    return g_object_ref_sink(top);
#endif
}
static void ui_pane_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
#if defined(PLUGIN_MICROUI)
    (void)ctx, (void)ui;
#else
    (void)ctx;
    g_object_unref(ui);
#endif
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_notify_interface* notify_interface,
    struct plugin_notify_context* notify_ctx,
    GTypeModule* type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    (void)type_module;

    str_init(&ctx->str);

    ctx->notify_interface  = notify_interface;
    ctx->notify_ctx        = notify_ctx;
    ctx->graph             = NULL;
    ctx->parameters        = NULL;
    ctx->we_are_in_control = 0;
    ctx->order             = 6;

    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    str_deinit(ctx->str);
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static void on_graph_load(
    struct plugin_ctx* ctx, struct csfg_graph* graph, int node_in, int node_out)
{
    ctx->graph = graph;
    (void)node_in, (void)node_out;

    if (csfg_graph_node_count(graph) == 0)
        ctx->we_are_in_control = 1;
    else
        ctx->we_are_in_control = 0;
}
static void on_graph_unload(struct plugin_ctx* ctx)
{
    ctx->graph             = NULL;
    ctx->we_are_in_control = 0;
}
static void
on_graph_structure_changed(struct plugin_ctx* ctx, int node_in, int node_out)
{
    (void)node_in, (void)node_out;
    ctx->we_are_in_control = 0;
}
static void on_graph_layout_changed(struct plugin_ctx* ctx)
{
    (void)ctx;
}

/* -------------------------------------------------------------------------- */
static void
on_parameters_set(struct plugin_ctx* ctx, struct csfg_var_table* parameters)
{
    ctx->parameters = parameters;
}
static void on_parameters_clear(struct plugin_ctx* ctx)
{
    ctx->parameters = NULL;
}
static void on_parameters_changed(struct plugin_ctx* ctx)
{
    (void)ctx;
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};
static struct dpsfg_graph_interface graph = {
    on_graph_load,
    on_graph_unload,
    on_graph_structure_changed,
    on_graph_layout_changed};
static struct dpsfg_parameters_interface parameters = {
    on_parameters_set, on_parameters_clear, on_parameters_changed};

static struct dpsfg_plugin_info info = {
    "Filter Designer",
    "editor",
    "TheComet",
    "@TheComet93",
    "Design common filters"};

PLUGIN_API struct dpsfg_plugin_interface dpsfg_plugin = {
    PLUGIN_VERSION,
    0,
    &info,
    create,
    destroy,
    NULL,
    &ui_pane,
    &graph,
    NULL,
    NULL,
    &parameters,
    NULL,
    NULL};
