#pragma once

#include "csfg/numeric/poly.h"

struct csfg_tf_expr;

struct csfg_tf
{
    /* The polynomials listed below are monic polynomials, meaning, the
     * coefficients are re-scaled such that the highest-degree coefficient
     * equals 1.0. The constant factor is saved here so we can still evaluate
     * the transfer function correctly. */
    struct csfg_complex factor;

    struct csfg_cpoly* num;
    struct csfg_cpoly* den;

    struct csfg_rpoly* zeros;
    struct csfg_rpoly* poles;

    struct csfg_pfd_poly* pfd_terms;
};

static void csfg_tf_init(struct csfg_tf* tf)
{
    tf->factor = csfg_complex(1.0, 0.0);
    csfg_cpoly_init(&tf->num);
    csfg_cpoly_init(&tf->den);
    csfg_rpoly_init(&tf->zeros);
    csfg_rpoly_init(&tf->poles);
    csfg_pfd_poly_init(&tf->pfd_terms);
}

static void csfg_tf_deinit(struct csfg_tf* tf)
{
    csfg_pfd_poly_deinit(tf->pfd_terms);
    csfg_rpoly_deinit(tf->poles);
    csfg_rpoly_deinit(tf->zeros);
    csfg_cpoly_deinit(tf->den);
    csfg_cpoly_deinit(tf->num);
}

int csfg_tf_from_symbolic(
    struct csfg_tf*              tf,
    struct csfg_expr_pool*       pool,
    const struct csfg_tf_expr*   tf_expr,
    const struct csfg_var_table* vt);

int csfg_tf_interesting_frequency_interval(
    const struct csfg_tf* tf, double* f_start_hz, double* f_end_hz);

int csfg_tf_interesting_time_interval(
    const struct csfg_tf* tf, double* t_start_s, double* t_end_s);

struct csfg_complex
csfg_tf_eval(const struct csfg_tf* tf, struct csfg_complex s);
