#pragma once

#include "csfg/numeric/complex.h"
#include "csfg/util/vec.h"

struct csfg_expr_pool;
struct csfg_var_table;
struct csfg_poly_expr;

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
    const struct csfg_poly_expr*     symbolic);

void csfg_rpoly_interesting_frequency_interval(
    const struct csfg_rpoly* poly, double* f_start_hz, double* f_end_hz);

void csfg_rpoly_interesting_time_interval(
    const struct csfg_rpoly* poly, double* t_start_s, double* t_end_s);
