#pragma once

#include "csfg/util/vec.h"

struct csfg_expr_pool;
struct csfg_var_table;
struct csfg_poly;

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

VEC_DECLARE(csfg_cpoly, struct csfg_complex, 8)
VEC_DECLARE(csfg_rpoly, struct csfg_complex, 8)

int csfg_cpoly_find_roots(
    struct csfg_cpoly*  coeffs,
    struct csfg_rpoly** roots,
    int                 n_iters,
    double              tolerance);

int csfg_cpoly_from_symbolic(
    struct csfg_cpoly**          poly,
    struct csfg_expr_pool*       pool,
    const struct csfg_var_table* vt,
    const struct csfg_poly*      symbolic);
