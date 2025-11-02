#pragma once

#include "csfg/numeric/complex.h"
#include "csfg/util/vec.h"

struct csfg_expr_pool;
struct csfg_var_table;
struct csfg_poly_expr;

VEC_DECLARE(csfg_cpoly, struct csfg_complex, 8)
VEC_DECLARE(csfg_rpoly, struct csfg_complex, 8)

/*! Rescales the coefficiets so that the highest degree coefficient equals 1.0.
 * This is known as a monic polynomial. The scale factor is returned. */
struct csfg_complex csfg_cpoly_monic(struct csfg_cpoly* poly);

/*! Finds the roots of a monic polynomial. */
int csfg_cpoly_find_roots(
    struct csfg_rpoly**      roots,
    const struct csfg_cpoly* coeffs,
    int                      n_iters,
    double                   tolerance);

int csfg_cpoly_from_symbolic(
    struct csfg_cpoly**          poly,
    struct csfg_expr_pool*       pool,
    const struct csfg_var_table* vt,
    const struct csfg_poly_expr* symbolic);

struct csfg_complex
csfg_cpoly_eval(const struct csfg_cpoly* poly, struct csfg_complex s);
