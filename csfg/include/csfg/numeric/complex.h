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

static struct csfg_complex csfg_complex_inf(void)
{
    return csfg_complex(INFINITY, INFINITY);
}

static struct csfg_complex csfg_complex_nan(void)
{
    return csfg_complex(NAN, NAN);
}

static double csfg_complex_mag(const struct csfg_complex c)
{
    return sqrt(c.real * c.real + c.imag * c.imag);
}

static double csfg_complex_phase(const struct csfg_complex c)
{
    return atan2(c.imag, c.real);
}

static int csfg_complex_eq(struct csfg_complex a, struct csfg_complex b)
{
    return a.real == b.real && a.imag == b.imag;
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

static struct csfg_complex csfg_complex_neg(struct csfg_complex c)
{
    return csfg_complex(-c.real, -c.imag);
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

static struct csfg_complex
csfg_complex_pow(struct csfg_complex a, struct csfg_complex b)
{
    /* (a+bi)^(c+di) = exp((c+di) * log(a+bi)) */
    if (a.imag == 0.0 && b.imag == 0.0)
        return csfg_complex(pow(a.real, b.real), 0);
    else
    {
        double r     = hypot(a.real, a.imag);
        double theta = atan2(a.imag, a.real);

        /* log(a) = ln(r) + i*theta */
        double log_re = log(r);
        double log_im = theta;

        /* (c+di) * (log_re + i log_im) */
        double x = b.real * log_re - b.imag * log_im;
        double y = b.real * log_im + b.imag * log_re;

        /* exp(x + iy) = e^x (cos y + i sin y) */
        double expx = exp(x);

        struct csfg_complex out;
        out.real = expx * cos(y);
        out.imag = expx * sin(y);

        return out;
    }
}
