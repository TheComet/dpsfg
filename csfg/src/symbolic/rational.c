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

        int expr1 =
            c1->expr > -1
                ? csfg_expr_mul(pool, c1->expr, csfg_expr_lit(pool, c1->factor))
                : -1;
        int expr2 =
            c2->expr > -1
                ? csfg_expr_mul(pool, c2->expr, csfg_expr_lit(pool, c2->factor))
                : -1;
        int expr = expr1 > -1 && expr1 > -1 ? csfg_expr_add(pool, expr1, expr2)
                   : expr1 > -1             ? expr1
                   : expr2 > -1             ? expr2
                                            : -1;
        double factor =
            c1->expr == -1 && c2->expr == -1 ? c1->factor + c2->factor : 1.0;

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
    int            left, right;
    struct strview var;

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
                &r->num, csfg_coeff(1.0, csfg_expr_copy(pool, expr)));
            csfg_coeff_vec_push(&r->den, csfg_coeff(1.0, -1));
            break;
        case CSFG_EXPR_VAR:
            var = strlist_view(
                (*pool)->var_names, (*pool)->nodes[expr].value.var_idx);
            if (!strview_eq(var, main_var))
                csfg_coeff_vec_push(
                    &r->num, csfg_coeff(1.0, csfg_expr_copy(pool, expr)));
            else
            {
                /* 0 + 1*s */
                csfg_coeff_vec_push(&r->num, csfg_coeff(0.0, -1));
                csfg_coeff_vec_push(&r->num, csfg_coeff(1.0, -1));
            }
            csfg_coeff_vec_push(&r->den, csfg_coeff(1.0, -1));
            break;
        case CSFG_EXPR_NEG: break;
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
            // gcd_cancel()
            /*
            *num = csfg_expr_set_add(
                pool,
                expr,
                csfg_expr_mul(pool, num_left, den_right),
                csfg_expr_mul(pool, num_right, den_left));
            *den = csfg_expr_mul(pool, den_left, den_right);
            */
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
             * /N1\   N1^k
             * |--| = ----
             * \D1/   D1^k
             */
            if ((*pool)->nodes[right].type != CSFG_EXPR_LIT)
                return -1;
            if (!is_almost_integer((*pool)->nodes[right].value.lit, 0.0000001))
                return -1;
            csfg_expr_to_rational(pool, left, main_var, &r1);
            // TODO
            break;
    }

    csfg_rational_deinit(&r1);
    csfg_rational_deinit(&r2);

    return 0;
}
