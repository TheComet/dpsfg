#include "csfg/graph/graph.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"
#include <stdio.h>

/* -------------------------------------------------------------------------- */
static int serialize_fmt(struct serializer** ser, const char* fmt, ...)
{
    va_list ap;
    int     err = 0;

    va_start(ap, fmt);

    for (; *fmt; ++fmt)
        if (*fmt != '%')
            err += serialize_char(ser, *fmt);
        else
            switch (*++fmt)
            {
                case 'd': {
                    char buf[sizeof("-2147483648")];
                    sprintf(buf, "%d", va_arg(ap, int));
                    err += serialize_data(ser, buf, strlen(buf));
                    break;
                }
                case 's': {
                    const char* str = va_arg(ap, char*);
                    err += serialize_data(ser, str, strlen(str));
                    break;
                }
                default: CSFG_DEBUG_ASSERT(0); return -1;
            }

    va_end(ap);
    return err;
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
    const struct csfg_node* node;
    const struct csfg_edge* edge;
    int                     n_idx;
    struct str*             str;

    int err = 0;
    str_init(&str);

    err += serialize_fmt(ser, "digraph dpsfg {\n");

    csfg_graph_enumerate_nodes (graph, n_idx, node)
    {
        err += serialize_fmt(
            ser,
            "  n%d [label=\"%s\", shape=\"%s\"];\n",
            n_idx,
            str_cstr(node->name),
            n_idx == node_in || n_idx == node_out ? "doublecircle" : "circle");
    }

    csfg_graph_for_each_edge (graph, edge)
    {
        str_clear(str);
        csfg_expr_to_str(&str, edge->pool, edge->expr);
        err += serialize_fmt(
            ser,
            "  n%d -> n%d [label=\"%s\"];\n",
            edge->n_idx_from,
            edge->n_idx_to,
            str_cstr(str));
    }

    err += serialize_fmt(ser, "%s", "}\n");

    str_deinit(str);
    return err;

    (void)substitutions, (void)parameters;
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
    return log_err("Loading Graphviz DOT not implemented\n");
}

/* -------------------------------------------------------------------------- */
const struct csfg_io csfg_graph_io_graphviz = {"graphviz", save, load};
