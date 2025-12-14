#include "csfg/graph/graph.h"
#include "csfg/numeric/tf.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
#include "dpsfg/math_pipeline.h"
#include "dpsfg/plugin.h"

/* -------------------------------------------------------------------------- */
void math_pipeline_init(struct math_pipeline* pl)
{
    csfg_graph_init(&pl->graph);
    csfg_path_vec_init(&pl->paths);
    csfg_path_vec_init(&pl->loops);
    pl->node_in = -1;
    pl->node_out = -1;

    csfg_var_table_init(&pl->substitutions);

    csfg_expr_pool_init(&pl->graph_pool);
    csfg_expr_pool_init(&pl->subs_pool);
    csfg_expr_pool_init(&pl->lim_pool);
    pl->graph_expr = -1;
    pl->subs_expr = -1;
    pl->lim_expr = -1;

    csfg_expr_pool_init(&pl->tf_pool);
    csfg_tf_expr_init(&pl->tf_expr);

    csfg_var_table_init(&pl->parameters);

    csfg_tf_init(&pl->tf);
}

/* -------------------------------------------------------------------------- */
void math_pipeline_deinit(struct math_pipeline* pl)
{
    csfg_var_table_deinit(&pl->parameters);
    csfg_tf_expr_deinit(&pl->tf_expr);
    csfg_expr_pool_deinit(pl->tf_pool);
    csfg_expr_pool_deinit(pl->lim_pool);
    csfg_expr_pool_deinit(pl->subs_pool);
    csfg_expr_pool_deinit(pl->graph_pool);
    csfg_var_table_deinit(&pl->substitutions);
    csfg_path_vec_deinit(pl->loops);
    csfg_path_vec_deinit(pl->paths);
    csfg_graph_deinit(&pl->graph);
    csfg_tf_deinit(&pl->tf);
}

/* -------------------------------------------------------------------------- */
static void calc_graph_expression(struct math_pipeline* pipeline)
{
    /* It's OK if node_in/node_out are -1 here */
    csfg_path_vec_clear(pipeline->paths);
    csfg_graph_find_forward_paths(
        &pipeline->graph,
        &pipeline->paths,
        pipeline->node_in,
        pipeline->node_out);

    csfg_path_vec_clear(pipeline->loops);
    csfg_graph_find_loops(&pipeline->graph, &pipeline->loops);

    csfg_expr_pool_clear(pipeline->graph_pool);
    pipeline->graph_expr = csfg_graph_mason(
        &pipeline->graph,
        &pipeline->graph_pool,
        pipeline->paths,
        pipeline->loops);
    if (pipeline->graph_expr > -1)
    {
        csfg_expr_op_run(
            &pipeline->graph_pool,
            csfg_expr_opt_fold_constants,
            csfg_expr_opt_remove_useless_ops,
            NULL);
        pipeline->graph_expr =
            csfg_expr_gc(pipeline->graph_pool, pipeline->graph_expr);
    }
}
static void calc_substitutions(struct math_pipeline* pipeline)
{
    /* It's OK if graph_expr is -1 here */
    csfg_expr_pool_clear(pipeline->subs_pool);
    pipeline->subs_expr = csfg_expr_dup_recurse_from(
        &pipeline->subs_pool, pipeline->graph_pool, pipeline->graph_expr);
    if (pipeline->subs_expr > -1)
        if (csfg_expr_insert_substitutions(
                &pipeline->subs_pool,
                pipeline->subs_expr,
                &pipeline->substitutions) != 0)
        {
            csfg_expr_pool_clear(pipeline->subs_pool);
            pipeline->subs_expr = -1;
        }
    if (pipeline->subs_expr > -1)
    {
        csfg_expr_op_run(
            &pipeline->subs_pool,
            csfg_expr_opt_fold_constants,
            csfg_expr_opt_remove_useless_ops,
            NULL);
        pipeline->subs_expr =
            csfg_expr_gc(pipeline->subs_pool, pipeline->subs_expr);
    }
}
static void calc_limits(struct math_pipeline* pipeline)
{
    csfg_expr_pool_clear(pipeline->lim_pool);
    pipeline->lim_expr = -1;
    if (pipeline->subs_expr > -1)
        pipeline->lim_expr = csfg_expr_apply_limits(
            pipeline->subs_pool,
            pipeline->subs_expr,
            &pipeline->substitutions,
            &pipeline->lim_pool);
    if (pipeline->lim_expr > -1)
    {
        // TODO: Make this optimization pass more generic
        csfg_expr_op_run(
            &pipeline->lim_pool,
            csfg_expr_opt_fold_constants,
            csfg_expr_opt_remove_useless_ops,
            NULL);
        pipeline->lim_expr =
            csfg_expr_gc(pipeline->lim_pool, pipeline->lim_expr);
        csfg_expr_op_simplify_sums(&pipeline->lim_pool);
        csfg_expr_op_run(
            &pipeline->lim_pool,
            csfg_expr_opt_fold_constants,
            csfg_expr_opt_remove_useless_ops,
            NULL);
        pipeline->lim_expr =
            csfg_expr_gc(pipeline->lim_pool, pipeline->lim_expr);
    }
}
static void calc_symbolic_tf(struct math_pipeline* pipeline)
{
    csfg_tf_expr_clear(&pipeline->tf_expr);
    csfg_expr_pool_clear(pipeline->tf_pool);
    if (pipeline->lim_expr > -1)
        if (csfg_expr_to_rational(
                pipeline->lim_pool,
                pipeline->lim_expr,
                cstr_view("s"),
                &pipeline->tf_pool,
                &pipeline->tf_expr) != 0)
        {
            csfg_expr_pool_clear(pipeline->tf_pool);
            csfg_tf_expr_clear(&pipeline->tf_expr);
        }
    if (csfg_expr_pool_count(pipeline->tf_pool) > 0)
    {
        csfg_expr_op_run(
            &pipeline->tf_pool,
            csfg_expr_opt_fold_constants,
            csfg_expr_opt_remove_useless_ops,
            NULL);
    }
}
static void repopulate_parameter_table(struct math_pipeline* pipeline)
{
    const struct csfg_coeff_expr* coeff;

    csfg_var_table_reset_visited(&pipeline->parameters);

    vec_for_each (pipeline->tf_expr.num, coeff)
        csfg_var_table_populate(
            &pipeline->parameters, pipeline->tf_pool, coeff->expr);

    vec_for_each (pipeline->tf_expr.den, coeff)
        csfg_var_table_populate(
            &pipeline->parameters, pipeline->tf_pool, coeff->expr);

    csfg_var_table_erase_unvisited(&pipeline->parameters);
}
static void calc_numeric_tf(struct math_pipeline* mp)
{
    csfg_tf_from_symbolic(&mp->tf, mp->tf_pool, &mp->tf_expr, &mp->parameters);
}
void math_pipeline_update(
    struct math_pipeline* mp, enum math_pipeline_state state)
{
    switch (state)
    {
        case MATH_PIPELINE_GRAPH_CHANGED: /**/
            calc_graph_expression(mp);
            /* fallthrough */
        case MATH_PIPELINE_SUBSTITUTIONS_CHANGED: /**/
            calc_substitutions(mp);
            calc_limits(mp);
            calc_symbolic_tf(mp);
            repopulate_parameter_table(mp);
            /* fallthrough */
        case MATH_PIPELINE_PARAMETERS_CHANGED: /**/
            calc_numeric_tf(mp);
            /* fallthrough */
    }
}

/* -------------------------------------------------------------------------- */
static void notify_graph_changed(
    const struct math_pipeline*          pipeline,
    const struct dpsfg_plugin_interface* i,
    struct plugin_ctx*                   ctx)
{
    if (i->graph == NULL)
        return;

    i->graph->on_structure_changed(ctx, pipeline->node_in, pipeline->node_out);
    i->graph->on_layout_changed(ctx);
}
static void notify_expr_changed(
    const struct math_pipeline*          pipeline,
    const struct dpsfg_plugin_interface* i,
    struct plugin_ctx*                   ctx)
{
    if (i->expr == NULL)
        return;

    i->expr->on_mason_expr(ctx, pipeline->graph_pool, pipeline->graph_expr);
    i->expr->on_substituted_expr(ctx, pipeline->subs_pool, pipeline->subs_expr);
    i->expr->on_limit_expr(ctx, pipeline->lim_pool, pipeline->lim_expr);
    i->expr->on_tf_expr(ctx, pipeline->tf_pool, &pipeline->tf_expr);
}
static void notify_parameters_changed(
    const struct dpsfg_plugin_interface* i, struct plugin_ctx* ctx)
{
    if (i->parameters == NULL)
        return;

    i->parameters->on_changed(ctx);
}
static void notify_numeric_changed(
    const struct math_pipeline*          pipeline,
    const struct dpsfg_plugin_interface* i,
    struct plugin_ctx*                   ctx)
{
    if (i->numeric == NULL)
        return;

    i->numeric->on_tf_changed(ctx, &pipeline->tf);
}
void math_pipeline_notify_plugins(
    const struct math_pipeline*          pipeline,
    const struct dpsfg_plugin_interface* plugin_iface,
    struct plugin_ctx*                   plugin_ctx,
    enum math_pipeline_state             state,
    char                                 is_source_plugin)
{
    switch (state)
    {
        case MATH_PIPELINE_GRAPH_CHANGED:
            if (state != MATH_PIPELINE_GRAPH_CHANGED || !is_source_plugin)
            {
                notify_graph_changed(pipeline, plugin_iface, plugin_ctx);
            }
        /* fallthrough */
        case MATH_PIPELINE_SUBSTITUTIONS_CHANGED:
            if (state != MATH_PIPELINE_SUBSTITUTIONS_CHANGED ||
                !is_source_plugin)
            {
                notify_expr_changed(pipeline, plugin_iface, plugin_ctx);
                notify_parameters_changed(plugin_iface, plugin_ctx);
            }
        /* fallthrough */
        case MATH_PIPELINE_PARAMETERS_CHANGED:
            if (state != MATH_PIPELINE_PARAMETERS_CHANGED || !is_source_plugin)
            {
                notify_numeric_changed(pipeline, plugin_iface, plugin_ctx);
            }
            /* fallthrough */
    }
}
