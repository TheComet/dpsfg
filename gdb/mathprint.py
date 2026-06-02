import gdb
from util import float_to_str

EXPR_GC = 0
EXPR_LIT = 1
EXPR_VAR = 2
EXPR_INF = 3
EXPR_NEG = 4
EXPR_ADD = 5
EXPR_MUL = 6
EXPR_POW = 7

def expr_to_str(pool, expr):
    node = pool["nodes"][expr]
    tp = node["type"]
    if tp == EXPR_GC:
        return "!!GC!!"
    if tp == EXPR_LIT:
        return float_to_str(float(node["value"]["lit"]))
    if tp == EXPR_VAR:
        var_idx = int(node["value"]["var_idx"])
        strlist = pool["var_names"].dereference()
        data = strlist["data"].address
        span_table = gdb.Value(data + int(strlist["capacity"]))\
            .cast(gdb.lookup_type("struct strspan").pointer())
        span = gdb.Value(span_table - var_idx - 1).dereference()
        string = gdb.Value(data + span["off"])\
            .cast(gdb.lookup_type("char").pointer())\
            .string(length=span["len"])
        return string
    if tp == EXPR_INF:
        return "oo"
    if tp == EXPR_NEG:
        child = int(node["child"][0])
        return "-(" + expr_to_str(pool, child) + ")"
    if tp == EXPR_ADD:
        op = "+"
    elif tp == EXPR_MUL:
        op = "*"
    elif tp == EXPR_POW:
        op = "^"
    left = int(node["child"][0])
    right = int(node["child"][1])
    return "(" + expr_to_str(pool, left) + op + expr_to_str(pool, right) + ")"

def print_expr(pool, expr):
    print(expr_to_str(pool, expr))

def poly_expr_to_str(pool, poly):
    count = int(poly["count"])
    data = poly["data"]
    terms = list()
    for i in range(count):
        coeff = data[i]
        factor = float(coeff["factor"])
        expr = int(coeff["expr"])
        if factor == 0.0:
            continue
        term = float_to_str(factor)
        if expr > -1:
            term = term + "*(" + expr_to_str(pool, expr) + ")"
        if i > 0:
            term = term + "s"
            if i > 1:
                term = term + f"^{i}"
        terms.append(term)
    return " + ".join(reversed(terms))

def print_poly_expr(pool, poly):
    print(poly_expr_to_str(pool, poly))

def print_tf_expr(pool, tf):
    num = tf["num"].dereference()
    den = tf["den"].dereference()
    num = poly_expr_to_str(pool, num)
    den = poly_expr_to_str(pool, den)
    l = len(num) if len(num) > len(den) else len(den)
    print(num)
    print("-" * l)
    print(den)

class MathPrint(gdb.Command):
    def __init__(self):
        super(MathPrint, self).__init__("mathprint", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        exprs = arg.split()
        values = (gdb.parse_and_eval(x) for x in exprs)
        it = iter(values)
        for val in it:
            if str(val.type).endswith("csfg_expr_pool **"):
                val = val.dereference()
            if str(val.type).endswith("csfg_expr_pool *"):
                val = val.dereference()
            if str(val.type).endswith("csfg_expr_pool"):
                pool = val
                val = next(it, None)
                if str(val.type) == "int":
                    print_expr(pool, int(val))
                if str(val.type).endswith("csfg_poly_expr *"):
                    print_poly_expr(pool, val.dereference())
                if str(val.type).endswith("csfg_tf_expr *"):
                    val = val.dereference()
                if str(val.type).endswith("csfg_tf_expr"):
                    print_tf_expr(pool, val)

MathPrint()
