import gdb
from util import float_to_str, complex_to_str

def root_to_str(p):
    real = float(p["real"])
    imag = float(p["imag"])

    s = "s"
    if real < 0:
        s += "+"
    else:
        s += "-"

    if abs(imag) > 1e-4:
        s += "("

    if real < 0:
        s += float_to_str(-real)
    else:
        s += float_to_str(real)

    if abs(imag) > 1e-4:
        if imag >= 0.0:
            s += "+"
        s += float_to_str(imag) + "j"

    return s

class cpoly_PrettyPrinter:
    def __init__(self, p):
        self.p = p

    def to_string(self):
        if int(self.p) == 0:
            return "0"
        p = self.p.dereference()
        count = p["count"]
        data = p["data"]
        if int(count) == 0:
            return "0"

        s = ""
        for i in range(count):
            if i > 0:
                s += " + "
            s += complex_to_str(data[i])
            if i > 0:
                s += "s"
            if i > 1:
                s += f"^{i}"
        return s

class rpoly_PrettyPrinter:
    def __init__(self, p):
        self.p = p

    def to_string(self):
        if int(self.p) == 0:
            return "0"
        p = self.p.dereference()
        count = p["count"]
        data = p["data"]
        if int(count) == 0:
            return "0"

        s = ""
        for i in range(count):
            s += "("
            s += root_to_str(data[i])
            s += ")"
        return s


class pfd_poly_PrettyPrinter:
    def __init__(self, p):
        self.p = p

    def to_string(self):
        if int(self.p) == 0:
            return "0"
        p = self.p.dereference()
        count = p["count"]
        data = p["data"]
        if int(count) == 0:
            return "0"

        s = ""
        for i in range(count):
            if i > 0:
                s += " + "

            term = data[i]
            n = int(term["n"])

            s += complex_to_str(term["A"])
            s += "/("
            s += root_to_str(term["p"])
            s += ")"
            if n > 1:
                s += f"^{n}"
        return s
