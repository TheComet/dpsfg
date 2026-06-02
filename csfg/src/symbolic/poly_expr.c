#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly_expr.h"
#include "csfg/util/str.h"

VEC_DEFINE(csfg_poly_expr, struct csfg_coeff_expr, 8)

/* -------------------------------------------------------------------------- */
static int set_coeff(struct csfg_coeff_expr* c, double factor, int expr)
{
    c->factor = factor;
    c->expr   = expr;
    return 0;
}

/* -------------------------------------------------------------------------- */
static int
set_and_check_coeff(struct csfg_coeff_expr* c, double factor, int expr)
{
    if (expr < 0)
        return -1;
    return set_coeff(c, factor, expr);
}

/* -------------------------------------------------------------------------- */
static int largest_nonzero_coeff(const struct csfg_poly_expr* p)
{
    const struct csfg_coeff_expr* c;
    int i = vec_count(p) - 1;
    vec_for_each_r (p, c)
    {
        if (c->factor != 0.0)
            break;
        i--;
    }
    return i;
}

/* -------------------------------------------------------------------------- */
static int add_coeffs(
    struct csfg_expr_pool** pool,
    struct csfg_coeff_expr* out,
    const struct csfg_coeff_expr* c1,
    const struct csfg_coeff_expr* c2)
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

/* -------------------------------------------------------------------------- */
static int mul_coeffs(
    struct csfg_expr_pool** pool,
    struct csfg_coeff_expr* out,
    const struct csfg_coeff_expr* c1,
    const struct csfg_coeff_expr* c2)
{
    double factor = c1->factor * c2->factor;
    if (c1->expr > -1 && c2->expr > -1)
        return set_and_check_coeff(
            out, factor, csfg_expr_mul(pool, c1->expr, c2->expr));
    if (c1->expr > -1)
        return set_coeff(out, factor, c1->expr);
    if (c2->expr > -1)
        return set_coeff(out, factor, c2->expr);
    return set_coeff(out, factor, -1);
}

/* -------------------------------------------------------------------------- */
static int div_coeffs(
    struct csfg_expr_pool** pool,
    struct csfg_coeff_expr* out,
    const struct csfg_coeff_expr* c1,
    const struct csfg_coeff_expr* c2)
{
    double factor = c1->factor / c2->factor;
    if (c1->expr > -1 && c2->expr > -1)
        return set_and_check_coeff(
            out, factor, csfg_expr_div(pool, c1->expr, c2->expr));
    if (c1->expr > -1)
        return set_coeff(out, factor, c1->expr);
    if (c2->expr > -1)
        return set_coeff(out, factor, c2->expr);
    return set_coeff(out, factor, -1);
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_copy(
    struct csfg_poly_expr** dst, const struct csfg_poly_expr* src)
{
    const struct csfg_coeff_expr* c;
    CSFG_DEBUG_ASSERT(vec_count(*dst) == 0);
    vec_for_each (src, c)
        if (csfg_poly_expr_push(dst, *c) != 0)
            return -1;
    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_add(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** out,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2)
{
    int i, out_degree, min_degree;

    out_degree = vec_count(p1) > vec_count(p2) ? vec_count(p1) : vec_count(p2);
    min_degree = vec_count(p1) < vec_count(p2) ? vec_count(p1) : vec_count(p2);

    if (csfg_poly_expr_realloc(out, out_degree) != 0)
        return -1;
    CSFG_DEBUG_ASSERT(vec_count(*out) == 0);

    for (i = 0; i != min_degree; ++i)
    {
        struct csfg_coeff_expr result;
        const struct csfg_coeff_expr* c1 = vec_get(p1, i);
        const struct csfg_coeff_expr* c2 = vec_get(p2, i);

        if (add_coeffs(pool, &result, c1, c2) != 0)
            return -1;
        /* This can't fail because we realloc'd earlier */
        csfg_poly_expr_push_no_realloc(*out, result);
    }

    for (; i != out_degree; ++i)
    {
        const struct csfg_coeff_expr* c1 =
            i < vec_count(p1) ? vec_get(p1, i) : vec_get(p2, i);
        /* This can't fail because we realloc'd earlier */
        csfg_poly_expr_push_no_realloc(
            *out, csfg_coeff_expr(c1->factor, c1->expr));
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_mul(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** out,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2)
{
    struct csfg_coeff_expr acc_coeff;
    const struct csfg_coeff_expr *c1, *c2;
    int i, i1, i2, degree;

    degree = vec_count(p1) + vec_count(p2) - 1;
    if (csfg_poly_expr_realloc(out, degree) != 0)
        return -1;
    CSFG_DEBUG_ASSERT(vec_count(*out) == 0);

    for (i = 0; i != degree; ++i)
    {
        acc_coeff = csfg_coeff_expr(0.0, -1);
        vec_enumerate (p1, i1, c1)
            vec_enumerate (p2, i2, c2)
                if (i1 + i2 == i)
                {
                    struct csfg_coeff_expr coeff;
                    if (mul_coeffs(pool, &coeff, c1, c2) != 0)
                        return -1;

                    if (acc_coeff.expr == -1)
                        acc_coeff.expr = coeff.expr;
                    else if (coeff.expr > -1)
                    {
                        acc_coeff.expr =
                            csfg_expr_mul(pool, acc_coeff.expr, coeff.expr);
                        if (acc_coeff.expr < 0)
                            return -1;
                    }

                    acc_coeff.factor += coeff.factor;
                }

        if (acc_coeff.factor == 0.0 && acc_coeff.expr > -1)
            acc_coeff.expr = -1;

        /* This can't fail because we realloc'd earlier */
        csfg_poly_expr_push_no_realloc(*out, acc_coeff);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_div(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** quotient,
    struct csfg_poly_expr** remainder,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2)
{
    int out_degree, remainder_degree, divisor_degree, i, i2;
    struct csfg_coeff_expr coeff, sub_coeff, *c1;
    const struct csfg_coeff_expr *remainder_coeff, *c2, *divisor;
    CSFG_DEBUG_ASSERT(quotient == NULL || vec_count(*quotient) == 0);

    if (*remainder != p1)
        vec_for_each (p1, remainder_coeff)
            if (csfg_poly_expr_push(remainder, *remainder_coeff) != 0)
                return -1;

    divisor_degree = largest_nonzero_coeff(p2);
    divisor        = vec_get(p2, divisor_degree);

    while (1)
    {
        remainder_degree = csfg_poly_expr_degree(*remainder);
        out_degree       = remainder_degree - divisor_degree;
        if (out_degree < 0)
            break;

        remainder_coeff = vec_get(*remainder, remainder_degree);
        if (div_coeffs(pool, &coeff, remainder_coeff, divisor) != 0)
            return -1;
        if (quotient != NULL && csfg_poly_expr_push(quotient, coeff) != 0)
            return -1;

        vec_enumerate_range(*remainder, i, c1, 0, remainder_degree)
        {
            i2 = i - out_degree;
            if (i2 < 0 || i2 >= vec_count(p2) || vec_get(p2, i2)->factor == 0.0)
                continue;

            c2 = vec_get(p2, i2);
            if (mul_coeffs(pool, &sub_coeff, c2, &coeff) != 0)
                return -1;
            sub_coeff.factor *= -1.0;
            if (add_coeffs(pool, c1, c1, &sub_coeff) != 0)
                return -1;
        }
        csfg_poly_expr_pop(*remainder);
    }

    if (quotient != NULL)
        csfg_poly_expr_reverse(*quotient);

    while (csfg_poly_expr_degree(*remainder) > 0 &&
           vec_last(*remainder)->factor == 0.0)
    {
        csfg_poly_expr_pop(*remainder);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_gcd(
    struct csfg_expr_pool** pool,
    struct csfg_poly_expr** gcd,
    const struct csfg_poly_expr* p1,
    const struct csfg_poly_expr* p2)
{
    const struct csfg_coeff_expr* c;
    struct csfg_poly_expr *next_gcd, *tmp;
    csfg_poly_expr_init(&next_gcd);
    CSFG_DEBUG_ASSERT(vec_count(*gcd) == 0);

    if (csfg_poly_expr_realloc(gcd, vec_count(p2)) != 0 ||
        csfg_poly_expr_realloc(&next_gcd, vec_count(p1)) != 0)
        goto realloc_failed;

    vec_for_each (p1, c)
        csfg_poly_expr_push_no_realloc(next_gcd, *c);
    vec_for_each (p2, c)
        csfg_poly_expr_push_no_realloc(*gcd, *c);

    while (1)
    {
        csfg_poly_expr_div(pool, NULL, &next_gcd, next_gcd, *gcd);
        if (csfg_poly_expr_degree(next_gcd) <= 0)
            break;
        tmp      = next_gcd;
        next_gcd = *gcd;
        *gcd     = tmp;
    }

    csfg_poly_expr_deinit(next_gcd);
    return 0;

realloc_failed:
    csfg_poly_expr_deinit(next_gcd);
    return -1;
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_to_str(
    struct str** str,
    const struct csfg_expr_pool* pool,
    const struct csfg_poly_expr* poly)
{
    const struct csfg_coeff_expr* c;
    int degree;

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
            if (c->expr < 0)
                if (str_append_char(str, '1') != 0)
                    return -1;
        }
        else if (c->factor != 1.0 || c->expr < 0)
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
