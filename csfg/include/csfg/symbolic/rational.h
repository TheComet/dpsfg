#pragma once

#include "csfg/util/vec.h"
#include "csfg/util/strview.h"

struct csfg_expr_pool;

struct csfg_coeff
{
    double factor;
    int    expr;
};

VEC_DECLARE(csfg_coeff_vec, struct csfg_coeff, 8)

struct csfg_rational
{
    struct csfg_coeff_vec* num;
    struct csfg_coeff_vec* den;
};

static struct csfg_coeff csfg_coeff(double factor, int expr)
{
    struct csfg_coeff c;
    c.factor = factor;
    c.expr = expr;
    return c;
}

static void csfg_rational_init(struct csfg_rational* r)
{
    csfg_coeff_vec_init(&r->num);
    csfg_coeff_vec_init(&r->den);
}

static void csfg_rational_deinit(struct csfg_rational* r)
{
    csfg_coeff_vec_deinit(r->num);
    csfg_coeff_vec_deinit(r->den);
}

int csfg_expr_to_rational(
    struct csfg_expr_pool** pool,
    int                     expr,
    struct strview          main_var,
    struct csfg_rational*   rational);
