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
    text = "digraph {\n"
    for n in range(pool["count"]):
        node = pool["nodes"][n]
        if node["type"] == 0:
            text += f'  n{n} [label="{node['value']['lit']}"];\n'
        if node["type"] == 1:
            i = node["value"]["var_idx"]
            text += f'  n{n} [label="{get_string(pool, i)}"];\n'
        if node["type"] == 2:
            text += f'  n{n} [label="oo"];\n'
        if node["type"] == 3:
            text += f'  n{n} [label="+"];\n'
        if node["type"] == 4:
            text += f'  n{n} [label="-"];\n'
        if node["type"] == 5:
            text += f'  n{n} [label="*"];\n'
        if node["type"] == 6:
            text += f'  n{n} [label="/"];\n'
        if node["type"] == 7:
            text += f'  n{n} [label="^"];\n'
    def print_edges(n):
        nonlocal text
        left = pool["nodes"][n]["child"][0]
        right = pool["nodes"][n]["child"][1]
        if left != -1:
            text += f'  n{n} -> n{left};\n'
            print_edges(left)
        if right != -1:
            text += f'  n{n} -> n{right};\n'
            print_edges(right)
    print_edges(root)
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
