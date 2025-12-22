#include "csfg/graph/graph.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"
#include <limits.h>
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
                case 'f': {
                    char buf[sizeof("-1.234e-123")];
                    sprintf(buf, "%.4g", va_arg(ap, double));
                    err += serialize_data(ser, buf, strlen(buf));
                    break;
                }
                case 's': {
                    const char* str = va_arg(ap, char*);
                    err += serialize_data(ser, str, strlen(str));
                    break;
                }
                case '%': {
                    err += serialize_data(ser, "%", 1);
                    break;
                }
                default: CSFG_DEBUG_ASSERT(0); return -1;
            }

    va_end(ap);
    return err;
}

/* -------------------------------------------------------------------------- */
static int write_style(struct serializer** ser)
{
    int err = 0;
    /* clang-format off */
    err += serialize_fmt(ser, "%% Put this into your preamble\n");
    err += serialize_fmt(ser, "\\usepackage{tikz}\n");
    err += serialize_fmt(ser, "\\usetikzlibrary{decorations.markings,calc,arrows}\n");
    err += serialize_fmt(ser,"tikzset{%%\n");
    err += serialize_fmt(ser, "SFGNode/.style={circle,thick,draw=black,inner sep=0, minimum size=0.15cm},\n");
    err += serialize_fmt(ser, "SFGNodeName/.style={font=\footnotesize,black,outer sep=1},\n");
    err += serialize_fmt(ser, "SFGArrowName/.style={font=\footnotesize,auto,outer sep=1},\n");
    err += serialize_fmt(ser, "SFGBranch/.style={thick},\n");
    err += serialize_fmt(ser, "->-/.style={decoration={\n");
    err += serialize_fmt(ser, "    markings,\n");
    err += serialize_fmt(ser, "    mark=at position #1 with {\\draw[->,>=latex',line width=2pt](0pt,0)--(4pt,0);}},\n");
    err += serialize_fmt(ser, "  postaction={decorate}},\n");
    err += serialize_fmt(ser, "->-/.default=0.50,\n");
    err += serialize_fmt(ser, "SFGMarkMiddle/.style={decoration={\n");
    err += serialize_fmt(ser, "    markings,\n");
    err += serialize_fmt(ser, "    mark=at position 0.5 with {\node[SFGNodeName](middle_#1){};}},\n");
    err += serialize_fmt(ser, "  postaction={decorate}},\n");
    /* clang-format on */
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
    int                     n_idx, e_idx;
    double                  scale;
    struct str*             str;

    int err = 0;
    str_init(&str);

    err += write_style(ser);

    /* This seems to make it look correct-ish */
    scale = 1.0 / 20 / 3;

    err += serialize_fmt(ser, "\\begin{tikzpicture}[scale=1]\n");

    csfg_graph_enumerate_nodes (graph, n_idx, node)
    {
        err += serialize_fmt(
            ser,
            "  \\node[SFGNode] (n%d) at (%f,%f) {};\n",
            n_idx,
            node->x * scale,
            node->y * -scale);
    }
    csfg_graph_enumerate_nodes (graph, n_idx, node)
    {
        err += serialize_fmt(
            ser,
            "  \\node[SFGNodeName] at ($(n%d) + (0,%f)$) {$%s$};\n",
            n_idx,
            -15 * scale,
            str_cstr(node->name));
    }

    csfg_graph_enumerate_edges (graph, e_idx, edge)
    {
        err += serialize_fmt(
            ser,
            "  \\draw[->-, SFGBranch, SFGMarkMiddle=%d] (n%d) .. controls "
            "(%f,%f) .. (n%d);\n",
            e_idx,
            edge->n_idx_from,
            edge->x * scale,
            edge->y * -scale,
            edge->n_idx_to);
    }
    csfg_graph_enumerate_edges (graph, e_idx, edge)
    {
        str_clear(str);
        csfg_expr_to_str(&str, edge->pool, edge->expr);
        err += serialize_fmt(
            ser,
            "  \\node[SFGArrowName] at($(middle_%d) + (0,%f)$) {$%s$};\n",
            e_idx,
            15 * scale,
            str_cstr(str));
    }

    err += serialize_fmt(ser, "\\end{tikzpicture}\n");

    str_deinit(str);
    return err;

    (void)ser, (void)substitutions, (void)parameters;
    (void)graph, (void)node_in, (void)node_out;
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
