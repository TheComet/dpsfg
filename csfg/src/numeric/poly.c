#include "csfg/numeric/poly.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly_expr.h"
#include <float.h>

/* -------------------------------------------------------------------------- */
int csfg_cpoly_from_symbolic(
    struct csfg_cpoly**          poly,
    struct csfg_expr_pool*       pool,
    const struct csfg_var_table* vt,
    const struct csfg_poly_expr* symbolic)
{
    const struct csfg_coeff_expr* coeff;

    csfg_cpoly_clear(*poly);
    if (csfg_cpoly_realloc(poly, vec_count(symbolic)) != 0)
        return -1;

    vec_for_each (symbolic, coeff)
    {
        double value = coeff->factor;
        if (coeff->expr > -1)
            value *= csfg_expr_eval(pool, coeff->expr, vt);
        csfg_cpoly_push(poly, csfg_complex(value, 0.0));
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
void csfg_rpoly_interesting_frequency_interval(
    const struct csfg_rpoly* poly, double* f_start_hz, double* f_end_hz)
{
    const struct csfg_complex* c;

    double min_x = DBL_MAX;
    double max_x = 0.0;

    vec_for_each (poly, c)
    {
        double mag = csfg_complex_mag(c);
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
void csfg_rpoly_interesting_time_interval(
    const struct csfg_rpoly* poly, double* t_start_s, double* t_end_s)
{
    const struct csfg_complex* c;

    double closest_pole = DBL_MAX;

    vec_for_each (poly, c)
    {
        double real = fabs(c->real);
        if (closest_pole > real)
            closest_pole = real;
    }

    *t_start_s = 0.0;
    *t_end_s = 10 / closest_pole;
}
