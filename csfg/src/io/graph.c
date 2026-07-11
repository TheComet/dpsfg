#include "csfg/graph/graph.h"
#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"

/* -------------------------------------------------------------------------- */
int csfg_io_graph_save(
    struct serializer** ser,
    const struct csfg_graph* graph,
    int node_in,
    int node_out)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    const uint8_t version = 0;
    int err               = 0;

    err += serialize_u8(ser, version);

    err += serialize_li16(ser, node_in);
    err += serialize_li16(ser, node_out);

    err += serialize_li16(ser, csfg_graph_node_count(graph));
    csfg_graph_for_each_node (graph, n)
    {
        err += serialize_cstr(ser, str_cstr(n->name));
        err += serialize_li16(ser, n->x);
        err += serialize_li16(ser, n->y);
    }

    err += serialize_li16(ser, csfg_graph_edge_count(graph));
    csfg_graph_for_each_edge (graph, e)
    {
        err += serialize_li16(ser, e->n_idx_from);
        err += serialize_li16(ser, e->n_idx_to);
        err += serialize_li16(ser, e->x);
        err += serialize_li16(ser, e->y);
        err += csfg_io_expr_save(ser, e->pool, e->expr);
    }

    return err;
}

/* -------------------------------------------------------------------------- */
static int load_0(
    struct deserializer* des,
    struct csfg_graph* graph,
    int* node_in,
    int* node_out)
{
    int i, node_count, edge_count;

    *node_in  = deserialize_li16(des);
    *node_out = deserialize_li16(des);

    node_count = deserialize_li16(des);
    if (node_count < 0 || node_count > 10000)
        return log_err("Read implausible node count: %d\n", node_count);
    for (i = 0; i != node_count; ++i)
    {
        const char* name;
        int n_idx, x, y;

        name = deserialize_cstr(des);
        x    = deserialize_li16(des);
        y    = deserialize_li16(des);

        n_idx = csfg_graph_add_node(graph, name);
        if (n_idx < 0)
            return -1;
        csfg_graph_get_node(graph, n_idx)->x = x;
        csfg_graph_get_node(graph, n_idx)->y = y;
    }

    edge_count = deserialize_li16(des);
    if (edge_count < 0 || edge_count > 10000)
        return log_err("Read implausible node count: %d\n", edge_count);
    for (i = 0; i != edge_count; ++i)
    {
        struct csfg_expr_pool* pool;
        int expr, e_idx, n_idx_from, n_idx_to, x, y;

        csfg_expr_pool_init(&pool);

        n_idx_from = deserialize_li16(des);
        n_idx_to   = deserialize_li16(des);
        x          = deserialize_li16(des);
        y          = deserialize_li16(des);

        if (csfg_io_expr_load(des, &pool, &expr) != 0)
            goto fail;

        e_idx = csfg_graph_add_edge(graph, n_idx_from, n_idx_to, pool, expr);
        if (e_idx < 0)
            goto fail;

        csfg_graph_get_edge(graph, e_idx)->x = x;
        csfg_graph_get_edge(graph, e_idx)->y = y;
        continue;

    fail:
        csfg_expr_pool_deinit(pool);
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_io_graph_load(
    struct deserializer* des,
    struct csfg_graph* graph,
    int* node_in,
    int* node_out)
{
    uint8_t version = deserialize_u8(des);
    if (deserializer_err(des))
        return log_err("Failed to read version: EOF\n");

    csfg_graph_clear(graph);
    switch (version)
    {
        case 0x00: return load_0(des, graph, node_in, node_out);
    }

    return log_err("Unsupported format version: %d\n", version);
}
