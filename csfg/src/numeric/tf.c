#include "csfg/numeric/tf.h"
#include "csfg/symbolic/tf_expr.h"
#include <float.h>

/* -------------------------------------------------------------------------- */
int csfg_tf_from_symbolic(
    struct csfg_tf*              tf,
    struct csfg_expr_pool*       pool,
    const struct csfg_tf_expr*   tf_expr,
    const struct csfg_var_table* vt)
{
    if (csfg_cpoly_from_symbolic(&tf->num, pool, vt, tf_expr->num) != 0)
        return -1;
    if (csfg_cpoly_from_symbolic(&tf->den, pool, vt, tf_expr->den) != 0)
        return -1;

    /* Need to be monic polynomials for find_roots() */
    tf->factor = csfg_cpoly_monic(tf->den);
    tf->factor = csfg_complex_div(tf->factor, csfg_cpoly_monic(tf->num));

    csfg_cpoly_find_roots(&tf->zeros, tf->num, 0, 0.0);
    csfg_cpoly_find_roots(&tf->poles, tf->den, 0, 0.0);

    csfg_rpoly_partial_fraction_decomposition(
        &tf->pfd_terms, tf->num, tf->poles);

    return 0;
}

/* -------------------------------------------------------------------------- */
void csfg_tf_interesting_frequency_interval(
    const struct csfg_tf* tf, double* f_start_hz, double* f_end_hz)
{
    const struct csfg_complex* c;

    double min_x = DBL_MAX;
    double max_x = 0.0;

    vec_for_each (tf->poles, c)
    {
        double mag = csfg_complex_mag(*c);
        /* In case of zero poles, avoid setting 0.0 as a range so logarithmic
         * plots don't lose their shit */
        if (mag == 0.0)
            continue;

        if (max_x < mag)
            max_x = mag;
        if (min_x > mag)
            min_x = mag;
    }

    if (max_x == 0.0)
    {
        min_x = 1.0;
        max_x = 1.0;
    }

    *f_start_hz = min_x * 0.01;
    *f_end_hz = max_x * 100;
}

/* -------------------------------------------------------------------------- */
void csfg_tf_interesting_time_interval(
    const struct csfg_tf* tf, double* t_start_s, double* t_end_s)
{
    const struct csfg_complex* c;

    double closest_pole = DBL_MAX;

    vec_for_each (tf->poles, c)
    {
        double real = fabs(c->real);
        if (closest_pole > real)
            closest_pole = real;
    }

    *t_start_s = 0.0;
    *t_end_s = 10 / closest_pole;
}

/* -------------------------------------------------------------------------- */
struct csfg_complex
csfg_tf_eval(const struct csfg_tf* tf, struct csfg_complex s)
{
    struct csfg_complex num = csfg_cpoly_eval(tf->num, s);
    struct csfg_complex den = csfg_cpoly_eval(tf->den, s);
    struct csfg_complex c1 = csfg_complex_mul(num, tf->factor);
    struct csfg_complex c2 = csfg_complex_div(c1, den);
    return c2;
}
