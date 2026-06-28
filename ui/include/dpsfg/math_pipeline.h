#pragma once

#include "csfg/graph/graph.h"
#include "csfg/numeric/tf.h"
#include "csfg/symbolic/rulebook.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"

struct dpsfg_plugin_interface;
struct deserializer;
struct serializer;
struct plugin_ctx;

enum math_pipeline_state
{
    MATH_PIPELINE_GRAPH_CHANGED,
    MATH_PIPELINE_SUBSTITUTIONS_CHANGED,
    MATH_PIPELINE_PARAMETERS_CHANGED
};

struct math_pipeline
{
    struct csfg_rulebook rulebook;

    struct csfg_graph graph;
    struct csfg_path_vec* paths;
    struct csfg_path_vec* loops;
    int node_in, node_out;

    struct csfg_var_table substitutions;

    struct csfg_expr_pool* graph_pool;
    struct csfg_expr_pool* subs_pool;
    struct csfg_expr_pool* lim_pool;
    int graph_expr;
    int subs_expr;
    int lim_expr;

    struct csfg_expr_pool* tf_pool;
    struct csfg_tf_expr tf_expr;

    struct csfg_var_table parameters;

    struct csfg_tf tf;
    struct csfg_pfd_poly* pfd_impulse;
    struct csfg_pfd_poly* pfd_step;
    struct csfg_pfd_poly* pfd_ramp;
};

int math_pipeline_init(struct math_pipeline* pl);
void math_pipeline_deinit(struct math_pipeline* pl);
void math_pipeline_clear(struct math_pipeline* pl);
int math_pipeline_load(struct math_pipeline* pl, struct deserializer* des);
int math_pipeline_save(const struct math_pipeline* pl, struct serializer** ser);
void math_pipeline_update(
    struct math_pipeline* pl, enum math_pipeline_state state);

void math_pipeline_notify_plugins(
    const struct math_pipeline* pipeline,
    const struct dpsfg_plugin_interface* plugin_iface,
    struct plugin_ctx* plugin_ctx,
    enum math_pipeline_state state,
    char is_source_plugin);
