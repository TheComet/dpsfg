#pragma once

#include "csfg/symbolic/poly.h"
#include "csfg/util/strview.h"

struct csfg_expr_pool;

struct csfg_rational
{
    struct csfg_poly* num;
    struct csfg_poly* den;
};

static void csfg_rational_init(struct csfg_rational* r)
{
    csfg_poly_init(&r->num);
    csfg_poly_init(&r->den);
}

static void csfg_rational_deinit(struct csfg_rational* r)
{
    csfg_poly_deinit(r->num);
    csfg_poly_deinit(r->den);
}

static void csfg_rational_clear(struct csfg_rational* r)
{
    csfg_poly_clear(r->num);
    csfg_poly_clear(r->den);
}

int csfg_expr_to_rational(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               main_var,
    struct csfg_expr_pool**      rational_pool,
    struct csfg_rational*        rational);

/*!
 * @brief Calculates lim variable->oo of an expression and returns it as a
 * rational function. If successful, the result will be one of:
 *   1) oo/1
 *   2) -oo/1
 *   3) 0/1
 *   4) a/b, where a and b are variables or constants
 */
int csfg_expr_to_rational_limit(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               variable,
    struct csfg_expr_pool**      rational_pool,
    struct csfg_rational*        rational);

int csfg_rational_to_expr(
    const struct csfg_rational*  rational,
    const struct csfg_expr_pool* rational_pool,
    struct csfg_expr_pool**      expr_pool);
