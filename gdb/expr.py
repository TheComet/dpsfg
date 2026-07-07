import gdb
from util import float_to_str

def expr_to_str(pool, val):
    # todo
    return f"expr({val})"

def poly_to_str(pool, val):
    if int(val) == 0:
        return "0"
    val = val.dereference()
    degree = val["count"]
    array = val["data"]
    terms = list()
    for n in range(degree):
        factor = array[n]["factor"]
        if float(factor) == 0.0:
            continue
        expr = array[n]["expr"]
        factors = list()
        if float(factor) != 1.0:
            factors.append(f"{factor}")
        if n == 1:
            factors.append("s")
        elif n > 1:
            factors.append(f"s^{n}")
        if int(expr) > -1:
            factors.append(f"{expr_to_str(pool, expr)}")
        terms.append("*".join(factors))
    return " + ".join(terms)

def tf_to_str(pool, val):
    num = poly_to_str(pool, val["num"])
    den = poly_to_str(pool, val["den"])
    l = max(len(num), len(den))
    return "\n".join((num, '-'*l, den))

class Expr(gdb.Command):
    def __init__(self):
        super(Expr, self).__init__("expr", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        exprs = arg.split()
        values = (gdb.parse_and_eval(x) for x in exprs)

        it = iter(enumerate(values))
        i, val = next(it)

        pool = val
        if str(pool.type).endswith("csfg_expr_pool **"):
            pool = pool.dereference()
        if not str(pool.type).endswith("csfg_expr_pool *"):
            raise gdb.GdbError(f"First argument expected to be a csfg_expr_pool type, not {pool.type}")

        i, val = next(it)
        if val.type.code == gdb.TYPE_CODE_INT:
            print(expr_to_str(pool, val))
            return

        if str(val.type).endswith("csfg_poly_expr *"):
            print(poly_to_str(pool, val))
            return

        if str(val.type).endswith("csfg_tf_expr *"):
            val = val.dereference()
        if str(val.type).endswith("csfg_tf_expr"):
            print(tf_to_str(pool, val))
            return

        raise gdb.GdbError(f"Don't know how to print type {val.type}")

Expr()
