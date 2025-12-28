def float_to_str(f):
    return str(round(float(f), 3))

def complex_to_str(c):
    s = str()
    real = float(c["real"])
    imag = float(c["imag"])
    if abs(imag) > 1e-4:
        s += "("
    s += float_to_str(real)
    if abs(imag) > 1e-4:
        if imag >= 0:
            s += "+"
        s += float_to_str(imag) + "j)"
    return s
