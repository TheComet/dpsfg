#pragma once

#include "csfg/util/vec.h"

struct csfg_expr_pool;
struct str;

struct csfg_coeff_expr
{
    double factor;
    int expr;
};

static struct csfg_coeff_expr csfg_coeff_expr(double factor, int expr)
{
    struct csfg_coeff_expr c;
    c.factor = factor;
    c.expr   = expr;
    return c;
}

VEC_DECLARE(csfg_poly_expr, struct csfg_coeff_expr, 8)

/*! Duplicates a polynomial. "dst" must be empty before calling. */
int csfg_poly_expr_copy(
    struct csfg_poly_expr** dst, const struct csfg_poly_expr* src);

/*!
 * Calculates the sum of two polynomials. "out" must be empty before calling.
 */
int csfg_poly_expr_add(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** out,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2);

/*!
 * Calculates the product of two polynomials. "out" must be empty before
 * calling.
 */
int csfg_poly_expr_mul(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** out,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2);

/*!
 * \brief Performs long division on two polynomials.
 * \note This is an in-place division, meaning, one of the input polynomials is
 * modified.
 * \param[out] quotient Result of division is written to this out-param.
 * \param[inout] remainder Input is the dividend (numerator). Output is the
 * remainder of the division.
 */
int csfg_poly_expr_div(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** quotient,
    struct csfg_poly_expr** remainder,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2);

int csfg_poly_expr_gcd(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** gcd,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2);

int csfg_poly_expr_to_str(
    struct str** str,
    const struct csfg_expr_pool* pool,
    const struct csfg_poly_expr* poly);

#define csfg_poly_expr_degree(p) (vec_count(p) - 1)
