#pragma once

#include "csfg/util/vec.h"

struct csfg_expr_pool;
struct str;

struct csfg_coeff
{
    double factor;
    int    expr;
};

static struct csfg_coeff csfg_coeff(double factor, int expr)
{
    struct csfg_coeff c;
    c.factor = factor;
    c.expr = expr;
    return c;
}

VEC_DECLARE(csfg_poly, struct csfg_coeff, 8)

/*! Duplicates a polynomial. "dst" must be empty before calling. */
int csfg_poly_copy(struct csfg_poly** dst, const struct csfg_poly* src);

/*!
 * Calculates the sum of two polynomials. "out" must be empty before calling.
 */
int csfg_poly_add(
    struct csfg_expr_pool** pool,
    struct csfg_poly**      out,
    const struct csfg_poly* p1,
    const struct csfg_poly* p2);

/*!
 * Calculates the product of two polynomials. "out" must be empty before
 * calling.
 */
int csfg_poly_mul(
    struct csfg_expr_pool** pool,
    struct csfg_poly**      out,
    const struct csfg_poly* p1,
    const struct csfg_poly* p2);

int csfg_poly_to_str(
    struct str**                 str,
    const struct csfg_expr_pool* pool,
    const struct csfg_poly*      poly);
