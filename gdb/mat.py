import gdb
from util import float_to_str, complex_to_str

class mat_PrettyPrinter:
    def __init__(self, mat):
        self.mat = mat

    def to_string(self):
        if int(self.mat) == 0:
            return "mat(0)"
        mat = self.mat.dereference()
        rows = mat["rows"]
        cols = mat["cols"]
        data = mat["data"]
        s = f"mat(rows={rows}, cols={cols}\n"
        for r in range(rows):
            for c in range(cols):
                real = data[r*cols + c]["real"]
                imag = data[r*cols + c]["imag"]
                s += " " +  complex_to_str(data[r*cols+c])
            s += "\n"
        s += ")"
        return s

