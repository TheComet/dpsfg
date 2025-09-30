#pragma once

#include <stdint.h>

struct plugin_ctx;
struct csfg_graph;

typedef struct _GtkWidget   GtkWidget;
typedef struct _GTypeModule GTypeModule;

struct plugin_callbacks
{
    void (*graph_changed)(struct plugin_callbacks* cb);
};

struct dpsfg_ui_center_interface
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
        const struct plugin_callbacks* cb, GTypeModule* type_module);
    void (*destroy)(GTypeModule* type_module, struct plugin_ctx* ctx);
    const struct dpsfg_ui_center_interface* ui_center;
    const struct dpsfg_graph_interface*     graph;
};
