#pragma once

#include <stdint.h>

struct plugin_ctx;
struct csfg_graph;
struct csfg_expr_pool;

typedef struct _GtkWidget   GtkWidget;
typedef struct _GTypeModule GTypeModule;

struct plugin_callbacks;
struct plugin_callbacks_interface
{
    void (*graph_changed)(
        struct plugin_callbacks* ctx,
        const struct plugin_ctx* source_plugin,
        int                      node_in,
        int                      node_out);
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

struct dpsfg_graph_interface
{
    void (*on_set)(struct plugin_ctx* ctx, struct csfg_graph* graph);
    void (*on_changed)(struct plugin_ctx* ctx, struct csfg_graph* graph);
    void (*on_clear)(struct plugin_ctx* ctx);
};

struct dpsfg_expr_interface
{
    void (*on_graph_expr)(
        struct plugin_ctx* ctx, const struct csfg_expr_pool* pool, int expr);
    void (*on_graph_tf)(
        struct plugin_ctx* ctx, const struct csfg_expr_pool* pool, int expr);
};

struct plugin_info
{
    const char* name;
    const char* category;
    const char* author;
    const char* contact;
    const char* description;
};

struct plugin_interface
{
    uint32_t            plugin_version;
    uint32_t            vh_version;
    struct plugin_info* info;
    struct plugin_ctx* (*create)(
        const struct plugin_callbacks_interface* icb,
        struct plugin_callbacks*                 cb,
        GTypeModule*                             type_module);
    void (*destroy)(struct plugin_ctx* ctx, GTypeModule* type_module);
    const struct dpsfg_ui_center_interface* ui_center;
    const struct dpsfg_ui_pane_interface*   ui_pane;
    const struct dpsfg_graph_interface*     graph;
    const struct dpsfg_expr_interface*      expr;
};
