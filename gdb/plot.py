import gdb
import subprocess

catpuccin = dict(
    bgcolor      = "#1e1e2e",
    edgecolor    = "#6c7086",
    gc           = ["doubleoctagon", "#e39aa6", "#e39aa6",],
    literal      = ["record",        "#fab387", "#fab387",],
    variable     = ["record",        "#b4befe", "#b4befe",],
    inf          = ["record",        "#cba6f7", "#cba6f7",],
    neg          = ["oval",          "#f8f0e3", "#f8f0e3",],
    op           = ["oval",          "#f9e2af", "#f9e2af",],
    func         = ["octagon",       "#89b4fa", "#89b4fa",],
    constant     = ["record",        "#a6e3a1", "#a6e3a1",],
    unused1      = ["circle",        "#89dceb", "#89dceb",],
    unused2      = ["record",        "#cba6f7", "#cba6f7",],
    unused3      = ["house",         "#ecea8d", "#ecea8d",],
)
dark_nightfly = dict(
    bgcolor      = "#011627",
    edgecolor    = "#8792a7",
    gc           = ["doubleoctagon", "#f95772", "#f95772",],
    literal      = ["record",        "#f78c6c", "#f78c6c",],
    variable     = ["record",        "#b0b2f4", "#b0b2f4",],
    inf          = ["record",        "#21c7a8", "#21c7a8",],
    neg          = ["oval",          "#ecc48d", "#ecc48d",],
    op           = ["oval",          "#ecea8d", "#ecea8d",],
    func         = ["octagon",       "#82aaff", "#82aaff",],
    constant     = ["record",        "#a57dc9", "#a57dc9",],
    unused1      = ["octagon",       "#e39aa6", "#e39aa6",],
    unused2      = ["doubleoctagon", "#82aaff", "#82aaff",],
    unused3      = ["box3d",         "#c3ccdc", "#c3ccdc",],
)

style = catpuccin

def plot_expr_graph(pool, root):
    def get_string(pool, i):
        strlist = pool["var_names"].dereference()
        data = strlist["data"].address
        span_table = gdb.Value(data + int(strlist["capacity"]))\
            .cast(gdb.lookup_type("struct strspan").pointer())
        span = gdb.Value(span_table - i - 1).dereference()
        string = gdb.Value(data + span["off"])\
            .cast(gdb.lookup_type("char").pointer())\
            .string(length=span["len"])
        return string
    def print_node_style(n, node_style, name):
        nonlocal text
        s = style[node_style]
        text += f'  n{n} [label="[{n}] {name}", shape="{s[0]}", color="{s[1]}", fontcolor="{s[2]}"];\n'
    def print_node(n):
        node = pool["nodes"][n]
        if node["type"] == 0:
            print_node_style(n, "gc", "gc")
        if node["type"] == 1:
            print_node_style(n, "literal", node['value']['lit'])
        if node["type"] == 2:
            i = node["value"]["var_idx"]
            print_node_style(n, "variable", get_string(pool, i))
        if node["type"] == 3:
            print_node_style(n, "inf", "oo")
        if node["type"] == 4:
            print_node_style(n, "neg", "neg")
        if node["type"] == 5:
            print_node_style(n, "op", "+")
        if node["type"] == 6:
            print_node_style(n, "op", "*")
        if node["type"] == 7:
            print_node_style(n, "op", "^")
    def print_edge(n):
        nonlocal text
        left = pool["nodes"][n]["child"][0]
        right = pool["nodes"][n]["child"][1]
        if left != -1:
            text += f'  n{n} -> n{left} [color="{style['edgecolor']}", fontcolor="{style['edgecolor']}"];\n'
        if right != -1:
            text += f'  n{n} -> n{right} [color="{style['edgecolor']}", fontcolor="{style['edgecolor']}"];\n'
    def print_edges(n):
        print_edge(n)
        left = pool["nodes"][n]["child"][0]
        right = pool["nodes"][n]["child"][1]
        if left != -1:
            print_edges(left)
        if right != -1:
            print_edges(right)
    def print_nodes(n):
        nonlocal unvisited
        unvisited.remove(int(n))
        left = pool["nodes"][n]["child"][0]
        right = pool["nodes"][n]["child"][1]
        if left != -1:
            print_nodes(left)
        if right != -1:
            print_nodes(right)
        print_node(n)
    text = "digraph {\n"
    text += f'  bgcolor="{style['bgcolor']}";\n'
    if root is not None:
        unvisited = set(range(pool["count"]))
        print_nodes(root)
        print_edges(root)
        for n in unvisited:
            print_node(n)
    else:
        for n in range(pool["count"]):
            print_node(n)
            print_edge(n)
    text += "}\n"
    p = subprocess.Popen(
        ["dot", "-Tx11"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True)
    p.stdin.write(text)
    p.stdin.close()

class Plot(gdb.Command):
    def __init__(self):
        super(Plot, self).__init__("plot", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        handle = None
        exprs = arg.split()
        values = (gdb.parse_and_eval(x) for x in exprs)
        it = iter(enumerate(values))
        for i, val in it:
            if str(val.type).endswith("csfg_expr_pool **"):
                val = val.dereference()
            if str(val.type).endswith("csfg_expr_pool *"):
                val = val.dereference()
            if str(val.type).endswith("csfg_expr_pool"):
                pool = val
                i, val = next(it, (None, None))
                plot_expr_graph(pool, val)

Plot()
