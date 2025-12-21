#pragma once

#include "csfg/numeric/complex.h"
#include "csfg/util/vec.h"

struct csfg_expr_pool;
struct csfg_var_table;
struct csfg_poly_expr;

/*!
 * "cpoly" is short for "coefficient-polynomial". A list of coefficients are
 * stored in ascending degree:
 *
 *   p(s) = c0 + c1*s + c2*s^2 + ... + cn*s^n
 */
VEC_DECLARE(csfg_cpoly, struct csfg_complex, 8)

/*!
 * "rpoly" is short for "root-polynomial". A list of roots are stored in no
 * particular order. Roots are allowed to repeat.
 * @note This is confusing, but the roots are stored negated. For example:
 *
 *  p(s) = (s-1)*(s+2)  we store  [1, -2]
 */
VEC_DECLARE(csfg_rpoly, struct csfg_complex, 8)

/*!
 * @brief Stores one term of a Partial Fraction Decomposition
 *
 *              A
 *    T(s) = ------- + ...
 *           (s-p)^n
 */
struct csfg_pfd
{
    struct csfg_complex A;
    struct csfg_complex p;
    int8_t              n;
};

VEC_DECLARE(csfg_pfd_poly, struct csfg_pfd, 8)

/*! Rescales the coefficiets so that the highest degree coefficient equals 1.0.
 * This is known as a monic polynomial. The scale factor is returned. */
struct csfg_complex csfg_cpoly_monic(struct csfg_cpoly* poly);

/*! Finds the roots of a monic polynomial using the Durand-Kerner method. */
int csfg_cpoly_find_roots(
    struct csfg_rpoly**      roots,
    const struct csfg_cpoly* coeffs,
    int                      n_iters,
    double                   tolerance);

/*!
 * @brief Calculates the Partial Fraction Decomposition of a root-polynomial.
 *
 *          b0 + b1*s + ... + bn*s^n    A1     A2           An
 *   T(s) = ------------------------ = ---- + ---- + ... + ----
 *           (s-p1)(s-p2)...(s-pn)     s-p1   s-p2         s-pn
 *
 * Repeated roots are supported.
 *
 * @return Returns -1 if an error occurs, 0 if successful.
 */
int csfg_rpoly_partial_fraction_decomposition(
    struct csfg_pfd_poly**   pfd_terms,
    const struct csfg_cpoly* numerator,
    const struct csfg_rpoly* denominator);

double csfg_pfd_poly_eval_inverse_laplace(
    const struct csfg_pfd_poly* pfd,
    double t);

/*! Evaluates each coefficient expression of a symbolic polynomial, converting
 * it into a numeric coefficient-polynomial */
int csfg_cpoly_from_symbolic(
    struct csfg_cpoly**          coeffs,
    struct csfg_expr_pool*       pool,
    const struct csfg_var_table* vt,
    const struct csfg_poly_expr* symbolic);

/*! Evaluates a coefficient-polynomial at the value "s" */
struct csfg_complex
csfg_cpoly_eval(const struct csfg_cpoly* poly, struct csfg_complex s);
