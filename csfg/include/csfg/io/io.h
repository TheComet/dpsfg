#pragma once

struct csfg_graph;
struct csfg_var_table;
struct deserializer;
struct serializer;

struct csfg_io
{
    const char* format;

    int (*save)(
        struct serializer**          ser,
        const struct csfg_var_table* substitutions,
        const struct csfg_var_table* parameters,
        const struct csfg_graph*     graph,
        int                          node_in,
        int                          node_out);

    int (*load)(
        struct deserializer*   des,
        struct csfg_var_table* substitutions,
        struct csfg_var_table* parameters,
        struct csfg_graph*     graph,
        int*                   node_in,
        int*                   node_out);
};

int csfg_io_save(
    struct serializer**          ser,
    const struct csfg_var_table* substitutions,
    const struct csfg_var_table* parameters,
    const struct csfg_graph*     graph,
    int                          node_in,
    int                          node_out,
    const char*                  format);

int csfg_io_load(
    struct deserializer*   des,
    struct csfg_var_table* substitutions,
    struct csfg_var_table* parameters,
    struct csfg_graph*     graph,
    int*                   node_in,
    int*                   node_out,
    const char*            format);
