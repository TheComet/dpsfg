import gdb

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
            real = data[i]["real"]
            imag = data[i]["imag"]
            s += str(real)
            if abs(imag) > 1e-4:
                s += str(imag) + "j"
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
            s += "(s"
            real = data[i]["real"]
            imag = data[i]["imag"]
            if float(real) < 0:
                s += f"+{-float(real)}"
            else:
                s += f"-{real}"
            if abs(imag) > 1e-4:
                s += str(imag) + "j"
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
            term = data[i]
            A = term["A"]
            p = term["p"]
            n = int(term["n"])

            real = A["real"]
            imag = A["imag"]
            s += str(real)
            if abs(imag) > 1e-4:
                s += str(imag) + "j"
            s += "/("

            real = p["real"]
            imag = p["imag"]
            s += str(real)
            if abs(imag) > 1e-4:
                s += str(imag) + "j"
            if n > 1:
                s += f"^{n}"
        return s
