#pragma once

struct csfg_graph;
struct csfg_node;
struct csfg_edge;
struct csfg_var_table;
struct csfg_expr_pool;
struct deserializer;
struct serializer;

int csfg_io_expr_save(
    struct serializer** ser, const struct csfg_expr_pool* pool, int expr);
int csfg_io_expr_load(
    struct deserializer* des, struct csfg_expr_pool** pool, int* expr);

int csfg_io_graph_save(
    struct serializer** ser,
    const struct csfg_graph* graph,
    int node_in,
    int node_out);
int csfg_io_graph_load(
    struct deserializer* des,
    struct csfg_graph* graph,
    int* node_in,
    int* node_out);

int csfg_io_var_table_save(
    struct serializer** ser, const struct csfg_var_table* vt);
int csfg_io_var_table_load(struct deserializer* des, struct csfg_var_table* vt);
