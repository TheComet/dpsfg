#include "csfg/numeric/mat.h"
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
static int push_repeated_roots(struct csfg_pfd_poly** pfd_terms)
{
    struct csfg_pfd* term;
    vec_for_each_r (*pfd_terms, term)
    {
        int n = term->n;
        while (n-- > 1)
        {
            struct csfg_pfd* repeat = csfg_pfd_poly_emplace(pfd_terms);
            if (repeat == NULL)
                return -1;
            repeat->n = n;
            repeat->p = term->p;
            repeat->A = csfg_complex(NAN, NAN);
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static void expand_term(
    struct csfg_mat* mat, const struct csfg_pfd_poly* pfd_terms, int term_idx)
{
    int                    i, c, coeffs;
    const struct csfg_pfd *term, *my_term;

    CSFG_DEBUG_ASSERT(vec_count(pfd_terms) >= 2);

    coeffs = 0;
    my_term = vec_get(pfd_terms, term_idx);
    vec_enumerate (pfd_terms, i, term)
    {
        if (i == term_idx)
            continue;

        if (coeffs == 0)
            *csfg_mat_get(mat, coeffs++, term_idx) = csfg_complex_neg(term->p);

        if (csfg_complex_eq(my_term->p, term->p))
            if (my_term->n >= term->n)
                continue;

        if (coeffs == 1)
        {
            *csfg_mat_get(mat, coeffs++, term_idx) = csfg_complex(1, 0);
            continue;
        }

        *csfg_mat_get(mat, coeffs, term_idx) =
            *csfg_mat_get(mat, coeffs - 1, term_idx);
        coeffs++;

        for (c = coeffs - 2; c >= 1; --c)
        {
            struct csfg_complex c0 = *csfg_mat_get(mat, c - 1, term_idx);
            struct csfg_complex c1 = *csfg_mat_get(mat, c, term_idx);
            *csfg_mat_get(mat, c, term_idx) =
                csfg_complex_sub(c0, csfg_complex_mul(c1, term->p));
        }
        *csfg_mat_get(mat, 0, term_idx) = csfg_complex_neg(
            csfg_complex_mul(*csfg_mat_get(mat, 0, term_idx), term->p));
    }
}

/* -------------------------------------------------------------------------- */
static void
populate_matrix(struct csfg_mat* mat, const struct csfg_pfd_poly* pfd_terms)
{
    int                    c;
    const struct csfg_pfd* term;

    csfg_mat_zero(mat);
    vec_enumerate (pfd_terms, c, term)
        (void)term, expand_term(mat, pfd_terms, c);
}

/* -------------------------------------------------------------------------- */
static int eliminate_zero_terms(struct csfg_pfd* term, void* user)
{
    (void)user;
    if (term->A.real == 0.0 && term->A.imag == 0.0)
        return VEC_ERASE;
    return VEC_RETAIN;
}

/* -------------------------------------------------------------------------- */
int csfg_rpoly_partial_fraction_decomposition(
    struct csfg_pfd_poly**   pfd_terms,
    const struct csfg_cpoly* numerator,
    const struct csfg_rpoly* denominator)
{
    struct csfg_mat *        in, *out, *L, *U;
    struct csfg_mat_reorder* reorder;
    int                      pfd_term_count, i;

    csfg_mat_init(&in);
    csfg_mat_init(&out);
    csfg_mat_init(&L);
    csfg_mat_init(&U);
    csfg_mat_reorder_init(&reorder);

    csfg_pfd_poly_clear(*pfd_terms);
    if (vec_count(numerator) >= vec_count(denominator) + 1)
        return log_err(
            "Partial fraction decomposition on reducible rational functions is "
            "currently not supported\n");

    if (push_unique_roots(pfd_terms, denominator) != 0)
        goto fail;
    if (push_repeated_roots(pfd_terms) != 0)
        goto fail;

    if (vec_count(*pfd_terms) == 1)
    {
        if (vec_count(numerator) == 0)
            vec_first(*pfd_terms)->A = csfg_complex(0, 0);
        else
            vec_first(*pfd_terms)->A = *vec_first(numerator);
    }
    else
    {
        pfd_term_count = vec_count(*pfd_terms);
        if (csfg_mat_realloc(&in, pfd_term_count, pfd_term_count) != 0)
            goto fail;

        populate_matrix(in, *pfd_terms);
        if (csfg_mat_lu_decomposition(&L, &U, &reorder, in) != 0)
            goto fail;

        if (csfg_mat_realloc(&in, pfd_term_count, 1) != 0 ||
            csfg_mat_realloc(&out, pfd_term_count, 1) != 0)
            goto fail;
        for (i = 0; i != pfd_term_count; ++i)
        {
            struct csfg_complex value = i < vec_count(numerator)
                                            ? *vec_get(numerator, i)
                                            : csfg_complex(0, 0);
            *csfg_mat_get(in, i, 0) = value;
        }
        csfg_mat_solve_linear_lu(out, in, L, U, reorder);
        for (i = 0; i != pfd_term_count; ++i)
            vec_get(*pfd_terms, i)->A = *csfg_mat_get(out, i, 0);
    }

    csfg_pfd_poly_retain(*pfd_terms, eliminate_zero_terms, NULL);

    csfg_mat_reorder_deinit(reorder);
    csfg_mat_deinit(U);
    csfg_mat_deinit(L);
    csfg_mat_deinit(out);
    csfg_mat_deinit(in);
    return 0;

fail:
    csfg_mat_deinit(U);
    csfg_mat_deinit(L);
    csfg_mat_deinit(out);
    csfg_mat_deinit(in);
    return -1;
}
