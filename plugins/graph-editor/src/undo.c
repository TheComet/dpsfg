#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "graph-editor/attr.h"
#include "graph-editor/drawing.h"
#include "graph-editor/graph_helpers.h"
#include "graph-editor/graph_model.h"
#include "graph-editor/undo.h"

VEC_DEFINE(undo_stack_vec, struct serializer*, 32)

/* -------------------------------------------------------------------------- */
int undo_push_state(struct graph_model* model)
{
    struct serializer** ser;

    /* destroy future */
    while (vec_count(model->undo_stack) - 1 > model->undo_stack_ptr)
        serializer_deinit(*undo_stack_vec_pop(model->undo_stack));

    ser = undo_stack_vec_emplace(&model->undo_stack);
    if (ser == NULL)
        return -1;
    serializer_init(ser);

    if (csfg_io_graph_save(
            ser,
            model->graph,
            find_node_idx(model->graph, model->node_in_id),
            find_node_idx(model->graph, model->node_out_id)) != 0 ||
        attrs_save(ser, model->node_attrs, model->edge_attrs, model->graph) !=
            0 ||
        drawing_save(ser, model->drawing, model->graph) != 0)
    {
        serializer_deinit(*undo_stack_vec_pop(model->undo_stack));
        return -1;
    }

    model->undo_stack_ptr++;
    return 0;
}

/* -------------------------------------------------------------------------- */
int undo(struct graph_model* model)
{
    int node_in, node_out;
    struct serializer* ser;
    struct deserializer des;

    if (model->undo_stack_ptr > 0)
    {
        ser = *vec_get(model->undo_stack, model->undo_stack_ptr - 1);
        des = deserializer(vec_data(ser), vec_count(ser));
        if (csfg_io_graph_load(&des, model->graph, &node_in, &node_out) != 0)
            return -1;
        graph_model_rebuild_graph(model, node_in, node_out);
        if (attrs_load(
                &des, &model->node_attrs, &model->edge_attrs, model->graph) !=
            0)
            return -1;
        if (drawing_load(&des, &model->drawing, model->graph) != 0)
            return -1;
        model->undo_stack_ptr--;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int redo(struct graph_model* model)
{
    int node_in, node_out;
    struct serializer* ser;
    struct deserializer des;

    if (model->undo_stack_ptr + 1 < vec_count(model->undo_stack))
    {
        ser = *vec_get(model->undo_stack, model->undo_stack_ptr + 1);
        des = deserializer(vec_data(ser), vec_count(ser));
        if (csfg_io_graph_load(&des, model->graph, &node_in, &node_out) != 0)
            return -1;
        graph_model_rebuild_graph(model, node_in, node_out);
        if (attrs_load(
                &des, &model->node_attrs, &model->edge_attrs, model->graph) !=
            0)
            return -1;
        if (drawing_load(&des, &model->drawing, model->graph) != 0)
            return -1;
        model->undo_stack_ptr++;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int undo_save_stack(struct serializer** ser, const struct graph_model* model)
{
    struct serializer* const* state;
    int err = 0;

    if (model->graph == NULL)
        return 0;

    err += serialize_lu16(ser, vec_count(model->undo_stack));
    err += serialize_lu16(ser, model->undo_stack_ptr);
    vec_for_each (model->undo_stack, state)
    {
        err += serialize_lu16(ser, vec_count(*state));
        err += serialize_data(ser, vec_data(*state), vec_count(*state));
    }

    return err;
}

/* -------------------------------------------------------------------------- */
int undo_load_stack(struct deserializer* des, struct graph_model* model)
{
    int state_count, state_size;
    struct serializer** state;

    if (model->graph == NULL)
        return 0;

    undo_clear_stack(model);

    state_count           = deserialize_lu16(des);
    model->undo_stack_ptr = deserialize_lu16(des);
    while (state_count-- > 0)
    {
        state_size = deserialize_lu16(des);
        state      = undo_stack_vec_emplace(&model->undo_stack);
        if (state == NULL)
            return -1;
        serializer_init(state);
        if (serializer_realloc(state, state_size) != 0)
            return -1;
        deserialize_data2(des, vec_data(*state), state_size);
        (*state)->count = state_size;
    }

    if (model->undo_stack_ptr > vec_count(model->undo_stack) - 1)
        model->undo_stack_ptr = vec_count(model->undo_stack) - 1;

    return 0;
}

/* -------------------------------------------------------------------------- */
void undo_clear_stack(struct graph_model* model)
{
    while (vec_count(model->undo_stack))
        serializer_deinit(*undo_stack_vec_pop(model->undo_stack));
    model->undo_stack_ptr = -1;
}

/* -------------------------------------------------------------------------- */
void undo_reinit_stack(struct graph_model* model)
{
    undo_clear_stack(model);
    undo_push_state(model);
}
