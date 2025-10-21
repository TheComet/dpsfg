#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly.h"
#include "csfg/util/str.h"

VEC_DEFINE(csfg_poly, struct csfg_coeff, 8)

/* ------------------------------------------------------------------------- */
static int set_coeff(struct csfg_coeff* c, double factor, int expr)
{
    c->factor = factor;
    c->expr = expr;
    return 0;
}

/* ------------------------------------------------------------------------- */
static int set_and_check_coeff(struct csfg_coeff* c, double factor, int expr)
{
    if (expr < 0)
        return -1;
    return set_coeff(c, factor, expr);
}

/* ------------------------------------------------------------------------- */
static int add_coeffs(
    struct csfg_expr_pool**  pool,
    const struct csfg_coeff* c1,
    const struct csfg_coeff* c2,
    struct csfg_coeff*       out)
{
    if (c1->factor == 0.0) /* 0a + 3b = 3b */
    {
        if (c2->factor == 0.0) /* 0a + 0b = 0 */
            return set_coeff(out, 0.0, -1);
        return set_coeff(out, c2->factor, c2->expr);
    }
    else if (c2->factor == 0.0) /* 2a + 0b = 2a */
    {
        return set_coeff(out, c1->factor, c1->expr);
    }
    else if (c1->factor == 1.0 && c2->factor == 1.0) /* 1a + 1b = 1(a+b) */
    {
        if (c1->expr > -1 && c2->expr > -1) /* 1a + 1b = 1(a+b) */
            return set_and_check_coeff(
                out, 1.0, csfg_expr_add(pool, c1->expr, c2->expr));
        else if (c1->expr > -1) /* 1a + 1 = 1(a+1) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(pool, c1->expr, csfg_expr_lit(pool, 1.0)));
        else if (c2->expr > -1) /* 1 + 1b = 1(1+b) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(pool, csfg_expr_lit(pool, 1.0), c2->expr));
        else /* 1 + 1 = 2 */
            return set_coeff(out, c1->factor + c2->factor, -1);
    }
    else if (c1->factor == 1.0) /* 1a + 3b = 1(a+3b) */
    {
        if (c1->expr > -1 && c2->expr > -1) /* 1a + 3b = 1(a+3b) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(
                    pool,
                    c1->expr,
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c2->factor), c2->expr)));
        else if (c1->expr > -1) /* 1a + 3 = 1(a+3) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(pool, c1->expr, csfg_expr_lit(pool, c2->factor)));
        else if (c2->expr > -1) /* 1 + 3b = 1(1+3b) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(
                    pool,
                    csfg_expr_lit(pool, 1.0),
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c2->factor), c2->expr)));
        else /* 1 + 3 = 4 */
            return set_coeff(out, c1->factor + c2->factor, -1);
    }
    else if (c2->factor == 1.0) /* 2a + 1b = 1(2a+b) */
    {
        if (c1->expr > -1 && c2->expr > -1) /* 2a + b = 1(2a+b) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(
                    pool,
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                    c2->expr));
        else if (c1->expr > -1) /* 2a + 1 = 1(2a+1) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(
                    pool,
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                    csfg_expr_lit(pool, 1.0)));
        else if (c2->expr > -1) /* 2 + 1b = 1(2+b) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(pool, csfg_expr_lit(pool, c1->factor), c2->expr));
        else /* 2 + 1 = 3 */
            return set_coeff(out, c1->factor + c2->factor, -1);
    }
    else /* 2a + 3b = 1(2a + 3b) */
    {
        if (c1->expr > -1 && c2->expr > -1)
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(
                    pool,
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c2->factor), c2->expr)));
        else if (c1->expr > -1) /* 2a + 3  = 1(2a+3) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(
                    pool,
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c1->factor), c1->expr),
                    csfg_expr_lit(pool, c2->factor)));
        else if (c2->expr > -1) /* 2 + 3b = 1(2+3b) */
            return set_and_check_coeff(
                out,
                1.0,
                csfg_expr_add(
                    pool,
                    csfg_expr_lit(pool, c1->factor),
                    csfg_expr_mul(
                        pool, csfg_expr_lit(pool, c2->factor), c2->expr)));
        else /* 2 + 3 = 5 */
            return set_coeff(out, c1->factor + c2->factor, -1);
    }
}

/* ------------------------------------------------------------------------- */
int csfg_poly_copy(struct csfg_poly** dst, const struct csfg_poly* src)
{
    const struct csfg_coeff* c;
    CSFG_DEBUG_ASSERT(vec_count(*dst) == 0);
    vec_for_each (src, c)
        if (csfg_poly_push(dst, *c) != 0)
            return -1;
    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_poly_add(
    struct csfg_expr_pool** pool,
    struct csfg_poly**      out,
    const struct csfg_poly* p1,
    const struct csfg_poly* p2)
{
    int i, degree, min_degree;

    degree = vec_count(p1) > vec_count(p2) ? vec_count(p1) : vec_count(p2);
    min_degree = vec_count(p1) < vec_count(p2) ? vec_count(p1) : vec_count(p2);

    if (csfg_poly_realloc(out, degree) != 0)
        return -1;
    CSFG_DEBUG_ASSERT(vec_count(*out) == 0);

    for (i = 0; i != min_degree; ++i)
    {
        struct csfg_coeff        result;
        const struct csfg_coeff* c1 = vec_get(p1, i);
        const struct csfg_coeff* c2 = vec_get(p2, i);

        if (add_coeffs(pool, c1, c2, &result) != 0)
            return -1;
        /* This can't fail because we realloc'd earlier */
        csfg_poly_push(out, result);
    }

    for (; i != degree; ++i)
    {
        const struct csfg_coeff* c1 =
            i < vec_count(p1) ? vec_get(p1, i) : vec_get(p2, i);
        /* This can't fail because we realloc'd earlier */
        csfg_poly_push(out, csfg_coeff(c1->factor, c1->expr));
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_poly_mul(
    struct csfg_expr_pool** pool,
    struct csfg_poly**      out,
    const struct csfg_poly* p1,
    const struct csfg_poly* p2)
{
    const struct csfg_coeff *c1, *c2;
    int                      i, i1, i2, degree;

    degree = vec_count(p1) + vec_count(p2) - 1;
    if (csfg_poly_realloc(out, degree) != 0)
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
        /* This can't fail because we realloc'd earlier */
        csfg_poly_push(out, csfg_coeff(factor, factor != 0.0 ? expr : -1));
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_poly_to_str(
    struct str**                 str,
    const struct csfg_expr_pool* pool,
    const struct csfg_poly*      poly)
{
    const struct csfg_coeff* c;
    int                      degree;

    vec_enumerate (poly, degree, c)
    {
        if (c->factor == 0.0)
            continue;

        if (str_len(*str) > 0)
            if (str_append_cstr(str, " + ") != 0)
                return -1;

        if (c->factor == -1.0)
        {
            if (str_append_char(str, '-') != 0)
                return -1;
        }
        else if (c->factor != 1.0)
        {
            if (str_append_float(str, c->factor) != 0)
                return -1;
        }

        if (degree > 0)
            if (str_append_char(str, 's') != 0)
                return -1;
        if (degree > 1)
        {
            if (str_append_char(str, '^') != 0)
                return -1;
            if (str_append_int(str, degree) != 0)
                return -1;
        }

        if (c->expr > -1)
        {
            int need_parens = (pool->nodes[c->expr].type == CSFG_EXPR_ADD) &&
                              (c->factor != 1.0);
            if (need_parens)
                if (str_append_char(str, '(') != 0)
                    return -1;
            if (csfg_expr_to_str(str, pool, c->expr) != 0)
                return -1;
            if (need_parens)
                if (str_append_char(str, ')') != 0)
                    return -1;
        }
    }

    if (str_len(*str) == 0)
        if (str_append_char(str, '1') != 0)
            return -1;

    return 0;
}
