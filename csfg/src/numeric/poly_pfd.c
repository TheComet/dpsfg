#include "csfg/numeric/poly.h"

/* -------------------------------------------------------------------------- */
static int push_unique_roots(
    struct csfg_pfd_poly** pfd_terms, const struct csfg_rpoly* poles)
{
    const struct csfg_complex* c;
    struct csfg_pfd*           term;

    vec_for_each (poles, c)
    {
        vec_for_each (*pfd_terms, term)
            if (term->p.real == c->real && term->p.imag == c->imag)
            {
                term->n++;
                goto root_is_repeated;
            }

        term = csfg_pfd_poly_emplace(pfd_terms);
        if (term == NULL)
            return -1;
        term->A = csfg_complex(NAN, NAN);
        term->p = *c;
        term->n = 1;

    root_is_repeated:;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static struct csfg_complex heaviside_coverup(
    struct csfg_pfd_poly*    pfd_terms,
    const struct csfg_cpoly* numerator,
    int                      covered_idx)
{
    int                 i, first;
    struct csfg_pfd*    term;
    struct csfg_complex den;

    struct csfg_complex p = vec_get(pfd_terms, covered_idx)->p;
    struct csfg_complex num = csfg_cpoly_eval(numerator, p);

    first = 1;
    vec_enumerate (pfd_terms, i, term)
    {
        struct csfg_complex pole;
        if (i == covered_idx)
            continue;

        pole = csfg_complex_sub(p, term->p);
        if (first)
            first = 0, den = pole;
        else
            den = csfg_complex_mul(den, pole);
    }

    return csfg_complex_div(num, den);
}

/* -------------------------------------------------------------------------- */
static int factor_in_root(struct csfg_pfd_poly** pfd_terms, int index_to_factor)
{
    int              i;
    struct csfg_pfd *term, *other_term;
    struct csfg_pfd* factor_term = vec_get(*pfd_terms, index_to_factor);

    vec_enumerate_r (*pfd_terms, i, term)
    {
        if (i == index_to_factor)
            continue;

        if (term->p.real == factor_term->p.real &&
            term->p.imag == factor_term->p.imag)
        {
            term->n++;
            continue;
        }

        other_term = csfg_pfd_poly_emplace(pfd_terms);
        if (other_term == NULL)
            return -1;
        other_term->p = factor_term->p;
        other_term->n = 1;

        other_term->A =
            csfg_complex_div(term->A, csfg_complex_sub(other_term->p, term->p));
        term->A =
            csfg_complex_div(term->A, csfg_complex_sub(term->p, other_term->p));
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_rpoly_partial_fraction_decomposition(
    struct csfg_pfd_poly**   pfd_terms,
    const struct csfg_cpoly* numerator,
    const struct csfg_rpoly* denominator)
{
    int              i;
    struct csfg_pfd* term;

    csfg_pfd_poly_clear(*pfd_terms);

    if (push_unique_roots(pfd_terms, denominator) != 0)
        return -1;

    vec_enumerate (*pfd_terms, i, term)
        term->A = heaviside_coverup(*pfd_terms, numerator, i);

    vec_enumerate_r (*pfd_terms, i, term)
    {
        int degree = term->n;
        while (degree-- > 1)
        {
            if (factor_in_root(pfd_terms, i) != 0)
                return -1;
        }
    }

    return 0;
}
