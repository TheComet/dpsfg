import gdb

class mat_PrettyPrinter:
    def __init__(self, mat):
        self.mat = mat

    def display_hint(self):
        return "array"

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
                s += " " +  str(real)
                if abs(imag) > 1e-4:
                    s += str(imag) + "j"
            s += "\n"
        s += ")"
        return s

