#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rational.h"
#include <math.h>

VEC_DEFINE(csfg_coeff_vec, struct csfg_coeff, 8)

/* ------------------------------------------------------------------------- */
static int combine_exprs_and_factors_if_necessary(
    struct csfg_expr_pool**  pool,
    const struct csfg_coeff* c1,
    const struct csfg_coeff* c2)
{
    if (c1->factor == 0.0) /* 0a + 3b = 3b */
    {
        if (c2->factor == 0.0)
            return -1;
        if (c2->expr > -1 && c2->factor != 1.0)
            return csfg_expr_mul(
                pool, c2->expr, csfg_expr_lit(pool, c2->factor));
        return c2->expr;
    }
    else if (c2->factor == 0.0) /* 2a + 0b = 2a */
    {
        if (c1->expr > -1 && c1->factor != 1.0)
            return csfg_expr_mul(
                pool, c1->expr, csfg_expr_lit(pool, c1->factor));
        return c1->expr;
    }
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
static int
poly_copy(struct csfg_coeff_vec** dst, const struct csfg_coeff_vec* src)
{
    const struct csfg_coeff* c;
    CSFG_DEBUG_ASSERT(vec_count(*dst) == 0);
    vec_for_each (src, c)
        if (csfg_coeff_vec_push(dst, *c) != 0)
            return -1;
    return 0;
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
            k = (int)round(value);
            if (fabs(value - (double)k) >= 0.0000001)
                return -1;

            if (k == 0) /* a^0 = 1 */
            {
                csfg_coeff_vec_push(&r->num, csfg_coeff(1.0, -1));
                csfg_coeff_vec_push(&r->den, csfg_coeff(1.0, -1));
                break;
            }

            if (k > 0)
            {
                csfg_expr_to_rational(pool, left, main_var, &r1);
                poly_copy(&r->num, r1.num);
                poly_copy(&r->den, r1.den);
                while (--k > 0)
                {
                    csfg_coeff_vec_clear(r2.num);
                    csfg_coeff_vec_clear(r2.den);
                    poly_mul(pool, &r2.num, r->num, r1.num);
                    poly_mul(pool, &r2.den, r->den, r1.den);
                    csfg_coeff_vec_swap(&r->num, &r2.num);
                    csfg_coeff_vec_swap(&r->den, &r2.den);
                }
            }
            else if (k < 0)
            {
                csfg_expr_to_rational(pool, left, main_var, &r2);
                csfg_coeff_vec_swap(&r2.num, &r1.den);
                csfg_coeff_vec_swap(&r2.den, &r1.num);
                poly_copy(&r->num, r1.num);
                poly_copy(&r->den, r1.den);
                while (++k < 0)
                {
                    csfg_coeff_vec_clear(r2.num);
                    csfg_coeff_vec_clear(r2.den);
                    poly_mul(pool, &r2.num, r->num, r1.num);
                    poly_mul(pool, &r2.den, r->den, r1.den);
                    csfg_coeff_vec_swap(&r->num, &r2.num);
                    csfg_coeff_vec_swap(&r->den, &r2.den);
                }
            }
            break;
    }

    csfg_rational_deinit(&r1);
    csfg_rational_deinit(&r2);

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_rational_limit(
    struct csfg_expr_pool** pool,
    int                     expr,
    struct strview          variable,
    struct csfg_rational*   rational)
{
    int                      num_idx, den_idx;
    const struct csfg_coeff* c;

    if (csfg_expr_to_rational(pool, expr, variable, rational) != 0)
        return -1;

    vec_enumerate_r (rational->num, num_idx, c)
        if (c->factor != 0.0)
            break;
    vec_enumerate_r (rational->den, den_idx, c)
        if (c->factor != 0.0)
            break;

    if (den_idx == vec_count(rational->den))
    {
    }
    else if (num_idx == vec_count(rational->num))
    {
    }
    else if (num_idx > den_idx)
    {
    }
    else if (num_idx < den_idx)
    {
    }
    else
    {
        csfg_coeff_vec_swap_values(rational->num, 0, num_idx);
        csfg_coeff_vec_swap_values(rational->den, 0, den_idx);
        rational->num->count = 1;
        rational->den->count = 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_poly_to_expr(
    const struct csfg_coeff_vec* poly,
    const struct csfg_expr_pool* coeff_pool,
    struct csfg_expr_pool**      expr_pool)
{
    const struct csfg_coeff* c;
    int                      expr = -1;
    vec_for_each (poly, c)
    {
        int coeff_expr;
        if (c->factor == 0.0)
            continue;

        if (c->expr < 0)
            coeff_expr = csfg_expr_lit(expr_pool, c->factor);
        else
            coeff_expr = csfg_expr_mul(
                expr_pool,
                csfg_expr_dup_recurse_from(expr_pool, coeff_pool, c->expr),
                csfg_expr_lit(expr_pool, c->factor));

        if (expr == -1)
            expr = coeff_expr;
        else
            expr = csfg_expr_add(expr_pool, expr, coeff_expr);

        if (expr < 0)
            return -1;
    }

    return expr;
}

/* ------------------------------------------------------------------------- */
int csfg_rational_to_expr(
    const struct csfg_rational*  rational,
    const struct csfg_expr_pool* rational_pool,
    struct csfg_expr_pool**      expr_pool)
{
    return csfg_expr_div(
        expr_pool,
        csfg_poly_to_expr(rational->num, rational_pool, expr_pool),
        csfg_poly_to_expr(rational->den, rational_pool, expr_pool));
}
