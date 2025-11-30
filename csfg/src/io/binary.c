#include "csfg/graph/graph.h"
#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/str.h"

/* -------------------------------------------------------------------------- */
static int
serialize_expr_pool(struct serializer** ser, const struct csfg_expr_pool* pool)
{
    int n;
    int err = 0;

    err += serialize_li16(ser, pool->count);
    for (n = 0; n != pool->count; ++n)
    {
        err += serialize_u8(ser, pool->nodes[n].type);
        err += serialize_li16(ser, pool->nodes[n].child[0]);
        err += serialize_li16(ser, pool->nodes[n].child[1]);

        switch (pool->nodes[n].type)
        {
            case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); return -1;
            case CSFG_EXPR_LIT:
                err += serialize_lf32(ser, pool->nodes[n].value.lit);
                break;
            case CSFG_EXPR_VAR:
                err += serialize_cstr(
                    ser,
                    strlist_cstr(
                        pool->var_names, pool->nodes[n].value.var_idx));
                break;
            case CSFG_EXPR_INF:
            case CSFG_EXPR_NEG:
            case CSFG_EXPR_ADD:
            case CSFG_EXPR_MUL:
            case CSFG_EXPR_POW: break;
        }
    }

    return err;
}
static int serialize_expr(
    struct serializer** ser, const struct csfg_expr_pool* pool, int expr)
{
    int err = 0;
    err += serialize_li16(ser, expr);
    if (expr > -1)
        err += serialize_expr_pool(ser, pool);

    return err;
}

/* -------------------------------------------------------------------------- */
static int deserialize_expr(
    struct deserializer* des, struct csfg_expr_pool** pool, int* expr)
{
    int n;
    int node_count;

    *expr = deserialize_li16(des);
    if (deserializer_err(des))
        return log_err("Failed to read expression: EOF\n");

    if (*expr == -1)
        return 0;

    node_count = deserialize_li16(des);
    if (node_count < 0 || node_count > 1000)
        return log_err("Read an implausable node count: %d\n", node_count);

    for (n = 0; n != node_count; ++n)
    {
        enum csfg_expr_type type = deserialize_u8(des);
        int                 left = deserialize_li16(des);
        int                 right = deserialize_li16(des);

        switch (type)
        {
            case CSFG_EXPR_GC: break;

            case CSFG_EXPR_LIT:
                if (csfg_expr_lit(pool, deserialize_lf32(des)) < 0)
                    return -1;
                if (left != -1 || right != -1)
                    log_warn(
                        "Invalid left/right indices read: left=%d, right=%d\n",
                        left,
                        right);
                continue;
            case CSFG_EXPR_VAR:
                if (csfg_expr_var(pool, cstr_view(deserialize_cstr(des))) < 0)
                    return -1;
                if (left != -1 || right != -1)
                    log_warn(
                        "Invalid left/right indices read: left=%d, right=%d\n",
                        left,
                        right);
                continue;
            case CSFG_EXPR_INF:
                if (csfg_expr_inf(pool) < 0)
                    return -1;
                if (left != -1 || right != -1)
                    log_warn(
                        "Invalid left/right indices read: left=%d, right=%d\n",
                        left,
                        right);
                continue;

            case CSFG_EXPR_NEG:
                if (csfg_expr_neg(pool, left) < 0)
                    return -1;
                if (right != -1)
                    log_warn(
                        "Invalid left/right indices read: left=%d, right=%d\n",
                        left,
                        right);
                continue;

            case CSFG_EXPR_ADD:
            case CSFG_EXPR_MUL:
            case CSFG_EXPR_POW:
                if (csfg_expr_binop(pool, type, left, right) < 0)
                    return -1;
                continue;
        }

        return log_err("Invalid node type read: %d\n", type);
    }

    if (csfg_expr_integrity_check(*pool, *expr) != 0)
        return -1;

    return 0;
}

/* -------------------------------------------------------------------------- */
static int
serialize_var_table(struct serializer** ser, const struct csfg_var_table* vt)
{
    int16_t                      slot;
    struct str*                  name;
    struct csfg_var_table_entry* entry;

    int err = 0;

    err += serialize_li16(ser, hmap_count(vt->map));
    hmap_for_each (vt->map, slot, name, entry)
    {
        err += serialize_cstr(ser, str_cstr(name));
        err += serialize_expr(ser, entry->pool, entry->expr);
    }

    return err;
}

/* -------------------------------------------------------------------------- */
static int
deserialize_var_table(struct deserializer* des, struct csfg_var_table* vt)
{
    int16_t entry_count = deserialize_li16(des);
    if (entry_count > 1000)
        return log_err(
            "Variable table entry count is implausible: %d\n", entry_count);

    while (entry_count-- > 0)
    {
        struct csfg_expr_pool* pool;
        int                    expr;
        const char*            name;

        csfg_expr_pool_init(&pool);

        name = deserialize_cstr(des);
        if (deserialize_expr(des, &pool, &expr) != 0)
            goto fail;

        if (csfg_var_table_set_expr(vt, cstr_view(name), pool, expr) != 0)
            goto fail;

        continue;

    fail:
        csfg_expr_pool_deinit(pool);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int save(
    struct serializer**          ser,
    const struct csfg_var_table* substitutions,
    const struct csfg_var_table* parameters,
    const struct csfg_graph*     graph,
    int                          node_in,
    int                          node_out)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;

    const char     magic[4] = {'C', 'S', 'F', 'G'};
    const uint16_t version = 0x0000;
    int            err = 0;

    err += serialize_data(ser, magic, 4);
    err += serialize_lu16(ser, version);

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
        err += serialize_expr(ser, e->pool, e->expr);
    }

    err += serialize_var_table(ser, substitutions);
    err += serialize_var_table(ser, parameters);

    return err;
}

/* -------------------------------------------------------------------------- */
static int load_0_0(
    struct deserializer*   des,
    struct csfg_var_table* substitutions,
    struct csfg_var_table* parameters,
    struct csfg_graph*     graph,
    int*                   node_in,
    int*                   node_out)
{
    int i, node_count, edge_count;

    *node_in = deserialize_li16(des);
    *node_out = deserialize_li16(des);

    node_count = deserialize_li16(des);
    if (node_count > 10000)
        return log_err("Read implausible node count: %d\n", node_count);
    for (i = 0; i != node_count; ++i)
    {
        const char* name;
        int         n_idx, x, y;

        name = deserialize_cstr(des);
        x = deserialize_li16(des);
        y = deserialize_li16(des);

        n_idx = csfg_graph_add_node(graph, name);
        if (n_idx < 0)
            return -1;
        csfg_graph_get_node(graph, n_idx)->x = x;
        csfg_graph_get_node(graph, n_idx)->y = y;
    }

    edge_count = deserialize_li16(des);
    if (edge_count > 10000)
        return log_err("Read implausible node count: %d\n", edge_count);
    for (i = 0; i != edge_count; ++i)
    {
        struct csfg_expr_pool* pool;
        int                    expr, e_idx, n_idx_from, n_idx_to, x, y;

        csfg_expr_pool_init(&pool);

        n_idx_from = deserialize_li16(des);
        n_idx_to = deserialize_li16(des);
        x = deserialize_li16(des);
        y = deserialize_li16(des);

        if (deserialize_expr(des, &pool, &expr) != 0)
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

    if (deserialize_var_table(des, substitutions) != 0)
        return -1;
    if (deserialize_var_table(des, parameters) != 0)
        return -1;

    return 0;
}

/* -------------------------------------------------------------------------- */
static int load(
    struct deserializer*   des,
    struct csfg_var_table* substitutions,
    struct csfg_var_table* parameters,
    struct csfg_graph*     graph,
    int*                   node_in,
    int*                   node_out)
{
    char     magic[4];
    uint16_t version;
    int      result;

    deserialize_data(des, magic, 4);
    if (deserializer_err(des) || memcmp(magic, "CSFG", 4) != 0)
        return log_err("Invalid header\n");

    version = deserialize_lu16(des);
    if (deserializer_err(des))
        return log_err("Failed to read version: EOF\n");

    csfg_var_table_clear(substitutions);
    csfg_var_table_clear(parameters);
    csfg_graph_clear(graph);

    switch (version)
    {
        case 0x0000:
            result = load_0_0(
                des, substitutions, parameters, graph, node_in, node_out);
            break;

        default:
            result = log_err(
                "Unsupported format version: %d.%d\n",
                version >> 8,
                version & 0xFF);
    }

    if (deserializer_err(des))
        result = log_err("Reached EOF while reading SFG binary\n");

    if (result != 0)
    {
        csfg_var_table_clear(substitutions);
        csfg_var_table_clear(parameters);
        csfg_graph_clear(graph);
        *node_in = -1;
        *node_out = -1;
    }

    return result;
}

/* -------------------------------------------------------------------------- */
const struct csfg_io csfg_graph_io_binary = {"binary", save, load};
