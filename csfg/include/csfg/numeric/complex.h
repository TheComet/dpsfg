#pragma once

#include <math.h>

struct csfg_complex
{
    double real;
    double imag;
};

static struct csfg_complex csfg_complex(double real, double imag)
{
    struct csfg_complex result;
    result.real = real;
    result.imag = imag;
    return result;
}

static double csfg_complex_mag(const struct csfg_complex c)
{
    return sqrt(c.real * c.real + c.imag * c.imag);
}

static double csfg_complex_phase(const struct csfg_complex c)
{
    return atan2(c.imag, c.real);
}

static struct csfg_complex
csfg_complex_add(struct csfg_complex a, struct csfg_complex b)
{
    return csfg_complex(a.real + b.real, a.imag + b.imag);
}

static struct csfg_complex
csfg_complex_sub(struct csfg_complex a, struct csfg_complex b)
{
    return csfg_complex(a.real - b.real, a.imag - b.imag);
}

static struct csfg_complex
csfg_complex_mul(struct csfg_complex a, struct csfg_complex b)
{
    /* (a+b*i) * (c+d*i) = (a*c - b*d) + (a*d + c*b)*i */
    return csfg_complex(
        a.real * b.real - a.imag * b.imag, /* make clang-format look nicer */
        a.real * b.imag + b.real * a.imag);
}

static struct csfg_complex
csfg_complex_div(struct csfg_complex a, struct csfg_complex b)
{
    /* a+b*i   (a+b*i)(c-d*i)   a*c + b*d   b*c - a*d
     * ----- = -------------- = --------- + ---------i
     * c+d*i     c^2 - d^2      c^2 - d^2   c^2 - d^2
     */
    double den = b.real * b.real + b.imag * b.imag;
    return csfg_complex(
        (a.real * b.real + a.imag * b.imag) / den,
        (a.imag * b.real - a.real * b.imag) / den);
}
