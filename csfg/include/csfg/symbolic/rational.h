#pragma once

#include "csfg/symbolic/poly.h"
#include "csfg/util/strview.h"

struct csfg_expr_pool;
struct csfg_var_table;

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
 * @brief Calculates lim_{variable->oo} of an expression and returns it as a
 * rational function. In the case of divergence, zero and infinity remain
 * symbolic. I.e. no floating point infinity or NaN values are set in the
 * result.
 *
 * The second version, _limits(), accepts a variable table instead of a single
 * variable. In this case, each "oo" entry in the table will be applied in
 * succession.
 *
 * If successful, the result will be one of:
 *   1) oo/1
 *   2) -oo/1
 *   3) 1/oo
 *   4) -1/oo
 *   5) 0/0
 *   6) a/b, where a and b are expressions
 *
 * Returns -1 on failure, 0 on success.
 */
int csfg_expr_to_rational_limit(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               variable,
    struct csfg_expr_pool**      rational_pool,
    struct csfg_rational*        rational);
int csfg_expr_to_rational_limits(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    const struct csfg_var_table* vt,
    struct csfg_expr_pool**      rational_pool,
    struct csfg_rational*        rational);

int csfg_rational_to_expr(
    const struct csfg_rational*  rational,
    const struct csfg_expr_pool* rational_pool,
    struct csfg_expr_pool**      expr_pool);
