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

static double csfg_complex_mag(const struct csfg_complex* c)
{
    return sqrt(c->real * c->real + c->imag * c->imag);
}
