#include "csfg/io/io.h"
#include "csfg/util/log.h"
#include <string.h>

extern const struct csfg_io csfg_graph_io_binary;
extern const struct csfg_io csfg_graph_io_graphviz;
extern const struct csfg_io csfg_graph_io_tikz;

const struct csfg_io* io[] = {
    &csfg_graph_io_binary,
    &csfg_graph_io_graphviz,
    &csfg_graph_io_tikz,
    NULL,
    NULL};

/* -------------------------------------------------------------------------- */
int csfg_io_save(
    struct serializer**          ser,
    const struct csfg_var_table* substitutions,
    const struct csfg_var_table* parameters,
    const struct csfg_graph*     graph,
    int                          node_in,
    int                          node_out,
    const char*                  format)
{
    int i;

    for (i = 0; io[i] != NULL; ++i)
    {
        if (strcmp(format, io[i]->format) == 0)
            return io[i]->save(
                ser, substitutions, parameters, graph, node_in, node_out);
    }

    return log_err("Unsupported format: %s\n", format);
}

/* -------------------------------------------------------------------------- */
int csfg_io_load(
    struct deserializer*   des,
    struct csfg_var_table* substitutions,
    struct csfg_var_table* parameters,
    struct csfg_graph*     graph,
    int*                   node_in,
    int*                   node_out,
    const char*            format)
{
    int i;
    for (i = 0; io[i] != NULL; ++i)
    {
        if (strcmp(format, io[i]->format) == 0)
            return io[i]->load(
                des, substitutions, parameters, graph, node_in, node_out);
    }

    return log_err("Unsupported format: %s\n", format);
}
