#include "csfg/numeric/poly.h"
#include <math.h>
#include <stdlib.h>

VEC_DEFINE(csfg_cpoly, struct csfg_complex, 8)
VEC_DEFINE(csfg_rpoly, struct csfg_complex, 8)

static const double EPSILON = 1e-8;

/* -------------------------------------------------------------------------- */
static double rand_float(void)
{
    return (double)rand() / RAND_MAX;
}

/* -------------------------------------------------------------------------- */
static int near(double a, double b, double c, double d, double tol)
{
    double qa = a - c;
    double qb = b - d;
    double r = qa * qa + qb * qb;
    return r * r < tol;
}

/* -------------------------------------------------------------------------- */
static double bound(const struct csfg_cpoly* p)
{
    double                     b = 0.0;
    const struct csfg_complex* c;
    vec_for_each (p, c)
    {
        double r = c->real * c->real + c->imag * c->imag;
        if (b < r)
            b = r;
    }
    return 1.0 + sqrt(b);
}

/* -------------------------------------------------------------------------- */
static struct csfg_complex
calc_denominator(const struct csfg_rpoly* roots, int j, double tolerance)
{
    /* Compute denominator
     *  (zj - z0) * (zj - z1) * ... * (zj - z_{n-1}) */

    int                        k;
    double                     k1, k2, k3;
    struct csfg_complex        den, q;
    const struct csfg_complex* r2;

    struct csfg_complex r1 = *vec_get(roots, j);

    den.real = 1.0;
    den.imag = 0.0;
    vec_enumerate (roots, k, r2)
    {
        if (k == j)
            continue;

        q.real = r1.real - r2->real;
        q.imag = r1.imag - r2->imag;
        if (q.real * q.real + q.imag * q.imag < tolerance)
            continue;

        k1 = q.real * (den.real + den.imag);
        k2 = den.real * (q.imag - q.real);
        k3 = den.imag * (q.real + q.imag);
        den.real = k1 - k3;
        den.imag = k1 + k2;
    }

    return den;
}

/* -------------------------------------------------------------------------- */
static struct csfg_complex
calc_numerator(const struct csfg_cpoly* coeffs, struct csfg_complex root)
{
    int    k;
    double s1 = root.imag - root.real;
    double s2 = root.real + root.imag;

    struct csfg_complex num = *vec_last(coeffs);
    for (k = vec_count(coeffs) - 2; k >= 0; --k)
    {
        double k1 = root.real * (num.real + num.imag);
        double k2 = num.real * s1;
        double k3 = num.imag * s2;
        num.real = k1 - k3 + vec_get(coeffs, k)->real;
        num.imag = k1 + k2 + vec_get(coeffs, k)->imag;
    }

    return num;
}

/* -------------------------------------------------------------------------- */
static struct csfg_complex
calc_reciprocal(struct csfg_complex num, struct csfg_complex den)
{
    double k1, k2, k3;

    k1 = den.real * den.real + den.imag * den.imag;
    if (fabs(k1) > EPSILON)
    {
        den.real /= k1;
        den.imag /= -k1;
    }
    else
    {
        den.real = 1.0;
        den.imag = 0.0;
    }

    k1 = num.real * (den.real + den.imag);
    k2 = den.real * (num.imag - num.real);
    k3 = den.imag * (num.real + num.imag);

    return csfg_complex(k1 - k3, k1 + k2);
}

/* -------------------------------------------------------------------------- */
static void durand_kerner(
    const struct csfg_cpoly* coeffs,
    int                      n_iters,
    double                   tolerance,
    struct csfg_rpoly*       roots)
{
    int                  i, j;
    double               d;
    struct csfg_complex  num, den, rec;
    struct csfg_complex* root;

    for (i = 0; i < n_iters; ++i)
    {
        d = 0.0;
        vec_enumerate (roots, j, root)
        {
            den = calc_denominator(roots, j, tolerance);
            num = calc_numerator(coeffs, *root);
            rec = calc_reciprocal(num, den);

            /* Multiply and accumulate */
            root->real -= rec.real;
            root->imag -= rec.imag;

            d = fmax(d, fmax(fabs(rec.real), fabs(rec.imag)));
        }

        /* If converged, exit early */
        if (d < tolerance)
            break;
    }
}

/* -------------------------------------------------------------------------- */
static void postprocess(struct csfg_rpoly* roots, double tolerance)
{
    int                  i, j, count;
    struct csfg_complex* r1;
    struct csfg_complex* r2;
    struct csfg_complex  c;

    /* Combine any repeated roots */
    vec_enumerate (roots, i, r1)
    {
        count = 1;
        c.real = r1->real;
        c.imag = r1->imag;
        vec_enumerate (roots, j, r2)
        {
            if (i == j)
                continue;

            if (near(r1->real, r1->imag, r2->real, r2->imag, tolerance))
            {
                ++count;
                c.real += r2->real;
                c.imag += r2->imag;
            }
        }

        if (count == 1)
            continue;

        c.real /= count;
        c.imag /= count;
        vec_enumerate (roots, j, r2)
        {
            if (i == j)
                continue;

            if (near(r1->real, r1->imag, r2->real, r2->imag, tolerance))
            {
                r2->real = c.real;
                r2->imag = c.imag;
            }
        }
        r1->real = c.real;
        r1->imag = c.imag;
    }
}

/* -------------------------------------------------------------------------- */
struct csfg_complex csfg_cpoly_monic(struct csfg_cpoly* poly)
{
    struct csfg_complex scale;
    double              d, s, t, k1, k2, k3;
    int                 i;

    if (vec_count(poly) <= 1)
        return csfg_complex(1.0, 0.0);

    scale = *vec_last(poly);
    d = scale.real * scale.real + scale.imag * scale.imag;
    scale.real /= d;
    scale.imag /= -d;
    s = scale.imag - scale.real;
    t = scale.real + scale.imag;
    for (i = 0; i < vec_count(poly) - 1; ++i)
    {
        k1 = scale.real * (vec_get(poly, i)->real + vec_get(poly, i)->imag);
        k2 = vec_get(poly, i)->real * s;
        k3 = vec_get(poly, i)->imag * t;
        vec_get(poly, i)->real = k1 - k3;
        vec_get(poly, i)->imag = k1 + k2;
    }
    vec_last(poly)->real = 1.0;
    vec_last(poly)->imag = 0.0;

    return scale;
}

/* -------------------------------------------------------------------------- */
int csfg_cpoly_find_roots(
    struct csfg_rpoly**      roots,
    const struct csfg_cpoly* coeffs,
    int                      n_iters,
    double                   tolerance)
{
    int i;

    csfg_rpoly_clear(*roots);

    if (vec_count(coeffs) <= 1)
        return 0;

    /* Use csfg_cpoly_monic() first if these asserts fail */
    CSFG_DEBUG_ASSERT(vec_last(coeffs)->real == 1.0);
    CSFG_DEBUG_ASSERT(vec_last(coeffs)->imag == 0.0);

    if (n_iters <= 0)
        n_iters = 100 * vec_count(coeffs);

    if (tolerance <= 0.0)
        tolerance = 1e-6;

    /* Pick default initial guess if unspecified */
    if (vec_count(*roots) == 0)
    {
        double t, c, r;
        if (csfg_rpoly_realloc(roots, vec_count(coeffs) - 1) != 0)
            return -1;

        r = bound(coeffs);
        for (i = 0; i < vec_count(coeffs) - 1; ++i)
        {
            t = rand_float() * r;
            c = cos(rand_float() * 2.0 * M_PI);
            csfg_rpoly_push(roots, csfg_complex(t * c, t * sqrt(1.0 - c * c)));
        }
    }

    durand_kerner(coeffs, n_iters, tolerance, *roots);
    postprocess(*roots, tolerance);
    return 0;
}
