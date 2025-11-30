#include "csfg/graph/graph.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"

/* -------------------------------------------------------------------------- */
static int save(
    struct serializer**          ser,
    const struct csfg_var_table* substitutions,
    const struct csfg_var_table* parameters,
    const struct csfg_graph*     graph,
    int                          node_in,
    int                          node_out)
{
    (void)ser, (void)substitutions, (void)parameters;
    (void)graph, (void)node_in, (void)node_out;
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
    (void)des, (void)substitutions, (void)parameters;
    (void)graph, (void)node_in, (void)node_out;
    return 0;
}

/* -------------------------------------------------------------------------- */
const struct csfg_io csfg_graph_io_tikz = {"tikz", save, load};
