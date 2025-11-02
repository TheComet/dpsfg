#pragma once

#include <stdint.h>

struct plugin_ctx;
struct csfg_graph;
struct csfg_expr_pool;
struct csfg_var_table;
struct csfg_tf_expr;
struct csfg_tf;

typedef struct _GtkWidget   GtkWidget;
typedef struct _GTypeModule GTypeModule;

struct dpsfg_plugin_callbacks;

/*!
 * Plugins are able to modify structures provided by the main application.
 * Whenever this happens, the plugin should call the appropriate notification
 * function to propagate changes.
 *
 * To prevent self-calling, plugins must pass in their context object so that
 * the main application can filter the callbacks.
 */
struct plugin_notify_interface
{
    /*! Call this if the graph changes in any way that would alter the
     * resulting transfer function. This includes adding/removing nodes/edges,
     * setting the input or output nodes, or changing an edge expression.
     *
     * The graph is passed in via @see dpsfg_graph_interface. */
    void (*graph_structure_changed)(
        struct dpsfg_plugin_callbacks* ctx,
        const struct plugin_ctx*       source_plugin,
        int                            node_in,
        int                            node_out);

    /*! Call this if the substitution table is altered. The substitution table
     * is passed in via @see dpsfg_substitutions_interface.  */
    void (*substitutions_changed)(
        struct dpsfg_plugin_callbacks* ctx,
        const struct plugin_ctx*       source_plugin);

    /*! Call this if the parameters table is altered. The parameters table is
     * passed in via @see dpsfg_parameters_interface. */
    void (*parameters_changed)(
        struct dpsfg_plugin_callbacks* ctx,
        const struct plugin_ctx*       source_plugin);
};

struct dpsfg_ui_center_interface
{
    GtkWidget* (*create)(struct plugin_ctx* ctx);
    void (*destroy)(struct plugin_ctx* ctx, GtkWidget* view);
};

struct dpsfg_ui_pane_interface
{
    GtkWidget* (*create)(struct plugin_ctx* ctx);
    void (*destroy)(struct plugin_ctx* ctx, GtkWidget* view);
};

/*!
 * Implement this if your plugin wants to view or make changes to the
 * signal-flow-graph.
 */
struct dpsfg_graph_interface
{
    /*! Called when a new graph is created or loaded by the main application.
     * The pointer remains valid up until on_clear() is called, so it is safe
     * to store the pointer internally for later use. This function is
     * guaranteed to be called before on_structure_changed(). If a plugin makes
     * any modifications to the graph, it should call
     * graph_structure_changed(). @see plugin_notify_interface */
    void (*on_set)(struct plugin_ctx* ctx, struct csfg_graph* graph);

    /*! Called when the graph is deallocated by the main application. A plugin
     * should clear any references to the graph pointer passed by on_set(). */
    void (*on_clear)(struct plugin_ctx* ctx);

    /*! Called whenever the graph's structure is changed. This function is
     * triggered by calls to graph_structure_changed(). Note that the main
     * application will not call on_structure_changed() for the plugin that
     * triggered it. */
    void (*on_structure_changed)(struct plugin_ctx* ctx);
};

/*!
 * Implement this if your plugin wants to display the various symbolic
 * intermediate results.
 */
struct dpsfg_expr_interface
{
    /*! Called when the graph's symbolic expression is calculated. The
     * expression will be large and messy, as it is the raw output of mason's
     * gain rule. */
    void (*on_mason_expr)(
        struct plugin_ctx* ctx, const struct csfg_expr_pool* pool, int expr);

    /*! Called after the substitution table has been applied to the graph's
     * expression. This will mostly make the expression even larger than
     * on_mason_expr(). */
    void (*on_substituted_expr)(
        struct plugin_ctx* ctx, const struct csfg_expr_pool* pool, int expr);

    /*! Called after all limits have been applied, for example, lim A->oo. The
     * expression will usually be much simpler at this stage. */
    void (*on_limit_expr)(
        struct plugin_ctx* ctx, const struct csfg_expr_pool* pool, int expr);

    /*! Called after converting the limit expression into a rational function.
     * This is also known as the "transfer function", or "tf" for short. */
    void (*on_tf_expr)(
        struct plugin_ctx*           ctx,
        const struct csfg_expr_pool* pool,
        const struct csfg_tf_expr*   tf);
};

/*!
 * There are two variable tables used at different stages in the pipeline, that
 * share very similar interfaces but have different purposes:
 *   1) Substitutions table
 *   2) Parameters table
 *
 * The substitutions table contains user-defined symbolic expressions, which
 * are inserted into the graphs expression directly after evaluating mason's
 * gain rule, but before evaluating limits and calculating the transfer
 * function. For example, maybe the gain "A" needs to be substituted with "oo"
 * (infinity), or "w/s". Such substitutions can be inserted into this table.
 *
 * The parameters table on the other hand is auto-generated by the main
 * application and serves to completely evaluate the transfer function. It
 * contains all unresolved symbols and maps them to default values. Plugins can
 * alter the values of these parameters, but the parameters table may only
 * resolve to real values and NOT expressions.
 */
struct dpsfg_substitutions_interface
{
    /*! Called when a new substitutions table is created by the main
     * application. Initially, it will be empty. The main application does not
     * perform any insertions or deletions. This is handled by plugins. The
     * pointer remains valid up until on_clear() is called, so it is safe to
     * store the pointer internally for later use. This function is guaranteed
     * to be called before on_changed(). */
    void (*on_set)(
        struct plugin_ctx* ctx, struct csfg_var_table* substitutions);

    /*! Called when the substitutions table is deallocated by the main
     * application. A plugin should clear any references to the table pointer
     * passed in by on_set(). */
    void (*on_clear)(struct plugin_ctx* ctx);

    void (*on_changed)(struct plugin_ctx* ctx);
};
struct dpsfg_parameters_interface
{
    /*! Called when a new parameters table is created by the main application.
     * The pointer remains valid up until on_clear() is called, so it is safe
     * to store the pointer internally for later use. This function is
     * guaranteed to be called before on_xxx_changed(). */
    void (*on_set)(struct plugin_ctx* ctx, struct csfg_var_table* parameters);

    /*! Called when the parameters table is deallocated by the main
     * application. A plugin should clear any references to the table pointer
     * passed in by on_set(). */
    void (*on_clear)(struct plugin_ctx* ctx);

    void (*on_changed)(struct plugin_ctx* ctx);
};

struct dpsfg_numeric_interface
{
    void (*on_tf_changed)(struct plugin_ctx* ctx, const struct csfg_tf* tf);
};

struct dpsfg_plugin_info
{
    const char* name;
    const char* category;
    const char* author;
    const char* contact;
    const char* description;
};

struct dpsfg_plugin_interface
{
    uint32_t                  plugin_version;
    uint32_t                  app_version;
    struct dpsfg_plugin_info* info;

    /* Main create/destroy of plugin context */
    struct plugin_ctx* (*create)(
        const struct plugin_notify_interface* icb,
        struct dpsfg_plugin_callbacks*        cb,
        GTypeModule*                          type_module);
    void (*destroy)(struct plugin_ctx* ctx, GTypeModule* type_module);

    /* Various interfaces that can optionally be implemented */
    const struct dpsfg_ui_center_interface*     ui_center;
    const struct dpsfg_ui_pane_interface*       ui_pane;
    const struct dpsfg_graph_interface*         graph;
    const struct dpsfg_substitutions_interface* substitutions;
    const struct dpsfg_expr_interface*          expr;
    const struct dpsfg_parameters_interface*    parameters;
    const struct dpsfg_numeric_interface*       numeric;
};
