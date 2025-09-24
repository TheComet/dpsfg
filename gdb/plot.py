import gdb
import subprocess

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
    def print_node(n):
        nonlocal text
        node = pool["nodes"][n]
        if node["type"] == 0:
            text += f'  n{n} [label="({n}) deletem"];\n'
        if node["type"] == 1:
            text += f'  n{n} [label="({n}) {node['value']['lit']}"];\n'
        if node["type"] == 2:
            i = node["value"]["var_idx"]
            text += f'  n{n} [label="({n}) {get_string(pool, i)}"];\n'
        if node["type"] == 3:
            text += f'  n{n} [label="({n}) oo"];\n'
        if node["type"] == 4:
            text += f'  n{n} [label="({n}) neg"];\n'
        if node["type"] == 5:
            text += f'  n{n} [label="({n}) +"];\n'
        if node["type"] == 6:
            text += f'  n{n} [label="({n}) *"];\n'
        if node["type"] == 7:
            text += f'  n{n} [label="({n}) ^"];\n'
    def print_edge(n):
        nonlocal text
        left = pool["nodes"][n]["child"][0]
        right = pool["nodes"][n]["child"][1]
        if left != -1:
            text += f'  n{n} -> n{left};\n'
        if right != -1:
            text += f'  n{n} -> n{right};\n'
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
    unvisited = set(range(pool["count"]))
    text = "digraph {\n"
    print_nodes(root)
    print_edges(root)
    for n in unvisited:
        print_node(n)
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
                i, val = next(it)
                plot_expr_graph(pool, val)

Plot()
