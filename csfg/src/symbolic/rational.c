#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rational.h"
#include <math.h>

VEC_DEFINE(csfg_coeff_vec, struct csfg_coeff, 8)

/* ------------------------------------------------------------------------- */
static int is_almost_integer(double value, double epsilon)
{
    double nearest = round(value);
    return fabs(value - nearest) < epsilon;
}

static int combine_exprs_and_factors_if_necessary(
    struct csfg_expr_pool**  pool,
    const struct csfg_coeff* c1,
    const struct csfg_coeff* c2)
{
    if (c1->factor == 0.0) /* 0a + 3b = 3b */
        return c2->expr;
    else if (c2->factor == 0.0) /* 2a + 0b = 2a */
        return c1->expr;
    else if (c1->factor == 1.0 && c2->factor == 1.0) /* 1a + 1b = 1(a+b) */
    {
        if (c1->expr > -1 && c2->expr > -1) /* 1a + 1b = 1(a+b) */
            return csfg_expr_add(pool, c1->expr, c2->expr);
        else if (c1->expr > -1) /* 1a + 1 = 1(a+1) */
            return csfg_expr_add(pool, c1->expr, csfg_expr_lit(pool, 1.0));
        else if (c2->expr > -1) /* 1 + 1b = 1(1+b) */
            return csfg_expr_add(pool, c2->expr, csfg_expr_lit(pool, 1.0));
        else /* 1 + 1 = 2 */
            return -1;
    }
    else if (c1->factor == 1.0) /* 1a + 3b = 1(a+3b) */
    {
        if (c1->expr > -1 && c2->expr > -1) /* 1a + 3b = 1(a+3b) */
            return csfg_expr_add(
                pool,
                c1->expr,
                csfg_expr_mul(pool, csfg_expr_lit(pool, c2->factor), c2->expr));
        else if (c1->expr > -1) /* 1a + 3 = 1(a+3) */
            return csfg_expr_add(
                pool, csfg_expr_lit(pool, c2->factor), c1->expr);
        else if (c2->expr > -1) /* 1 + 3b = 1(1+3b) */
            return csfg_expr_add(
                pool,
                csfg_expr_lit(pool, 1.0),
                csfg_expr_mul(pool, csfg_expr_lit(pool, c2->factor), c2->expr));
        else
            return -1;
    }
    else if (c2->factor == 1.0) /* 2a + 1b = 1(2a+b) */
    {
        if (c1->expr > -1 && c2->expr > -1) /* 2a + b = 1(2a+b) */
            return csfg_expr_add(
                pool,
                csfg_expr_mul(pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                c2->expr);
        else if (c1->expr > -1) /* 2a + 1 = 1(2a+1) */
            return csfg_expr_add(
                pool,
                csfg_expr_mul(pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                csfg_expr_lit(pool, 1.0));
        else if (c2->expr > -1) /* 2 + 1b = 1(2+b) */
            return csfg_expr_add(
                pool, csfg_expr_lit(pool, c1->factor), c2->expr);
        else
            return -1;
    }
    else /* 2a + 3b = 1(2a + 3b) */
    {
        if (c1->expr > -1 && c2->expr > -1)
            return csfg_expr_add(
                pool,
                csfg_expr_mul(pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                csfg_expr_mul(pool, csfg_expr_lit(pool, c2->factor), c2->expr));
        else if (c1->expr > -1) /* 2a + 3  = 1(2a+3) */
            return csfg_expr_add(
                pool,
                csfg_expr_mul(pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                csfg_expr_lit(pool, c2->factor));
        else if (c2->expr > -1) /* 2 + 3b = 1(2+3b) */
            return csfg_expr_add(
                pool,
                csfg_expr_lit(pool, c1->factor),
                csfg_expr_mul(pool, csfg_expr_lit(pool, c2->factor), c2->expr));
        else /* 2 + 3 = 5 */
            return -1;
    }
}

/* ------------------------------------------------------------------------- */
static int poly_add(
    struct csfg_expr_pool**      pool,
    struct csfg_coeff_vec**      out,
    const struct csfg_coeff_vec* p1,
    const struct csfg_coeff_vec* p2)
{
    int i, degree, min_degree;

    degree = vec_count(p1) > vec_count(p2) ? vec_count(p1) : vec_count(p2);
    min_degree = vec_count(p1) < vec_count(p2) ? vec_count(p1) : vec_count(p2);

    if (csfg_coeff_vec_realloc(out, degree) != 0)
        return -1;
    CSFG_DEBUG_ASSERT(vec_count(*out) == 0);

    for (i = 0; i != min_degree; ++i)
    {
        const struct csfg_coeff* c1 = vec_get(p1, i);
        const struct csfg_coeff* c2 = vec_get(p2, i);

        int    expr = combine_exprs_and_factors_if_necessary(pool, c1, c2);
        double factor = expr > -1 ? 1.0 : c1->factor + c2->factor;

        csfg_coeff_vec_push(out, csfg_coeff(factor, expr));
    }

    for (; i != degree; ++i)
    {
        const struct csfg_coeff* c1 =
            i < vec_count(p1) ? vec_get(p1, i) : vec_get(p2, i);
        csfg_coeff_vec_push(out, csfg_coeff(c1->factor, c1->expr));
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int poly_mul(
    struct csfg_expr_pool**      pool,
    struct csfg_coeff_vec**      out,
    const struct csfg_coeff_vec* p1,
    const struct csfg_coeff_vec* p2)
{
    const struct csfg_coeff *c1, *c2;
    int                      i, i1, i2, degree;

    degree = vec_count(p1) + vec_count(p2) - 1;
    if (csfg_coeff_vec_realloc(out, degree) != 0)
        return -1;
    CSFG_DEBUG_ASSERT(vec_count(*out) == 0);

    /* (1x^2 + 2x + 3)(9x^2 + 5x + 1) */
    /* (1*9)x^4 + (1*5+2*9)x^3 + (1*1+2*5+3*9)x^2 + (2*1+3*5)x + (3*1) */
    /* 9x^4 + 23x^3 + 38x^2 + 17x + 3 */
    for (i = 0; i != degree; ++i)
    {
        double factor = 0.0;
        int    expr = -1;
        vec_enumerate (p1, i1, c1)
            vec_enumerate (p2, i2, c2)
                if (i1 + i2 == i)
                {
                    int subexpr = c1->expr > -1 && c2->expr > -1
                                      ? csfg_expr_mul(pool, c1->expr, c2->expr)
                                  : c1->expr > -1 ? c1->expr
                                  : c2->expr > -1 ? c2->expr
                                                  : -1;
                    if (subexpr > -1 && expr > -1)
                        expr = csfg_expr_mul(pool, expr, subexpr);
                    else
                        expr = subexpr;

                    factor += c1->factor * c2->factor;
                }
        csfg_coeff_vec_push(out, csfg_coeff(factor, expr));
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_rational(
    struct csfg_expr_pool** pool,
    int                     expr,
    struct strview          main_var,
    struct csfg_rational*   r)
{
    int                left, right, k;
    struct strview     var;
    struct csfg_coeff* coeff;
    double             value;

    struct csfg_rational r1, r2;
    csfg_rational_init(&r1);
    csfg_rational_init(&r2);

    left = (*pool)->nodes[expr].child[0];
    right = (*pool)->nodes[expr].child[1];

    switch ((*pool)->nodes[expr].type)
    {
        case CSFG_EXPR_GC: assert(0); break;
        case CSFG_EXPR_LIT:
            csfg_coeff_vec_push(
                &r->num, csfg_coeff((*pool)->nodes[expr].value.lit, -1));
            csfg_coeff_vec_push(&r->den, csfg_coeff(1.0, -1));
            break;

        case CSFG_EXPR_INF:
            csfg_coeff_vec_push(
                &r->num, csfg_coeff(1.0, csfg_expr_dup_single(pool, expr)));
            csfg_coeff_vec_push(&r->den, csfg_coeff(1.0, -1));
            break;

        case CSFG_EXPR_VAR:
            var = strlist_view(
                (*pool)->var_names, (*pool)->nodes[expr].value.var_idx);
            if (!strview_eq(var, main_var))
                csfg_coeff_vec_push(
                    &r->num, csfg_coeff(1.0, csfg_expr_dup_single(pool, expr)));
            else
            {
                /* 0 + 1*s */
                csfg_coeff_vec_push(&r->num, csfg_coeff(0.0, -1));
                csfg_coeff_vec_push(&r->num, csfg_coeff(1.0, -1));
            }
            csfg_coeff_vec_push(&r->den, csfg_coeff(1.0, -1));
            break;

        case CSFG_EXPR_NEG:
            csfg_expr_to_rational(pool, left, main_var, r);
            vec_for_each (r->num, coeff)
                coeff->factor *= -1;
            break;

        case CSFG_EXPR_OP_ADD:
            /*
             * N1   N2   N1*D2 + N2*D1
             * -- + -- = -------------
             * D1   D2       D1*D2
             */
            csfg_expr_to_rational(pool, left, main_var, &r1);
            csfg_expr_to_rational(pool, right, main_var, &r2);
            /* N1*D2 */
            poly_mul(pool, &r->num, r1.num, r2.den);
            /* N2*D1 */
            csfg_coeff_vec_clear(r1.num);
            poly_mul(pool, &r1.num, r2.num, r1.den);
            /* N1*D2 + N2*D1 */
            csfg_coeff_vec_swap(&r->num, &r2.num);
            csfg_coeff_vec_clear(r->num);
            poly_add(pool, &r->num, r1.num, r2.num);
            /* D1*D2 */
            poly_mul(pool, &r->den, r1.den, r2.den);
            break;

        case CSFG_EXPR_OP_MUL:
            /*
             * N1   N2   N1*N2
             * -- * -- = -----
             * D1   D2   D1*D2
             */
            csfg_expr_to_rational(pool, left, main_var, &r1);
            csfg_expr_to_rational(pool, right, main_var, &r2);
            poly_mul(pool, &r->num, r1.num, r2.num);
            poly_mul(pool, &r->den, r1.den, r2.den);
            break;

        case CSFG_EXPR_OP_POW:
            /*
             *     k
             * /N1\   N1^k   D1^-k
             * |--| = ---- = -----
             * \D1/   D1^k   N1^-k
             */
            if ((*pool)->nodes[right].type != CSFG_EXPR_LIT)
                return -1;

            value = (*pool)->nodes[right].value.lit;
            if (!is_almost_integer(value, 0.0000001))
                return -1;
            k = (int)round(value);

            if (k == 0) /* a^0 = 1 */
            {
                csfg_coeff_vec_push(&r->num, csfg_coeff(1.0, -1));
                csfg_coeff_vec_push(&r->den, csfg_coeff(1.0, -1));
            }
            else if (k > 0)
            {
                csfg_expr_to_rational(pool, left, main_var, r);
                while (--k > 0)
                {
                    poly_mul(pool, &r1.num, r->num, r->num);
                    poly_mul(pool, &r1.den, r->den, r->den);
                    csfg_coeff_vec_swap(&r->num, &r1.num);
                    csfg_coeff_vec_swap(&r->den, &r1.den);
                }
            }
            else if (k < 0)
            {
                csfg_expr_to_rational(pool, left, main_var, &r1);
                csfg_coeff_vec_swap(&r->num, &r1.den);
                csfg_coeff_vec_swap(&r->den, &r1.num);
                while (++k < 0)
                {
                    poly_mul(pool, &r1.num, r->num, r->num);
                    poly_mul(pool, &r1.den, r->den, r->den);
                    csfg_coeff_vec_swap(&r->num, &r1.num);
                    csfg_coeff_vec_swap(&r->den, &r1.den);
                }
            }
            break;
    }

    csfg_rational_deinit(&r1);
    csfg_rational_deinit(&r2);

    return 0;
}
