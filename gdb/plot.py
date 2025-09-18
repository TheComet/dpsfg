import gdb

class Plot(gdb.Command):
    def __init__(self):
        super(Plot, self).__init__("plot", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        handle = None
        exprs = arg.split()
        values = (gdb.parse_and_eval(x) for x in exprs)
        for i, val in enumerate(values):
            if str(val.type).endswith("csfg_expr *"):
                val = val.dereference()
            if str(val.type).endswith("csfg_expr"):
                pass

Plot()
